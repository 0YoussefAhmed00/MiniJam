#include "World.h"
#include<iostream>
constexpr float PPM = 30.f;      // Pixels per meter
constexpr float INV_PPM = 1.f / PPM;

World::World(b2World& worldRef)
    : physicsWorld(worldRef)       // Gravity downward
{
    initParallax();

    // Create ALL obstacles here
    createObstacle(440, 588, true, 170, 170, "Assets/Obstacles/Untitled-2.png");   // Dynamic obstacle 
    createObstacle(700, 470, true, 300, 400, "Assets/Obstacles/foull car.png"); // Static (ground-only) obstacle
    createObstacle(1800, 646, true, 200, 40, "Assets/Obstacles/Closed_sewers_cap.png");


    createObstacle(2890, 630, false, 100, 30, "Assets/Obstacles/sewers_cap1.png");
    createObstacle(2900, 620, false, 300, 140, "Assets/Obstacles/sewers.png");
    createObstacle(3800, 600, false, 250, 170, "Assets/Obstacles/grocery.png");

    createObstacle(4800, 0, false, 150, 100, "Assets/Obstacles/1.png");
    createObstacle(4700, 600, false, 600, 100, "Assets/Obstacles/Untitled-2.png");

    createObstacle(6000, 640, false, 170, 80, "Assets/Obstacles/doggie.png");


    createObstacle(7200, -100, true, 170, 80, "Assets/Obstacles/man falling.png");
    createObstacle(7200, 600, false, 400, 100, "Assets/Obstacles/Untitled-2.png");
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
    bodyDef.type = b2_staticBody;

    b2Body* body = physicsWorld.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(scaleX / 2 * INV_PPM, scaleY / 2 * INV_PPM); // Box2D half-size

    b2FixtureDef fixture;
    fixture.shape = &box;
    fixture.density = onlyGround ? 0.f : 2.f;
    fixture.filter.categoryBits = CATEGORY_OBSTACLE;
    fixture.filter.maskBits = onlyGround ? (CATEGORY_GROUND | CATEGORY_PLAYER) : CATEGORY_GROUND;
    fixture.friction = 0.0f;
    body->CreateFixture(&fixture);

    // -------- SFML SHAPE --------
    sf::RectangleShape shape(sf::Vector2f(scaleX, scaleY));
    shape.setOrigin(scaleX / 2, scaleY / 2);
    shape.setPosition(x, y);
    shape.setTexture(&obstacleTextures.back());
    shape.setTextureRect(sf::IntRect(0, 0, texSize.x, texSize.y));

    // Store obstacle with texture index
    obstacles.emplace_back(body, shape, onlyGround, obstacleTextures.size() -1);

    // Update level extents (in pixels)
    float left = x - scaleX *0.5f;
    float right = x + scaleX *0.5f;
    if (left < levelMinX) levelMinX = left;
    if (right > levelMaxX) levelMaxX = right;
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

    for (auto& obj : obstacles)
    {
        sf::FloatRect obsBounds = obj.shape.getGlobalBounds();

        if (playerBounds.intersects(obsBounds))
        {
            mIsColliding = true;


            if (obj.textureIndex == 1)
                obj.shape.setFillColor(sf::Color::Yellow);
            else if (obj.textureIndex == 3)
                obj.shape.setFillColor(sf::Color::Yellow);
            else if (obj.textureIndex == 5)
                obj.shape.setFillColor(sf::Color::Yellow);
            else if (obj.textureIndex == 7)
                obj.shape.setFillColor(sf::Color::Blue);
            else if (obj.textureIndex == 8)
                obj.shape.setFillColor(sf::Color::Blue);
            else if (obj.textureIndex == 10)
                obj.shape.setFillColor(sf::Color::Blue);


        }
        else
        {
            // obj.shape.setFillColor(obj.onlyGround ? sf::Color::Red : sf::Color::White);

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
    drawParallax(window);

    // Draw ground layer (parallax index8) stretched across level
    drawGround(window);

    for (auto& obj : obstacles)
        window.draw(obj.shape);
}
// ======================================================================
// PARALLAX INITIALIZATION (12 layers, back → front)
// ======================================================================
void World::initParallax()
{
    parallaxLayers.clear();
    parallaxLayers.resize(12);

    // -----------------------------
    // CENTRALIZED PARALLAX CONFIG
    // -----------------------------
    struct LayerConfig {
        float speedX;
        float speedY;
        float baseYOffset;
        float scale;
    };

    // BACK → FRONT
    const LayerConfig config[13] =
    {
        {0.01f, 0.005f, +120.f, 1.00f},  // layer 1 (very far)
        {0.02f, 0.010f, +100.f, 1.00f},  // layer 2
        {0.03f, 0.015f, +80.f, 1.00f},  // layer 3

        {0.04f, 0.020f, +90.f, 1.00f},  // layer 4
        {0.05f, 0.025f, +90.f, 1.00f},  // layer 5
        {0.06f, 0.030f,   90.f, 1.00f},  // layer 6

        {0.08f, 0.050f, -10.f, 1.00f},  // layer 7
        {0.12f, 0.070f, -20.f, 1.00f},  // layer 8

        {0.18f, 0.110f,   0.f, 1.00f},  // layer 9 (GROUND layer)

        {0.18f, 0.160f, -20.f, 1.10f},  // layer 10 (foreground)
        {0.40f, 0.240f, -40.f, 1.15f},  // layer 11
        {0.55f, 0.350f, -60.f, 1.20f},  // layer 12 (closest)
        {0.55f, 0.350f, -60.f, 1.20f},  // layer 13 (closest)
    };

    for (int i = 0; i < 12; i++)
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
void World::updateParallax(const sf::Vector2f& camPos)
{
    for (auto& layer : parallaxLayers)
    {
        float px = -camPos.x * layer.speedX + layer.baseXOffset;
        float py = -camPos.y * layer.speedY + parallaxYOffset + layer.baseYOffset;

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
void World::drawParallax(sf::RenderWindow& window)
{
 const float texW =1920.f;

 // Get view info so we tile just enough to cover the visible area (in pixels)
 sf::View view = window.getView();
 float viewW = view.getSize().x;
 float viewLeft = view.getCenter().x - viewW *0.5f;

 for (auto& layer : parallaxLayers)
 {
 float baseX = std::fmod(layer.sprite.getPosition().x, texW);
 if (baseX >0) baseX -= texW;

 float y = layer.sprite.getPosition().y;

 // figure which tile index covers viewLeft
 int firstTile = static_cast<int>(std::floor((viewLeft - baseX) / texW)) -1;
 int needed = static_cast<int>(std::ceil(viewW / texW)) +3; // extra padding

 for (int i =0; i < needed; ++i)
 {
 sf::Sprite copy = layer.sprite;
 copy.setPosition(baseX + texW * (firstTile + i), y);
 window.draw(copy);
 }
 }
}

// Draw ground from parallax layer #9 (index8)
void World::drawGround(sf::RenderWindow& window)
{
 const int groundIndex =8; //0-based index for layer9
 if (groundIndex <0 || groundIndex >= static_cast<int>(parallaxLayers.size())) return;

 ParallaxLayer& layer = parallaxLayers[groundIndex];
 const float texW =1920.f;

 // Use view to tile ground infinitely across the visible area (and a bit beyond)
 sf::View view = window.getView();
 float viewW = view.getSize().x;
 float viewLeft = view.getCenter().x - viewW *0.5f;
 float viewRight = viewLeft + viewW;

 float baseX = std::fmod(layer.sprite.getPosition().x, texW);
 if (baseX >0) baseX -= texW;
 float y = layer.sprite.getPosition().y;

 int firstTile = static_cast<int>(std::floor((viewLeft - baseX) / texW)) -1;
 int lastTile = static_cast<int>(std::ceil((viewRight - baseX) / texW)) +1;

 for (int i = firstTile; i <= lastTile; ++i) {
 sf::Sprite copy = layer.sprite;
 copy.setPosition(baseX + texW * i, y);
 window.draw(copy);
 }
}