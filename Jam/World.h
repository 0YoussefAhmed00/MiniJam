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
        
        float baseYOffset = 0.f;
        float baseXOffset = 0.f;
        float scale = 1.f;
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
    void drawParallaxBackground(sf::RenderWindow& window);
    void drawParallaxForeground(sf::RenderWindow& window);
    void drawLayer(sf::RenderWindow& window, ParallaxLayer& layer);

    void createObstacle(
        float x, float y,
        bool onlyGround,
        float sx, float sy,
        const std::string& texFile
    );

    // Split parallax draw so caller can insert player rendering between ground and foreground

private:
    b2World& physicsWorld;

    // ---------------------------------------------------------------------
    // PARALLAX SYSTEM
    // ---------------------------------------------------------------------
    std::vector<ParallaxLayer> parallaxLayers;
    void initParallax();
    void updateParallax(const sf::Vector2f& camPos);
    
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

    // level extents (pixels)
    float levelMinX = 1e9f;
    float levelMaxX = -1e9f;

    // Draw ground from parallax layer #9 (index8)
    // (kept public wrapper, internal implementation unchanged)
    // void drawGround(sf::RenderWindow& window);
};
