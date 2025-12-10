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
    obstacles.emplace_back(body, shape, onlyGround, obstacleTextures.size() - 1);
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

    // Correct order: slowest → fastest (BACK → FRONT)
    const float speedsX[12] = {
        0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f,
        0.08f, 0.12f, 0.18f, 0.26f, 0.40f, 0.55f
    };

    const float speedsY[12] = {
        0.005f, 0.01f, 0.015f, 0.02f, 0.025f, 0.03f,
        0.05f, 0.07f, 0.11f, 0.16f, 0.24f, 0.35f
    };

    for (int i = 0; i < 12; i++)
    {
        std::string path = "Assets/Parallax/" + std::to_string(i + 1) + ".png";

        if (!parallaxLayers[i].texture.loadFromFile(path))
            std::cerr << "FAILED TO LOAD PARALLAX: " << path << "\n";

        parallaxLayers[i].texture.setRepeated(true);
        parallaxLayers[i].sprite.setTexture(parallaxLayers[i].texture);

        // Prevent stretching and enforce exact 1920x1080 image size
        parallaxLayers[i].sprite.setTextureRect(sf::IntRect(0, 0, 1920, 1080));
        parallaxLayers[i].sprite.setScale(1.f, 1.f);

        parallaxLayers[i].speedX = speedsX[i];
        parallaxLayers[i].speedY = speedsY[i];
    }
}

// ======================================================================
// UPDATE PARALLAX (CAMERA-DRIVEN)
// ======================================================================
void World::updateParallax(const sf::Vector2f& camPos)
{
    for (auto& layer : parallaxLayers)
    {
        float px = -camPos.x * layer.speedX;
        float py = -camPos.y * layer.speedY + parallaxYOffset;

        layer.sprite.setPosition(px, py);
    }

    // ------------------------------------------------------------------
    // FIRST FRAME — FIX VERTICAL OFFSET
    // ------------------------------------------------------------------
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
    const float texW = 1920.f;

    for (auto& layer : parallaxLayers)
    {
        float baseX = std::fmod(layer.sprite.getPosition().x, texW);
        if (baseX > 0) baseX -= texW;

        float y = layer.sprite.getPosition().y;

        for (int i = 0; i < 3; i++)
        {
            sf::Sprite copy = layer.sprite;
            copy.setPosition(baseX + texW * i, y);
            window.draw(copy);
        }
    }
}