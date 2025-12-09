#include "Player.h"
#include "World.h"
#include "Game.h"
#include "Units.h"
#include <sstream>

using namespace sf;

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
    boxFixture.friction = 0.3f;
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
    m_sprite.setScale(0.25f, 0.25f);

    // Run frames
    std::vector<std::string> runPaths;
    runPaths.reserve(8);
    const std::string baseRun = "Assets/Player/Run/";
    for (int i = 1; i <= 8; ++i) {
        runPaths.emplace_back(baseRun + "Run" + std::to_string(i) + ".png");
    }
    bool okRun = m_anim.AddClip("Run", runPaths, 0.08f, true);

    // Walk frames (Shift)
    std::vector<std::string> walkPaths;
    walkPaths.reserve(7);
    const std::string baseWalk = "Assets/Player/Walk/";
    for (int i = 1; i <= 7; ++i) {
        walkPaths.emplace_back(baseWalk + "Walk" + std::to_string(i) + ".png");
    }
    bool okWalk = m_anim.AddClip("Walk", walkPaths, 0.10f, true);

    // Idle frame (single)
    const std::string baseIdle = "Assets/Player/Run/";
    std::vector<std::string> idlePaths = { baseIdle + "Idle.png" };
    bool okIdle = m_anim.AddClip("Idle", idlePaths, 0.2f, true);

    if (!okRun && !okIdle && !okWalk) {
        sf::Image img;
        img.create(50, 50, Color::Red);
        sf::Texture t;
        t.loadFromImage(img);
        t.setSmooth(true);
        m_sprite.setTexture(t);
        m_sprite.setOrigin(25.f, 25.f);
    }
    else {
        // Prefer Idle if available
        m_anim.SetClip(okIdle ? "Idle" : (okWalk ? "Walk" : "Run"), true);
    }

    m_sprite.setColor(Color::Red);
    m_anim.SetFacingRight(true);

    CreateFixturesFromSpriteBounds(m_body, m_footFixture, m_sprite);
    SyncGraphics();
}

Player::~Player()
{
    m_body = nullptr;
    m_footFixture = nullptr;
}

void Player::Update(float dt, bool /*grounded*/)
{
    if (!m_body) return;
    b2Vec2 vel = m_body->GetLinearVelocity();

    if (vel.x > 0.01f) {
        if (!m_facingRight) { m_facingRight = true; m_anim.SetFacingRight(true); }
    }
    else if (vel.x < -0.01f) {
        if (m_facingRight) { m_facingRight = false; m_anim.SetFacingRight(false); }
    }

    const float moveEpsilon = 0.05f;
    const bool isMoving = std::abs(vel.x) > moveEpsilon;

    // Choose clip: Idle, Walk (Shift), Run
    std::string desiredClip;
    if (!isMoving) {
        desiredClip = "Idle";
    }
    else {
        desiredClip = m_isWalking ? "Walk" : "Run";
    }

    const bool clipChanged = (m_anim.CurrentClip() != desiredClip);
    if (clipChanged) {
        m_anim.SetClip(desiredClip, true);
        CreateFixturesFromSpriteBounds(m_body, m_footFixture, m_sprite);
    }

    // Audio mood from color (Magenta/Cyan => Crazy)
    const sf::Color c = m_sprite.getColor();
    const bool isCrazyVisual = (c == sf::Color::Magenta || c == sf::Color::Cyan);
    m_audioState = isCrazyVisual ? PlayerAudioState::Crazy : PlayerAudioState::Neutral;

    m_anim.Update(dt);
}

void Player::SyncGraphics()
{
    if (!m_body) return;
    b2Vec2 pos = m_body->GetPosition();
    m_sprite.setPosition(pos.x * Units::PPM, pos.y * Units::PPM);
}

void Player::Draw(RenderWindow& window)
{
    SyncGraphics();
    window.draw(m_sprite);
}