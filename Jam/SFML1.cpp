#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace sf;

constexpr float PPM = 30.f;
constexpr float INV_PPM = 1.f / PPM;

// Generate random float between min and max
float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

// Generate small random offset for shake
float randomOffset(float magnitude) {
    return ((rand() % 2000) / 1000.f - 1.f) * magnitude; // -magnitude..+magnitude
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow window(VideoMode(1280, 720), "SFML + Box2D Template");
    window.setFramerateLimit(60);

    View camera(FloatRect(0, 0, 1280, 720));

    // Box2D World
    b2Vec2 gravity(0.f, 9.8f);
    b2World world(gravity);

    // Ground
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

    // Player
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(640 * INV_PPM, 200 * INV_PPM);
    boxDef.fixedRotation = true;
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

    // Psycho mode
    bool psychoMode = false;
    float nextPsychoSwitch = randomFloat(6.f, 8.f); // longer duration
    Clock psychoClock;

    // Split mode camera rotation
    bool splitMode = false;
    float splitDuration = 0.f;
    float nextSplitCheck = randomFloat(1.f, 3.f);
    Clock splitClock;
    float shakeMagnitude = 8.f; // pixels

    while (window.isOpen())
    {
        Event e;
        while (window.pollEvent(e))
        {
            if (e.type == Event::Closed ||
                (e.type == Event::KeyPressed && e.key.code == Keyboard::Escape))
                window.close();
        }

        world.Step(1.f / 60.f, 8, 3);

        // Player input
        b2Vec2 vel = box->GetLinearVelocity();
        float moveSpeed = 5.f;

        bool left = Keyboard::isKeyPressed(Keyboard::A);
        bool right = Keyboard::isKeyPressed(Keyboard::D);
        bool jump = Keyboard::isKeyPressed(Keyboard::W);

        if (psychoMode) std::swap(left, right);

        if (left) vel.x = -moveSpeed;
        else if (right) vel.x = moveSpeed;
        else vel.x = 0;

        if (jump && fabs(vel.y) < 0.01f)
            vel.y = -10.f;

        box->SetLinearVelocity(vel);

        // Psycho mode switch
        if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
            psychoMode = !psychoMode;
            boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
            nextPsychoSwitch = randomFloat(6.f, 8.f); // next duration
            psychoClock.restart();
            // Reset split mode on psycho start
            splitMode = false;
            splitClock.restart();
            nextSplitCheck = randomFloat(1.f, 3.f);
        }

        // Split mode (only in psycho)
        if (psychoMode) {
            float t = splitClock.getElapsedTime().asSeconds();
            if (!splitMode && t >= nextSplitCheck) {
                // 50% chance to enable split mode
                if (rand() % 2 == 0) {
                    splitMode = true;
                    splitDuration = randomFloat(2.f, 4.f);
                    camera.setRotation(180.f);
                }
            }
            if (splitMode && t >= splitDuration + nextSplitCheck) {
                // End split mode
                splitMode = false;
                camera.setRotation(0.f);
                splitClock.restart();
                nextSplitCheck = randomFloat(1.f, 3.f);
            }
        }
        else {
            splitMode = false;
            camera.setRotation(0.f);
        }

        // Update box position
        b2Vec2 pos = box->GetPosition();
        boxShape.setPosition(pos.x * PPM, pos.y * PPM);

        // Camera follows player
        Vector2f camCenter = boxShape.getPosition();

        // Apply shake if split mode is active
        if (splitMode) {
            camCenter.x += randomOffset(shakeMagnitude);
            camCenter.y += randomOffset(shakeMagnitude);
        }

        camera.setCenter(camCenter);
        window.setView(camera);

        // Draw
        window.clear(Color::Black);
        window.draw(groundShape);
        window.draw(boxShape);
        window.display();
    }

    return 0;
}
