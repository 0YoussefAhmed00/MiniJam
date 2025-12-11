#include "World.h"
#include "AudioEmitter.h" // kept from first version (safe if present)
#include <iostream>

#include <SFML/Graphics.hpp>
constexpr float PPM = 30.f;      // Pixels per meter
constexpr float INV_PPM = 1.f / PPM;

World::World(b2World& worldRef)
    : physicsWorld(worldRef)       // Gravity downward
{
    initParallax();

    // Create ALL obstacles here (from the second / latest version)
    createObstacle(410, 588 + 210, true, 170, 170, "Assets/Obstacles/Untitled-2.png");   // Dynamic obstacle 
    createObstacle(700, 470 + 210, true, 450, 400, "Assets/Obstacles/foull car.png"); // Static (ground-only) obstacle
    createObstacle(1800, 646 + 210, true, 200, 40, "Assets/Obstacles/Closed_sewers_cap.png");

    createObstacle(2890, 630 + 210, false, 100, 30, "Assets/Obstacles/sewers_cap1.png");
    createObstacle(2900, 620 + 210, false, 300, 140, "Assets/Obstacles/sewers.png");
    createObstacle(3800, 600 + 170, false, 250 * 1.1f, 170 * 1.2f, "Assets/Obstacles/grocery.png");

    createObstacle(4825, 0 + 210, false, 120, 80, "Assets/Obstacles/the shit.png");

    createObstacle(4800, 0 + 210, false, 150, 100, "Assets/Obstacles/1.png");
    createObstacle(4700, 600 + 210, false, 600, 100, "Assets/Obstacles/Untitled-2.png");

    createObstacle(6000, 640 + 170, false, 220, 220, "Assets/Obstacles/doggie.png");

    createObstacle(7200, -100 + 210, false, 250, 150, "Assets/Obstacles/man falling.png");
    createObstacle(7200, 600 + 210, false, 400, 100, "Assets/Obstacles/Untitled-2.png");
}

void World::createObstacle(float x, float y, bool onlyGround, float scaleX, float scaleY, const std::string& textureFile)
{
    // Load texture and store it
    obstacleTextures.emplace_back();
    if (!obstacleTextures.back().loadFromFile(textureFile))
        std::cerr << "Failed to load texture: " << textureFile << std::endl;
    obstacleTextures.back().setRepeated(false);

    // keep track of filename for substring searches
    obstacleTextureFiles.push_back(textureFile);

    sf::Vector2u texSize = obstacleTextures.back().getSize();

    // -------- BOX2D BODY --------
    b2BodyDef bodyDef;
    bodyDef.position.Set(x * INV_PPM, y * INV_PPM);
    bodyDef.type = b2_staticBody;

    b2Body* body = physicsWorld.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(scaleX / 2.f * INV_PPM, scaleY / 2.f * INV_PPM); // Box2D half-size

    b2FixtureDef fixture;
    fixture.shape = &box;
    fixture.density = onlyGround ? 0.f : 2.f;
    fixture.filter.categoryBits = CATEGORY_OBSTACLE;
    fixture.filter.maskBits = onlyGround ? (CATEGORY_GROUND | CATEGORY_PLAYER) : CATEGORY_GROUND;
    fixture.friction = 0.0f;

    
    body->CreateFixture(&fixture);

    // -------- SFML SHAPE --------
    sf::RectangleShape shape(sf::Vector2f(scaleX, scaleY));
    shape.setOrigin(scaleX / 2.f, scaleY / 2.f);
    shape.setPosition(x, y);
    shape.setTexture(&obstacleTextures.back());
    shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));

    // Store obstacle with texture index
    obstacles.emplace_back(body, shape, onlyGround, obstacleTextures.size() - 1);

    // Update level extents (in pixels)
    float left = x - scaleX * 0.5f;
    float right = x + scaleX * 0.5f;
    if (left < levelMinX) levelMinX = left;
    if (right > levelMaxX) levelMaxX = right;
}

World::Obstacle* World::getObstacleByTexture(size_t textureIndex)
{
    for (auto& o : obstacles)
    {
        if (o.textureIndex == textureIndex)
            return &o;
    }
    return nullptr;
}

