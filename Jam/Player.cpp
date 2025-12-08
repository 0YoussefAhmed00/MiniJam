#include "Player.h"
#include <sstream>

using namespace sf;

static void CreateFixturesFromSpriteBounds(b2Body* body, b2Fixture*& footFixture, const sf::Sprite& sprite)
{
    // Remove existing fixtures (optional: keep if they match). We remove both main and foot.
    // Iterate and destroy to ensure no leaks.
    for (b2Fixture* f = body->GetFixtureList(); f; ) {
        b2Fixture* next = f->GetNext();
        body->DestroyFixture(f);
        f = next;
    }

    // Get visual bounds in pixels (includes scaling)
    const sf::FloatRect gb = sprite.getGlobalBounds();
    const float halfWidthPx = gb.width * 0.5f;
    const float halfHeightPx = gb.height * 0.5f;

    // Main body sized to sprite
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(halfWidthPx * Units::INV_PPM, halfHeightPx * Units::INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.3f;
    body->CreateFixture(&boxFixture);

    // Foot sensor under the body to detect ground contact.
    // Make it a thin strip at the bottom, width slightly narrower than sprite.
    const float footHalfWidthPx = std::max(4.f, (gb.width - 6.f) * 0.5f);   // small inset
    const float footHalfHeightPx = std::max(2.f, gb.height * 0.04f);        // ~8% of height, min 4px total
    const float footOffsetYPx = halfHeightPx;                             // at bottom edge

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
    m_facingRight(true)
{
    if (!m_world) return;

    // Create dynamic body
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(startX * Units::INV_PPM, startY * Units::INV_PPM);
    boxDef.fixedRotation = true;
    m_body = m_world->CreateBody(&boxDef);

    // Bind sprite to animation
    m_anim.BindSprite(&m_sprite);
    m_sprite.setScale(0.25f, 0.25f); // your chosen visual scale

    // Load Run1..Run8 from "Assets/Player/Run/Run#.png"
    std::vector<std::string> runPaths;
    runPaths.reserve(8);
    const std::string base = "Assets/Player/Run/"; // forward slashes work cross-platform
    for (int i = 1; i <= 8; ++i) {
        runPaths.emplace_back(base + "Run" + std::to_string(i) + ".png");
    }
    bool okRun = m_anim.AddClip("Run", runPaths, 0.08f, true);

    // Load Idle from "Assets/Player/Run/Idle.png"
    std::vector<std::string> idlePaths = { base + "Idle.png" };
    bool okIdle = m_anim.AddClip("Idle", idlePaths, 0.2f, true);

    if (!okRun && !okIdle) {
        // Fallback: visual red block
        sf::Image img;
        img.create(50, 50, Color::Red);
        sf::Texture t;
        t.loadFromImage(img);
        t.setSmooth(true);
        m_sprite.setTexture(t);
        m_sprite.setOrigin(25.f, 25.f);
    }
    else {
        // Prefer Idle on start if available, otherwise Run
        m_anim.SetClip(okIdle ? "Idle" : "Run", true);
    }

    // Initial tint and facing
    m_sprite.setColor(Color::Red);
    m_anim.SetFacingRight(true);

    // Create physics fixtures to match current sprite bounds and scale
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

    // Update facing based on movement direction
    if (vel.x > 0.01f) {
        if (!m_facingRight) { m_facingRight = true; m_anim.SetFacingRight(true); }
    }
    else if (vel.x < -0.01f) {
        if (m_facingRight) { m_facingRight = false; m_anim.SetFacingRight(false); }
    }

    // Switch between Run and Idle based on velocity
    const float moveEpsilon = 0.05f;
    const bool isMoving = std::abs(vel.x) > moveEpsilon;
    const std::string desiredClip = isMoving ? "Run" : "Idle";
    const bool clipChanged = (m_anim.CurrentClip() != desiredClip);
    if (clipChanged) {
        m_anim.SetClip(desiredClip, true);
        // Rebuild fixtures in case the frame size differs (and for safety after clip changes)
        CreateFixturesFromSpriteBounds(m_body, m_footFixture, m_sprite);
    }

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