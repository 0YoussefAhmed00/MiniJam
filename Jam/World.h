#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <string>

class World
{
public:
    World(b2World& worldRef);

    const uint16 CATEGORY_PLAYER = 0x0001;
    const uint16 CATEGORY_GROUND = 0x0002;
    const uint16 CATEGORY_OBSTACLE = 0x0004;
    const uint16 CATEGORY_SENSOR = 0x0008;

    sf::RectangleShape mHitRect;
    bool mIsColliding = false;

    struct Obstacle {
        b2Body* body;
        sf::RectangleShape shape;
        bool onlyGround;
        size_t textureIndex; // index in obstacleTextures

        Obstacle(b2Body* b, const sf::RectangleShape& s, bool og, size_t texIdx)
            : body(b), shape(s), onlyGround(og), textureIndex(texIdx) {
        }
    };

    std::vector<Obstacle> obstacles;
    std::vector<sf::Texture> obstacleTextures;

    void createObstacle(float x, float y, bool onlyGround, float scaleX, float scaleY, const std::string& textureFile);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void checkCollision(const sf::RectangleShape& playerShape);

private:
    b2World& physicsWorld;
};