// ======================================================================
// WORLD UPDATE
// ======================================================================
void World::update(float dt, const sf::Vector2f& camPos)
{
    physicsWorld.Step(dt, 8, 3);

    updateParallax(camPos);

    // Sync obstacles
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

    // reset last collided index each frame
    lastCollidedObstacleIndex = -1;

    for (size_t i = 0; i < obstacles.size(); ++i)
    {
        auto& obj = obstacles[i];
        sf::FloatRect obsBounds = obj.shape.getGlobalBounds();

        if (playerBounds.intersects(obsBounds))
        {
            mIsColliding = true;

            // record first collided obstacle index
            if (lastCollidedObstacleIndex == -1)
                lastCollidedObstacleIndex = static_cast<int>(i);

            // Collision behavior mapping (combined & cleaned up)
            // Yellow: some obstacle markers (1,3)
            if (obj.textureIndex == 1 || obj.textureIndex == 3)
            {
                obj.shape.setFillColor(sf::Color::Yellow);
            }
            // Magenta: index 5 (kept from previous mapping)
            else if (obj.textureIndex == 5)
            {
                obj.shape.setFillColor(sf::Color::Magenta);
            }
            // Index 7: becomes dynamic (falling / physics-enabled)
            else if (obj.textureIndex == 7)
            {
                obj.body->SetType(b2_dynamicBody);
                // keep the color change optional
                obj.shape.setFillColor(sf::Color::Blue);
            }
            // Index 8: trigger zone that makes obstacle with textureIndex 6 fall
            else if (obj.textureIndex == 8)
            {
                Obstacle* fallingObj = getObstacleByTexture(6);

                if (fallingObj)
                {
                    b2Fixture* f = fallingObj->body->GetFixtureList();
                    if (f)
                    {
                        b2Filter filter = f->GetFilterData();
                        filter.maskBits |= CATEGORY_GROUND;
                        f->SetFilterData(filter);
                    }
                    fallingObj->body->SetType(b2_dynamicBody);
                }
            }
            // Index 6: "shit" collision -> GAMEOVER behavior (color to Blue as marker)
            else if (obj.textureIndex == 6)
            {
                obj.shape.setFillColor(sf::Color::Blue);
                // Additional game-over logic should be handled by caller/game code
            }
            // Index 11: trigger zone that makes obstacle with textureIndex 10 fall
            else if (obj.textureIndex == 11)
            {
                Obstacle* fallingObj = getObstacleByTexture(10);

                if (fallingObj)
                {
                    b2Fixture* f = fallingObj->body->GetFixtureList();
                    if (f)
                    {
                        b2Filter filter = f->GetFilterData();
                        filter.maskBits |= CATEGORY_GROUND;
                        f->SetFilterData(filter);
                    }
                    fallingObj->body->SetType(b2_dynamicBody);
                }
            }
            // Index 10: "shit" collision -> GAMEOVER behavior (color to Blue)
            else if (obj.textureIndex == 10)
            {
                obj.shape.setFillColor(sf::Color::Blue);
            }
            // Default: leave texture as-is or apply other game-specific reactions
        }
        else
        {
            // restore texture and reset fill color for non-colliding obstacles
            if (obj.textureIndex >= 0 && obj.textureIndex < obstacleTextures.size())
            {
                sf::Texture& tex = obstacleTextures[obj.textureIndex];
                obj.shape.setTexture(&tex);
                sf::Vector2u texSize = tex.getSize();
                obj.shape.setTextureRect(sf::IntRect(0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)));
            }
            // reset color (optional — white works well)
            obj.shape.setFillColor(sf::Color::White);
        }
    }
}

