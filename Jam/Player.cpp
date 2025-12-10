#include "Player.h"
#include "World.h"
#include "Game.h"
#include "Units.h"
#include <sstream>
#include <iostream>

using namespace sf;
using namespace std;

// ------------------------------------------------------------
//  FIXTURE REBUILD (unchanged)
// ------------------------------------------------------------
static void CreateFixturesFromSpriteBounds(b2Body* body, b2Fixture*& footFixture, const sf::Sprite& sprite)
{
    for (b2Fixture* f = body->GetFixtureList(); f; ) {
        b2Fixture* next = f->GetNext();
        body->DestroyFixture(f);
        f = next;
    }

    const sf::FloatRect gb = sprite.getGlobalBounds();
    const float halfWidthPx = gb.width * 0.5f;
    const float halfHeightPx = gb.height * 0.5f;

    constexpr float COLLISION_WIDTH_SCALE = 0.45f;
    constexpr float COLLISION_HEIGHT_SCALE = 0.9f;

    const float bodyHalfWidthPx = halfWidthPx * COLLISION_WIDTH_SCALE;
    const float bodyHalfHeightPx = halfHeightPx * COLLISION_HEIGHT_SCALE;

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(bodyHalfWidthPx * Units::INV_PPM, bodyHalfHeightPx * Units::INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.f;
    boxFixture.filter.categoryBits = 0x0001;
    body->CreateFixture(&boxFixture);

    const float footHalfWidthPx = std::max(4.f, (gb.width - 6.f) * 0.5f);
    const float footHalfHeightPx = std::max(2.f, gb.height * 0.04f);
    const float footOffsetYPx = halfHeightPx;

    b2PolygonShape footShape;
    footShape.SetAsBox(
        footHalfWidthPx * Units::INV_PPM,
        footHalfHeightPx * Units::INV_PPM,
        b2Vec2(0.f, footOffsetYPx * Units::INV_PPM),
        0.f
    );

    b2FixtureDef footFixtureDef;
    footFixtureDef.shape = &footShape;
    footFixtureDef.isSensor = true;
    footFixture = body->CreateFixture(&footFixtureDef);
}

// ------------------------------------------------------------
//  CONSTRUCTOR
// ------------------------------------------------------------
Player::Player(b2World* world, float startX, float startY)
    : m_world(world),
    m_body(nullptr),
    m_footFixture(nullptr),
    m_facingRight(true),
    m_isWalking(false),
    m_audioState(PlayerAudioState::Neutral)
{
    if (!m_world) return;

    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(startX * Units::INV_PPM, startY * Units::INV_PPM);
    boxDef.fixedRotation = true;
    m_body = m_world->CreateBody(&boxDef);

    m_anim.BindSprite(&m_sprite);
    m_sprite.setScale(0.33f, 0.33f);

    // -----------------------------------------------------
    //        NORMAL ANIMATIONS (sprite only, no tint)
    // -----------------------------------------------------

    // Run
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Run/";
        for (int i = 1; i <= 8; i++)
            paths.push_back(base + "Run" + std::to_string(i) + ".png");
        m_anim.AddClip("Run", paths, 0.08f, true);
    }

    // Walk
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Walk/";
        for (int i = 1; i <= 7; i++)
            paths.push_back(base + "Walk" + std::to_string(i) + ".png");
        m_anim.AddClip("Walk", paths, 0.10f, true);
    }

    // Idle
    {
        std::vector<std::string> paths = { "Assets/Player/Run/Idle.png" };
        m_anim.AddClip("Idle", paths, 0.2f, true);
    }

    // Jump
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Jump/";
        for (int i = 1; i <= 6; i++)
            paths.push_back(base + std::to_string(i) + ".png");
        m_anim.AddClip("Jump", paths, 0.10f, false);
    }
    

    // Angry variants are kept available (no color trigger), use programmatic state if needed
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Angry walk/";
        for (int i = 1; i <= 7; i++)
            paths.push_back(base + std::to_string(i) + ".png");
        m_anim.AddClip("AngryWalk", paths, 0.10f, true);
    }
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Angry run/";
        for (int i = 1; i <= 8; i++)
            paths.push_back(base + std::to_string(i) + ".png");
        m_anim.AddClip("AngryRun", paths, 0.08f, true);
    }
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Angry jump/";
        for (int i = 1; i <= 3; i++)
            paths.push_back(base + std::to_string(i) + ".png");
        m_anim.AddClip("AngryJump", paths, 0.10f, false);
    }
    {
        std::vector<std::string> paths = { "Assets/Player/Angry walk/Q.png" };
        m_anim.AddClip("AngryIdle", paths, 0.2f, true);
    }

    // Wave (played once during input lock)
    {
        std::vector<std::string> paths;
        const std::string base = "Assets/Player/Wave/";
        for (int i = 1; i <= 5; i++)
            paths.push_back(base + std::to_string(i) + ".png");
        m_anim.AddClip("Wave", paths, 0.12f, false);   // <-- NOT looping
    }
    m_anim.SetClip("Idle", true);
    m_anim.SetFacingRight(true);

    // Ensure no tint is applied; use sprite’s original texture colors
    m_sprite.setColor(sf::Color::White);

    CreateFixturesFromSpriteBounds(m_body, m_footFixture, m_sprite);
    SyncGraphics();
}

