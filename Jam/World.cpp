#include "World.h"
#include<iostream>
constexpr float PPM = 30.f;      // Pixels per meter
constexpr float INV_PPM = 1.f / PPM;

World::World(b2World& worldRef)
    : physicsWorld(worldRef)       // Gravity downward
{
    // Create ALL obstacles here
    createObstacle(700, 650, true);      // Static (ground-only) obstacle
    createObstacle(500, 650, false);     // Dynamic obstacle
}

void World::createObstacle(float x, float y, bool onlyGround)
{
    // -------- BOX2D BODY --------
    b2BodyDef bodyDef;
    bodyDef.position.Set(x * INV_PPM, y * INV_PPM);
    bodyDef.type = onlyGround ? b2_staticBody : b2_dynamicBody;

    b2Body* body = physicsWorld.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(20 * INV_PPM, 20 * INV_PPM);

    b2FixtureDef fixture;
    fixture.shape = &box;
    fixture.density = onlyGround ? 0.f : 2.f;

    // Collision filtering
    fixture.filter.categoryBits = CATEGORY_OBSTACLE;

    if (onlyGround)
        fixture.filter.maskBits = CATEGORY_GROUND | CATEGORY_PLAYER;
    else
        fixture.filter.maskBits = CATEGORY_GROUND;  // dynamic obstacle collides only with player

    body->CreateFixture(&fixture);

    // -------- SFML SHAPE --------
    sf::RectangleShape shape(sf::Vector2f(40, 40));
    shape.setOrigin(20, 20);
    shape.setPosition(x, y);

    shape.setFillColor(onlyGround ? sf::Color::Red : sf::Color::White);

    // Store obstacle
    obstacles.emplace_back(body, shape, onlyGround);
}

void World::update(float dt)
{
    // Step physics
    physicsWorld.Step(dt, 8, 3);

    // Sync SFML with Box2D
    for (auto& obj : obstacles)
    {
        b2Body* body = obj.body;
        sf::RectangleShape& shape = obj.shape;

        b2Vec2 pos = body->GetPosition();
        float angle = body->GetAngle();

        shape.setPosition(pos.x * PPM, pos.y * PPM);
        shape.setRotation(angle * 180.f / 3.14159f);
    }
}


void World::checkCollision(const sf::RectangleShape& playerShape, sf::RenderWindow& window)
{
    sf::FloatRect playerBounds = playerShape.getGlobalBounds();

    mIsColliding = false;

    for (auto& obj : obstacles)
    {
        sf::FloatRect obsBounds = obj.shape.getGlobalBounds();

        if (playerBounds.intersects(obsBounds))
        {
            mIsColliding = true;
            obj.shape.setFillColor(sf::Color::Yellow); 
        }
        else
        {
            obj.shape.setFillColor(obj.onlyGround ? sf::Color::Red : sf::Color::White);
        }
    }
}

void World::draw(sf::RenderWindow& window)
{
    for (auto& obj : obstacles)
        window.draw(obj.shape);
}