#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace sf;

constexpr float PPM = 30.f;
constexpr float INV_PPM = 1.f / PPM;

// Function to generate random float between min and max
float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    // ------------------------------------------------------
    // Window
    // ------------------------------------------------------
    RenderWindow window(VideoMode(1280, 720), "SFML + Box2D Template");
    window.setFramerateLimit(60);

    // ------------------------------------------------------
    // Box2D World
    // ------------------------------------------------------
    b2Vec2 gravity(0.f, 9.8f);
    b2World world(gravity);

    // ------------------------------------------------------
    // Ground body
    // ------------------------------------------------------
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640 * INV_PPM, 680 * INV_PPM);

    b2Body* ground = world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(500 * INV_PPM, 20 * INV_PPM);
    ground->CreateFixture(&groundBox, 0.f);

    RectangleShape groundShape(Vector2f(1000.f, 40.f));
    groundShape.setOrigin(500.f, 20.f);
    groundShape.setPosition(640.f, 680.f);
    groundShape.setFillColor(Color::Green);

    // ------------------------------------------------------
    // Player Box (dynamic)
    // ------------------------------------------------------
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(640 * INV_PPM, 200 * INV_PPM);
    boxDef.fixedRotation = true; // prevent rotation
    b2Body* box = world.CreateBody(&boxDef);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(25 * INV_PPM, 25 * INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.3f;
    box->CreateFixture(&boxFixture);

    RectangleShape boxShape(Vector2f(50.f, 50.f));
    boxShape.setOrigin(25.f, 25.f);
    boxShape.setFillColor(Color::Red);

    // ------------------------------------------------------
    // Psycho mode variables
    // ------------------------------------------------------
    bool psychoMode = false;
    float nextSwitch = randomFloat(2.f, 5.f); // next switch time in seconds
    Clock psychoClock;

    // ------------------------------------------------------
    // Game Loop
    // ------------------------------------------------------
    while (window.isOpen())
    {
        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed ||
                (e.type == Event::KeyPressed && e.key.code == Keyboard::Escape))
                window.close();
        }

        // ----------------------------
        // Step Box2D
        // ----------------------------
        world.Step(1.f / 60.f, 8, 3);

        // ----------------------------
        // Player input
        // ----------------------------
        b2Vec2 vel = box->GetLinearVelocity();
        float moveSpeed = 5.f;

        bool left = Keyboard::isKeyPressed(Keyboard::A);
        bool right = Keyboard::isKeyPressed(Keyboard::D);
        bool jump = Keyboard::isKeyPressed(Keyboard::W);

        if (psychoMode) std::swap(left, right); // invert controls

        if (left) vel.x = -moveSpeed;
        else if (right) vel.x = moveSpeed;
        else vel.x = 0;

        if (jump && fabs(vel.y) < 0.01f) // jump only if nearly on ground
            vel.y = -10.f;

        box->SetLinearVelocity(vel);

        // ----------------------------
        // Psycho mode switch
        // ----------------------------
        if (psychoClock.getElapsedTime().asSeconds() >= nextSwitch) {
            psychoMode = !psychoMode;
            boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
            nextSwitch = randomFloat(5.f, 10.f); // next interval
            psychoClock.restart();
        }

        // ----------------------------
        // Update SFML box position
        // ----------------------------
        b2Vec2 pos = box->GetPosition();
        boxShape.setPosition(pos.x * PPM, pos.y * PPM);

        // ----------------------------
        // Draw
        // ----------------------------
        window.clear(Color::Black);
        window.draw(groundShape);
        window.draw(boxShape);
        window.display();
    }

    return 0;
}
