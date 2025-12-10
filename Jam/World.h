#pragma once
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <vector>
#include <string>

class World
{
public:
    World(b2World& worldRef);

    // Collision categories
    static constexpr uint16 CATEGORY_PLAYER = 0x0001;
    static constexpr uint16 CATEGORY_GROUND = 0x0002;
    static constexpr uint16 CATEGORY_OBSTACLE = 0x0004;
    static constexpr uint16 CATEGORY_SENSOR = 0x0008;

    // ---------------------------------------------------------------------
    // PARALLAX LAYER STRUCTURE
    // ---------------------------------------------------------------------
    struct ParallaxLayer {
        sf::Texture texture;
        sf::Sprite sprite;

        float speedX = 0.f; // horizontal factor
        float speedY = 0.f; // vertical factor
    };

    // ---------------------------------------------------------------------
    // OBSTACLE STRUCTURE
    // ---------------------------------------------------------------------
    struct Obstacle {
        b2Body* body;
        sf::RectangleShape shape;
        bool onlyGround;
        size_t textureIndex;

        Obstacle(b2Body* b, const sf::RectangleShape& s, bool og, size_t texIdx)
            : body(b), shape(s), onlyGround(og), textureIndex(texIdx) {
        }
    };

    // ---------------------------------------------------------------------
    // PUBLIC API
    // ---------------------------------------------------------------------
    void update(float dt, const sf::Vector2f& camPos);
    void draw(sf::RenderWindow& window);
    void checkCollision(const sf::RectangleShape& playerShape);

    void createObstacle(
        float x, float y,
        bool onlyGround,
        float sx, float sy,
        const std::string& texFile
    );

private:
    b2World& physicsWorld;

    // ---------------------------------------------------------------------
    // PARALLAX SYSTEM
    // ---------------------------------------------------------------------
    std::vector<ParallaxLayer> parallaxLayers;
    void initParallax();
    void updateParallax(const sf::Vector2f& camPos);
    void drawParallax(sf::RenderWindow& window);

    // Y-offset fix (applied only once)
    bool parallaxAligned = false;
    float parallaxYOffset = 0.f;

    // ---------------------------------------------------------------------
    // OBSTACLES
    // ---------------------------------------------------------------------
    std::vector<Obstacle> obstacles;
    std::vector<sf::Texture> obstacleTextures;

    // Collision debug info
    bool mIsColliding = false;
};
