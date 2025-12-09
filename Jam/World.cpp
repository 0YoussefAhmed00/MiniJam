#include "World.h"
#include<iostream>
constexpr float PPM = 30.f;      // Pixels per meter
constexpr float INV_PPM = 1.f / PPM;

World::World(b2World& worldRef)
    : physicsWorld(worldRef)       // Gravity downward
{
    // Create ALL obstacles here
    createObstacle(500, 620, false, 100, 100, "Assets/Obstacles/Untitled-2.png");   // Dynamic obstacle
    createObstacle(700, 620, false, 200, 200, "Assets/Obstacles/box2.png"); // Static (ground-only) obstacle
}

void World::createObstacle(float x, float y, bool onlyGround, float scaleX, float scaleY, const std::string& textureFile)

{
    // Load texture and store it
    obstacleTextures.emplace_back();
    if (!obstacleTextures.back().loadFromFile(textureFile))
        std::cerr << "Failed to load texture: " << textureFile << std::endl;
    obstacleTextures.back().setRepeated(false);

    sf::Vector2u texSize = obstacleTextures.back().getSize();

    // -------- BOX2D BODY --------
    b2BodyDef bodyDef;
    bodyDef.position.Set(x * INV_PPM, y * INV_PPM);
    bodyDef.type = onlyGround ? b2_staticBody : b2_dynamicBody;

    b2Body* body = physicsWorld.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(scaleX / 2 * INV_PPM, scaleY / 2 * INV_PPM); // Box2D half-size

    b2FixtureDef fixture;
    fixture.shape = &box;
    fixture.density = onlyGround ? 0.f : 2.f;
    fixture.filter.categoryBits = CATEGORY_OBSTACLE;
    fixture.filter.maskBits = onlyGround ? (CATEGORY_GROUND | CATEGORY_PLAYER) : CATEGORY_GROUND;

    body->CreateFixture(&fixture);

    // -------- SFML SHAPE --------
    sf::RectangleShape shape(sf::Vector2f(scaleX, scaleY));
    shape.setOrigin(scaleX / 2, scaleY / 2);
    shape.setPosition(x, y);
    shape.setTexture(&obstacleTextures.back());
    shape.setTextureRect(sf::IntRect(0, 0, texSize.x, texSize.y));

    // Store obstacle with texture index
    obstacles.emplace_back(body, shape, onlyGround, obstacleTextures.size() - 1);
}

void World::update(float dt)
{
    physicsWorld.Step(dt, 8, 3);

    for (auto& obj : obstacles)
    {
        b2Vec2 pos = obj.body->GetPosition();
        float angle = obj.body->GetAngle();

        obj.shape.setPosition(pos.x * PPM, pos.y * PPM);
        obj.shape.setRotation(angle * 180.f / 3.14159f);
    }
}

void World::checkCollision(const sf::RectangleShape& playerShape)
{
    sf::FloatRect playerBounds = playerShape.getGlobalBounds();
    mIsColliding = false;

    for (auto& obj : obstacles)
    {
        sf::FloatRect obsBounds = obj.shape.getGlobalBounds();

        if (playerBounds.intersects(obsBounds))
        {
            mIsColliding = true;

            if (obj.textureIndex == 0)
                obj.shape.setFillColor(sf::Color::Yellow);
            else if (obj.textureIndex == 1)
                obj.shape.setFillColor(sf::Color::Blue);
        }
        else
        {
            obj.shape.setFillColor(obj.onlyGround ? sf::Color::Red : sf::Color::White);

            // Use obstacle-specific texture
            sf::Texture& tex = obstacleTextures[obj.textureIndex];
            obj.shape.setTexture(&tex);
            sf::Vector2u texSize = tex.getSize();
            obj.shape.setTextureRect(sf::IntRect(0, 0, texSize.x, texSize.y));
        }
    }
}

void World::draw(sf::RenderWindow& window)
{
    for (auto& obj : obstacles)
        window.draw(obj.shape);
}