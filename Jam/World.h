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
    static constexpr uint16 CATEGORY_PLAYER   = 0x0001;
    static constexpr uint16 CATEGORY_GROUND   = 0x0002;
    static constexpr uint16 CATEGORY_OBSTACLE = 0x0004;
    static constexpr uint16 CATEGORY_SENSOR   = 0x0008;

    struct ParallaxLayer {
        sf::Texture texture;
        sf::Sprite  sprite;
        float baseYOffset = 0.f;
        float baseXOffset = 0.f;
        float scale = 1.f;
        float speedX = 0.f;
        float speedY = 0.f;
    };

    struct Obstacle {
        b2Body* body;
        sf::RectangleShape shape;
        bool onlyGround;
        size_t textureIndex;

        // New: initial state to allow resets
        b2Vec2       startPosB2       {0.f, 0.f};
        float        startAngle       = 0.f;
        b2BodyType   startType        = b2_staticBody;
        uint16       initialCategoryBits = 0;
        uint16       initialMaskBits     = 0;

        Obstacle(b2Body* b, const sf::RectangleShape& s, bool og, size_t texIdx)
            : body(b), shape(s), onlyGround(og), textureIndex(texIdx) {}
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

    void createObstacle(float x, float y, bool onlyGround, float sx, float sy, const std::string& texFile);

    int   findObstacleByTextureSubstring(const std::string& substr) const;
    b2Vec2 getObstacleBodyPosition(int index) const;
    int   getLastCollidedObstacleIndex() const;
    Obstacle* getObstacleByTexture(size_t textureIndex);

    // Game-over trigger handshake with Game
    bool consumeGameOverTrigger();

    // New: Reset the whole world (obstacles, flags)
    void ResetWorld();

private:
    b2World& physicsWorld;

    // Parallax
    std::vector<ParallaxLayer> parallaxLayers;
    void initParallax();
    void updateParallax(const sf::Vector2f& camPos);
    bool  parallaxAligned = false;
    float parallaxYOffset = 0.f;

    // Obstacles
    std::vector<Obstacle> obstacles;
    std::vector<sf::Texture> obstacleTextures;
    std::vector<std::string> obstacleTextureFiles;

    // Collision debug info
    bool mIsColliding = false;
    int  lastCollidedObstacleIndex = -1;

    // Game over trigger
    bool mGameOverTriggered = false;

    // Level extents (pixels)
    float levelMinX = 1e9f;
    float levelMaxX = -1e9f;

    // Helpers
    void resetObstacle(Obstacle& o);
};