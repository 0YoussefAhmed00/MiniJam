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

// Generate small random offset for shake (-magnitude .. +magnitude)
float randomOffset(float magnitude) {
    return ((rand() % 2000) / 1000.f - 1.f) * magnitude;
}

// Contact listener to detect foot sensor contacts
class MyContactListener : public b2ContactListener {
public:
    MyContactListener() : footContacts(0), footFixture(nullptr) {}
    int footContacts;
    b2Fixture* footFixture;

    void BeginContact(b2Contact* contact) override {
        b2Fixture* a = contact->GetFixtureA();
        b2Fixture* b = contact->GetFixtureB();
        if (a == footFixture && !b->IsSensor()) footContacts++;
        if (b == footFixture && !a->IsSensor()) footContacts++;
    }
    void EndContact(b2Contact* contact) override {
        b2Fixture* a = contact->GetFixtureA();
        b2Fixture* b = contact->GetFixtureB();
        if (a == footFixture && !b->IsSensor()) footContacts--;
        if (b == footFixture && !a->IsSensor()) footContacts--;
        if (footContacts < 0) footContacts = 0;
    }
};

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow window(VideoMode(1280, 720), "SFML + Box2D Template");
    window.setFramerateLimit(60);

    View camera(FloatRect(0, 0, 1280, 720));

    // Box2D World
    b2Vec2 gravity(0.f, 9.8f);
    b2World world(gravity);

    MyContactListener contactListener;
    world.SetContactListener(&contactListener);

    //////////////////////////////////////////////////////
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

    // Player body
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(640 * INV_PPM, 200 * INV_PPM);
    boxDef.fixedRotation = true;
    b2Body* box = world.CreateBody(&boxDef);

    // Main body
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(25 * INV_PPM, 25 * INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.3f;
    box->CreateFixture(&boxFixture);

    // Foot sensor
    b2PolygonShape footShape;
    footShape.SetAsBox((22.f * INV_PPM), (5.f * INV_PPM), b2Vec2(0.f, 25.f * INV_PPM), 0.f);
    b2FixtureDef footFixtureDef;
    footFixtureDef.shape = &footShape;
    footFixtureDef.isSensor = true;
    b2Fixture* footSensor = box->CreateFixture(&footFixtureDef);

    contactListener.footFixture = footSensor;

    RectangleShape boxShape(Vector2f(50.f, 50.f));
    boxShape.setOrigin(25.f, 25.f);
    boxShape.setFillColor(Color::Red);

    // Psycho mode
    bool psychoMode = false;
    float nextPsychoSwitch = randomFloat(6.f, 8.f);
    Clock psychoClock;

    // Split mode
    bool splitMode = false;
    float splitDuration = 0.f;
    float nextSplitCheck = randomFloat(1.f, 3.f);
    Clock splitClock;
    bool inTransition = false;
    bool pendingEnable = false;
    Clock transitionClock;
    const float TRANSITION_TIME = 0.25f;
    const float PSYCHO_SHAKE_MAG = 4.f;
    const float TRANSITION_SHAKE_MAG = 20.f;

    // Input-lock logic
    bool inputLocked = false;
    bool inputLockPending = false;
    Clock inputLockClock;
    const float INPUT_LOCK_DURATION = 2.f;
    float nextInputLockCheck = randomFloat(3.f, 6.f);

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

        bool isGrounded = (contactListener.footContacts > 0);

        // Input-lock logic
        if (psychoMode) {
            float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

            if (inputLockPending) {
                if (isGrounded) {
                    inputLockPending = false;
                    inputLocked = true;
                    inputLockClock.restart();
                    boxShape.setFillColor(Color::Cyan);
                }
            }
            else if (!inputLocked) {
                if (elapsedLock >= nextInputLockCheck) {
                    if (rand() % 2 == 0) {
                        if (isGrounded) {
                            inputLocked = true;
                            inputLockClock.restart();
                            boxShape.setFillColor(Color::Cyan);
                        }
                        else {
                            inputLockPending = true;
                        }
                    }
                    else {
                        inputLockClock.restart();
                        nextInputLockCheck = randomFloat(3.f, 6.f);
                    }
                }
            }
            else {
                if (inputLockClock.getElapsedTime().asSeconds() >= INPUT_LOCK_DURATION) {
                    inputLocked = false;
                    inputLockClock.restart();
                    nextInputLockCheck = randomFloat(3.f, 6.f);
                    boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
                }
            }
        }
        else {
            inputLocked = false;
            inputLockPending = false;
            inputLockClock.restart();
            nextInputLockCheck = randomFloat(3.f, 6.f);
        }

        // Input
        b2Vec2 vel = box->GetLinearVelocity();
        float moveSpeed = 5.f;

        bool leftKey = Keyboard::isKeyPressed(Keyboard::A);
        bool rightKey = Keyboard::isKeyPressed(Keyboard::D);
        bool jumpKeyW = Keyboard::isKeyPressed(Keyboard::W);
        bool jumpKeyS = Keyboard::isKeyPressed(Keyboard::S);
        bool shiftKey = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift);

        moveSpeed = (shiftKey ? 2.5f : 5.f);

        if (inputLocked) {
            leftKey = rightKey = false;
            jumpKeyW = jumpKeyS = false;
        }
        else {
            if (psychoMode) {
                std::swap(leftKey, rightKey);

                // In psycho mode: W does nothing, S jumps
                jumpKeyW = false;
            }
            else {
                // In normal mode: S does nothing, W jumps
                jumpKeyS = false;
            }
        }

        // Movement
        if (leftKey) vel.x = -moveSpeed;
        else if (rightKey) vel.x = moveSpeed;
        else vel.x = 0;

        // Jump mapping: only the allowed key per mode will remain true
        bool jumpKey = jumpKeyW || jumpKeyS;

        if (jumpKey && isGrounded) {
            vel.y = -10.f;
        }

        box->SetLinearVelocity(vel);

        // Psycho toggling
        if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
            psychoMode = !psychoMode;
            boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
            nextPsychoSwitch = randomFloat(6.f, 12.f);
            psychoClock.restart();

            splitMode = false;
            inTransition = false;
            pendingEnable = false;
            camera.setRotation(0.f);
            splitClock.restart();
            nextSplitCheck = randomFloat(1.f, 3.f);

            inputLocked = false;
            inputLockPending = false;
            inputLockClock.restart();
            nextInputLockCheck = randomFloat(3.f, 6.f);
        }

        // Split mode logic
        if (psychoMode) {
            float elapsed = splitClock.getElapsedTime().asSeconds();

            if (!splitMode && !inTransition) {
                if (elapsed >= nextSplitCheck) {
                    if (rand() % 2 == 0) {
                        inTransition = true;
                        pendingEnable = true;
                        transitionClock.restart();
                    }
                    else {
                        splitClock.restart();
                        nextSplitCheck = randomFloat(1.f, 3.f);
                    }
                }
            }

            if (splitMode && !inTransition) {
                if (elapsed >= splitDuration) {
                    inTransition = true;
                    pendingEnable = false;
                    transitionClock.restart();
                }
            }

            if (inTransition) {
                float t = transitionClock.getElapsedTime().asSeconds();
                if (t >= TRANSITION_TIME) {
                    if (pendingEnable) {
                        splitMode = true;
                        splitDuration = randomFloat(2.f, 4.f);
                        camera.setRotation(180.f);
                        splitClock.restart();
                    }
                    else {
                        splitMode = false;
                        camera.setRotation(0.f);
                        splitClock.restart();
                        nextSplitCheck = randomFloat(1.f, 3.f);
                    }
                    inTransition = false;
                    pendingEnable = false;
                }
            }
        }
        else {
            splitMode = false;
            inTransition = false;
            pendingEnable = false;
            camera.setRotation(0.f);
        }

        // Render
        b2Vec2 pos = box->GetPosition();
        boxShape.setPosition(pos.x * PPM, pos.y * PPM);

        Vector2f camCenter = boxShape.getPosition();
        if (inTransition) {
            camCenter.x += randomOffset(TRANSITION_SHAKE_MAG);
            camCenter.y += randomOffset(TRANSITION_SHAKE_MAG);
        }
        else if (psychoMode) {
            camCenter.x += randomOffset(PSYCHO_SHAKE_MAG);
            camCenter.y += randomOffset(PSYCHO_SHAKE_MAG);
        }

        camera.setCenter(camCenter);
        window.setView(camera);

        window.clear(Color::Black);
        window.draw(groundShape);
        window.draw(boxShape);
        window.display();
    }

    return 0;
}