int World::findObstacleByTextureSubstring(const std::string& substr) const
{
    for (size_t i = 0; i < obstacleTextureFiles.size(); ++i) {
        if (obstacleTextureFiles[i].find(substr) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

b2Vec2 World::getObstacleBodyPosition(int index) const
{
    if (index < 0 || index >= static_cast<int>(obstacles.size())) return b2Vec2(0.f, 0.f);
    b2Body* b = obstacles[index].body;
    if (!b) return b2Vec2(0.f, 0.f);
    return b->GetPosition();
}

int World::getLastCollidedObstacleIndex() const
{
    return lastCollidedObstacleIndex;
}

void World::draw(sf::RenderWindow& window)
{
    // Draw background layers
    drawParallaxBackground(window);

    // Draw obstacles behind player if needed
    for (auto& obj : obstacles)
        window.draw(obj.shape);

    // Player will be drawn in Game::render after this

    // Foreground layers will be drawn after the player
    // drawParallaxForeground(window); // <-- called in Game::render AFTER player
}

// ======================================================================
// PARALLAX INITIALIZATION (14 layers, back → front)
// ======================================================================
void World::initParallax()
{
    parallaxLayers.clear();
    parallaxLayers.resize(14);

    // -----------------------------
    // CENTRALIZED PARALLAX CONFIG
    // -----------------------------
    struct LayerConfig {
        float speedX;
        float speedY;
        float baseYOffset;
        float scale;
    };

    // BACK → FRONT (14 layers)
    const LayerConfig config[14] =
    {
        //speedx/y      y axis pos, scale
        {0.0f, 0.00f, 0.0f, 1.00f},  // layer 1 (very far)
        {0.0f, 0.0f, +0.0f, 1.00f},  // layer 2
        {-0.09f, 0.0f, +0.0f, 1.00f},  // layer 3

        {-0.09f, 0.0f, +0.0f, 1.00f},  // layer 4
        {-0.09f, 0.0f, +0.0f, 1.00f},  // layer 5
        {-0.08f, 0.0f,   0.0f, 1.00f},  // layer 6

        {-0.07f, 0.0f, -0.0f, 1.00f},  // layer 7
        {-0.06f, 0.0f, -0.0f, 1.00f},  // layer 8

        {-0.05f, 0.0f,   0.f, 1.00f},  // layer 9 (GROUND layer)

        {0.0f, 0.0f, -0.0f, 1.00f},  // layer 10 (foreground)
        {0.0f, 0.0f, -0.0f, 1.00f},  // layer 11
        {0.0f, 0.0f, -0.0f, 1.00f},  // layer 12 (closest)
        {0.0f, 0.0f, 300.0f, 1.0f},  // layer 13 (closest)

        {0.0f, 0.0f, 300.0f, 1.0f},  // layer 14 (All Props)
    };

    for (int i = 0; i < 14; i++)
    {
        std::string path = "Assets/Parallax/" + std::to_string(i + 1) + ".png";

        if (!parallaxLayers[i].texture.loadFromFile(path))
            std::cerr << "FAILED TO LOAD PARALLAX: " << path << "\n";

        parallaxLayers[i].texture.setRepeated(true);

        parallaxLayers[i].sprite.setTexture(parallaxLayers[i].texture);
        parallaxLayers[i].sprite.setTextureRect(sf::IntRect(0, 0, 1920, 1080));

        // Apply user-friendly controls
        parallaxLayers[i].speedX = config[i].speedX;
        parallaxLayers[i].speedY = config[i].speedY;
        parallaxLayers[i].baseYOffset = config[i].baseYOffset;
        parallaxLayers[i].scale = config[i].scale;

        parallaxLayers[i].sprite.setScale(config[i].scale, config[i].scale);
    }
}

// ======================================================================
// UPDATE PARALLAX (CAMERA-DRIVEN)
// ======================================================================
static sf::Clock mCloudClock; // For independent cloud movement (file-scope static)
static float cloudSpeed = -30.f; // pixels per second

void World::updateParallax(const sf::Vector2f& camPos)
{
    float dtc = mCloudClock.restart().asSeconds(); // time since last frame

    for (size_t i = 0; i < parallaxLayers.size(); ++i)
    {
        auto& layer = parallaxLayers[i];

        float px, py;

        // cloud layers at indices 1 and 12 (per the second version)
        if (i == 1 || i == 12)
        {
            // independent drift
            if (i == 1)
                layer.baseXOffset += cloudSpeed * dtc;
            else
                layer.baseXOffset += (cloudSpeed + 15.f) * dtc;

            // wrap offset to avoid floating point overflow
            float texW = layer.texture.getSize().x * layer.scale;
            if (layer.baseXOffset <= -texW) layer.baseXOffset += texW;
            else if (layer.baseXOffset >= texW) layer.baseXOffset -= texW;

            px = layer.baseXOffset;
            py = -camPos.y * layer.speedY + parallaxYOffset + layer.baseYOffset;
        }
        else
        {
            px = -camPos.x * layer.speedX + layer.baseXOffset;
            py = -camPos.y * layer.speedY + parallaxYOffset + layer.baseYOffset;
        }

        layer.sprite.setPosition(px, py);
    }

    // FIRST FRAME: fix alignment
    if (!parallaxAligned)
    {
        parallaxAligned = true;

        float currentY = parallaxLayers[0].sprite.getPosition().y;
        parallaxYOffset = -currentY;

        for (auto& l : parallaxLayers)
        {
            sf::Vector2f p = l.sprite.getPosition();
            l.sprite.setPosition(p.x, p.y + parallaxYOffset);
        }
    }
}

// ======================================================================
// DRAW PARALLAX (HORIZONTAL WRAP ONLY)
// ======================================================================

// Draw background layers (0 → 11)
void World::drawParallaxBackground(sf::RenderWindow& window)
{
    for (size_t i = 0; i < 12 && i < parallaxLayers.size(); ++i) // layers 0–11
        drawLayer(window, parallaxLayers[i]);
}

// Draw foreground layers (12 → end)
void World::drawParallaxForeground(sf::RenderWindow& window)
{
    for (size_t i = 12; i < parallaxLayers.size(); ++i)
        drawLayer(window, parallaxLayers[i]);
}

// Helper to draw a single layer (wraps horizontally)
void World::drawLayer(sf::RenderWindow& window, ParallaxLayer& layer)
{
    sf::View view = window.getView();
    float viewW = view.getSize().x;
    float viewLeft = view.getCenter().x - viewW * 0.5f;

    float texW = layer.texture.getSize().x * layer.scale;
    if (texW <= 0.f) return; // safety

    float baseX = std::fmod(layer.sprite.getPosition().x, texW);
    if (baseX > 0) baseX -= texW;
    float y = layer.sprite.getPosition().y;

    int firstTile = static_cast<int>(std::floor((viewLeft - baseX) / texW)) - 1;
    int needed = static_cast<int>(std::ceil(viewW / texW)) + 3;

    for (int i = 0; i < needed; ++i)
    {
        sf::Sprite copy = layer.sprite;
        copy.setPosition(baseX + texW * (firstTile + i), y);
        window.draw(copy);
    }
}