Player::~Player()
{
    m_body = nullptr;
    m_footFixture = nullptr;
}

// ------------------------------------------------------------
//  UPDATE
// ------------------------------------------------------------
void Player::Update(float dt, bool grounded)
{
    if (!m_body) return;

    b2Vec2 vel = m_body->GetLinearVelocity();

    // Facing
    if (vel.x > 0.01f && !m_facingRight) {
        m_facingRight = true;
        m_anim.SetFacingRight(true);
    }
    else if (vel.x < -0.01f && m_facingRight) {
        m_facingRight = false;
        m_anim.SetFacingRight(false);
    }

    const bool isMoving = std::abs(vel.x) > 0.05f;

    // Choose animation WITHOUT color-based psycho detection
    std::string desired;

    // If you still want to use audio state to drive "angry" look, you can use m_audioState.
    const bool psycho = (m_audioState == PlayerAudioState::Crazy);

    if (m_playingWave)
    {
        int currentFrame = m_anim.CurrentFrameIndex();

        // Detect when animation freezes at last frame
        if (currentFrame == m_lastWaveFrame)
            m_waveTimer += dt;
        else
            m_waveTimer = 0.f; // reset because still animating

        m_lastWaveFrame = currentFrame;

        // If non-looping clip stops changing frames → finished
        if (m_waveTimer > 0.15f)
        {
            m_playingWave = false;
            // DO NOT return, fall through to normal state logic
        }
        else
        {
            // Still playing wave animation
            m_anim.Update(dt);
            return;
        }
    }

    if (!grounded)
    {
        desired = psycho ? "AngryJump" : "Jump";
    }
    else
    {
        if (!isMoving)
            desired = psycho ? "AngryIdle" : "Idle";
        else
            desired = psycho ? (m_isWalking ? "AngryWalk" : "AngryRun")
            : (m_isWalking ? "Walk" : "Run");
    }

    if (m_anim.CurrentClip() != desired)
    {
        m_anim.SetClip(desired, true);

        if (desired == "Jump" || desired == "AngryJump")
            m_anim.Reset();

        CreateFixturesFromSpriteBounds(m_body, m_footFixture, m_sprite);
    }

    

    m_anim.Update(dt);
}



// ------------------------------------------------------------
void Player::SyncGraphics()
{
    if (!m_body) return;
    b2Vec2 pos = m_body->GetPosition();
    m_sprite.setPosition(pos.x * Units::PPM, pos.y * Units::PPM);
}

void Player::PlayWave()
{
    if (!m_playingWave)
    {
        m_playingWave = true;
        m_waveTimer = 0.f;
        m_lastWaveFrame = -1;

        m_anim.SetClip("Wave", true);
        m_anim.Reset();
    }
}




// ------------------------------------------------------------
void Player::Draw(RenderWindow& window)
{
    SyncGraphics();
    window.draw(m_sprite);
}