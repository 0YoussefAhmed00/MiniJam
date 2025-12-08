#include "Player.h"

using namespace sf;

Player::Player(b2World* world, float startX, float startY)
    : m_world(world), m_body(nullptr), m_footFixture(nullptr), m_shape(Vector2f(50.f, 50.f))
{
    if (!m_world) return;

    // Create dynamic body
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(startX * Units::INV_PPM, startY * Units::INV_PPM);
    boxDef.fixedRotation = true;
    m_body = m_world->CreateBody(&boxDef);

    // Main body fixture (Box2D expects half-width/half-height in meters)
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(25.f * Units::INV_PPM, 25.f * Units::INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.3f;
    m_body->CreateFixture(&boxFixture);

    // Foot sensor: half-width 22px -> 22, half-height 5px -> 5, offset down by 25px
    b2PolygonShape footShape;
    footShape.SetAsBox(
        22.f * Units::INV_PPM,
        5.f * Units::INV_PPM,
        b2Vec2(0.f, 25.f * Units::INV_PPM),
        0.f
    );

    b2FixtureDef footFixtureDef;
    footFixtureDef.shape = &footShape;
    footFixtureDef.isSensor = true;
    m_footFixture = m_body->CreateFixture(&footFixtureDef);

    // SFML shape
    m_shape.setOrigin(25.f, 25.f);
    m_shape.setFillColor(Color::Red);
    SyncGraphics();
}

Player::~Player()
{
    // Box2D bodies are owned by the world – do not destroy here.
    m_body = nullptr;
    m_footFixture = nullptr;
}

void Player::SyncGraphics()
{
    if (!m_body) return;
    b2Vec2 pos = m_body->GetPosition();
    m_shape.setPosition(pos.x * Units::PPM, pos.y * Units::PPM);
}

void Player::Draw(RenderWindow& window)
{
    SyncGraphics();
    window.draw(m_shape);
}