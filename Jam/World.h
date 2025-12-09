#ifndef WORLD_H
#define WORLD_H

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>

class World
{
public:
    World(b2World& worldRef);

    const uint16 CATEGORY_PLAYER = 0x0001;   // 0001
    const uint16 CATEGORY_GROUND = 0x0002;   // 0010
    const uint16 CATEGORY_OBSTACLE = 0x0004; // 0100
    const uint16 CATEGORY_SENSOR = 0x0008;   // 1000

    sf::RectangleShape mHitRect;  // appears on collision
    bool mIsColliding = false;

    struct Obstacle {
        b2Body* body;
        sf::RectangleShape shape;
        bool onlyGround;

        Obstacle(b2Body* b, const sf::RectangleShape& s, bool og)
            : body(b), shape(s), onlyGround(og) {
        }
    };

    std::vector<Obstacle> obstacles;

    void createObstacle(float x, float y, bool isSensor);
    void update(float dt);
    void draw(sf::RenderWindow& window);
    void checkCollision(const sf::RectangleShape& playerShape, sf::RenderWindow& window);

private:
    b2World& physicsWorld;
};

#endif // WORLD_H