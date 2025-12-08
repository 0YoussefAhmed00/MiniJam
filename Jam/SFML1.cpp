// File: main.cpp
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <box2d/box2d.h>
#include <iostream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <cmath>

#include "AudioManager.h"

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

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RenderWindow window(VideoMode(1280, 720), "SFML + Box2D + AudioManager + Persona Demo");
    window.setFramerateLimit(60);

    // Camera / view
    View camera(FloatRect(0, 0, 1280, 720));
    View defaultView = window.getDefaultView();

    // Box2D world
    b2Vec2 gravity(0.f, 9.8f);
    b2World world(gravity);

    // Contact listener (for foot sensor)
    MyContactListener contactListener;
    world.SetContactListener(&contactListener);

    //////////////////////////////////////////////////////
    // Ground
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640 * INV_PPM, 680 * INV_PPM);
    b2Body* ground = world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(2000.0f * INV_PPM, 10.0f * INV_PPM);
    ground->CreateFixture(&groundBox, 0.f);

    RectangleShape groundShape(Vector2f(4000.f, 40.f));
    groundShape.setOrigin(2000.f, 20.f);
    groundShape.setPosition(640.f, 680.f);
    groundShape.setFillColor(Color(80, 160, 80));

    // Player body
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(640 * INV_PPM, 200 * INV_PPM);
    boxDef.fixedRotation = true;
    b2Body* box = world.CreateBody(&boxDef);

    // Main body fixture
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(25 * INV_PPM, 25 * INV_PPM);

    b2FixtureDef boxFixture;
    boxFixture.shape = &dynamicBox;
    boxFixture.density = 1.f;
    boxFixture.friction = 0.3f;
    box->CreateFixture(&boxFixture);

    // Foot sensor
    b2PolygonShape footShape;
    // width 44px -> 22, height 10px -> 5, offset down by 25 px
    footShape.SetAsBox((22.f * INV_PPM), (5.f * INV_PPM), b2Vec2(0.f, 25.f * INV_PPM), 0.f);
    b2FixtureDef footFixtureDef;
    footFixtureDef.shape = &footShape;
    footFixtureDef.isSensor = true;
    b2Fixture* footSensor = box->CreateFixture(&footFixtureDef);
    contactListener.footFixture = footSensor;

    RectangleShape boxShape(Vector2f(50.f, 50.f));
    boxShape.setOrigin(25.f, 25.f);
    boxShape.setFillColor(Color::Red);

    // ---------------------------
    // Audio emitters & manager
    // ---------------------------
    auto dialogueEmitter = std::make_shared<AudioEmitter>();
    dialogueEmitter->id = "dialogue1";
    dialogueEmitter->category = AudioCategory::Dialogue;
    dialogueEmitter->position = b2Vec2((640 - 100) * INV_PPM, (680 - 50) * INV_PPM);
    dialogueEmitter->minDistance = 0.5f;
    dialogueEmitter->maxDistance = 20.f;
    dialogueEmitter->baseVolume = 1.f;

    auto effectEmitter = std::make_shared<AudioEmitter>();
    effectEmitter->id = "effect1";
    effectEmitter->category = AudioCategory::Effects;
    effectEmitter->position = b2Vec2((640 + 100) * INV_PPM, (680 - 50) * INV_PPM);
    effectEmitter->minDistance = 0.5f;
    effectEmitter->maxDistance = 20.f;
    effectEmitter->baseVolume = 1.f;

    CircleShape diagMark(8.f);
    diagMark.setFillColor(Color::Yellow);
    diagMark.setOrigin(8.f, 8.f);

    CircleShape fxMark(8.f);
    fxMark.setFillColor(Color::Cyan);
    fxMark.setOrigin(8.f, 8.f);

    AudioManager audio;
    audio.SetCrossfadeTime(1.2f);

    bool ok = audio.loadMusic("assets/Audio/music_neutral.ogg", "assets/Audio/music_crazy.ogg");
    if (!ok) std::cerr << "Warning: music not loaded. Replace file paths with your assets.\n";

    if (!dialogueEmitter->loadBuffer("assets/Audio/dialogue.wav"))
        std::cerr << "Warning: dialogue.wav not loaded." << std::endl;
    if (!effectEmitter->loadBuffer("assets/Audio/effect.wav"))
        std::cerr << "Warning: effect.wav not loaded." << std::endl;

    audio.RegisterEmitter(dialogueEmitter);
    audio.RegisterEmitter(effectEmitter);

    if (dialogueEmitter->buffer) {
        dialogueEmitter->sound.setLoop(true);
        dialogueEmitter->play();
    }
    if (effectEmitter->buffer) {
        effectEmitter->sound.setLoop(true);
        effectEmitter->play();
    }

    audio.SetMasterVolume(1.f);
    audio.SetMusicVolume(0.9f);
    audio.SetBackgroundVolume(0.6f);
    audio.SetDialogueVolume(1.f);
    audio.SetEffectsVolume(1.f);

    // ---------------------------
    // Persona / psycho state variables
    // ---------------------------
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

    // ---------------------------
    // Debug text
    // ---------------------------
    Font font;
    if (!font.loadFromFile("assets/Font/Cairo-VariableFont_slnt,wght.ttf")) {
        std::cerr << "Warning: font not loaded (assets/arial.ttf)\n";
    }
    Text debugText;
    debugText.setFont(font);
    debugText.setCharacterSize(16);
    debugText.setFillColor(Color::White);
    debugText.setPosition(10.f, 10.f);

    // Frame loop
    Clock frameClock;
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();

        Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == Event::Closed || (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape))
                window.close();

            // Quick manual toggles for testing
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::M) {
                static bool mToggle = false; mToggle = !mToggle; audio.SetMusicVolume(mToggle ? 0.0f : 1.f);
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::B) {
                static bool bToggle = false; bToggle = !bToggle; audio.SetBackgroundVolume(bToggle ? 0.1f : 1.f);
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::P) {
                // manual psycho toggle
                psychoMode = !psychoMode;
                boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
                audio.SetMusicState(psychoMode ? GameState::Crazy : GameState::Neutral);

                // reset split/input locks on manual switch (mirrors your earlier logic)
                splitMode = false;
                inTransition = false;
                pendingEnable = false;
                splitClock.restart();
                nextSplitCheck = randomFloat(1.f, 3.f);

                inputLocked = false;
                inputLockPending = false;
                inputLockClock.restart();
                nextInputLockCheck = randomFloat(3.f, 6.f);
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Num1) {
                if (dialogueEmitter->buffer) dialogueEmitter->sound.play();
            }
            if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Num2) {
                if (effectEmitter->buffer) effectEmitter->sound.play();
            }
        }

        // Step physics
        world.Step(1.f / 60.f, 8, 3);

        // Grounded check
        bool isGrounded = (contactListener.footContacts > 0);

        // -------------------------
        // Persona timed switching (random)
        // -------------------------
        if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
            psychoMode = !psychoMode;
            boxShape.setFillColor(psychoMode ? Color::Magenta : Color::Red);
            nextPsychoSwitch = randomFloat(6.f, 12.f);
            psychoClock.restart();

            // inform audio manager to change music
            audio.SetMusicState(psychoMode ? GameState::Crazy : GameState::Neutral);

            // reset split/input-lock cycles
            splitMode = false;
            inTransition = false;
            pendingEnable = false;
            splitClock.restart();
            nextSplitCheck = randomFloat(1.f, 3.f);

            inputLocked = false;
            inputLockPending = false;
            inputLockClock.restart();
            nextInputLockCheck = randomFloat(3.f, 6.f);
        }

        // -------------------------
        // Input-lock & split mode logic (only active in psycho mode)
        // -------------------------
        if (psychoMode) {
            // ---- input-lock logic ----
            float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

            if (inputLockPending) {
                if (isGrounded) {
                    inputLockPending = false;
                    inputLocked = true;
                    inputLockClock.restart();
                    boxShape.setFillColor(Color::Cyan); // indicate lock visually
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

            // ---- split mode logic ----
            float elapsedSplit = splitClock.getElapsedTime().asSeconds();

            if (!splitMode && !inTransition) {
                if (elapsedSplit >= nextSplitCheck) {
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
                if (elapsedSplit >= splitDuration) {
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
                        camera.setRotation(180.f); // flip view
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
            // not psycho
            splitMode = false;
            inTransition = false;
            pendingEnable = false;
            camera.setRotation(0.f);

            inputLocked = false;
            inputLockPending = false;
            inputLockClock.restart();
            nextInputLockCheck = randomFloat(3.f, 6.f);
        }

        // -------------------------
        // Player input & movement (affected by psycho/inversion/input-lock)
        // -------------------------
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
                // Normal mode: S does nothing, W jumps
                jumpKeyS = false;
            }
        }

        // Movement
        if (leftKey) vel.x = -moveSpeed;
        else if (rightKey) vel.x = moveSpeed;
        else vel.x = 0;

        // Jump mapping
        bool jumpKey = jumpKeyW || jumpKeyS;
        if (jumpKey && isGrounded) {
            vel.y = -10.f;
        }

        box->SetLinearVelocity(vel);

        // -------------------------
        // Audio update (listener = player)
        // -------------------------
        b2Vec2 playerPos = box->GetPosition();
        audio.Update(dt, playerPos);

        // -------------------------
        // Rendering & camera shake
        // -------------------------
        b2Vec2 pos = box->GetPosition();
        boxShape.setPosition(pos.x * PPM, pos.y * PPM);

        diagMark.setPosition(dialogueEmitter->position.x * PPM, dialogueEmitter->position.y * PPM);
        fxMark.setPosition(effectEmitter->position.x * PPM, effectEmitter->position.y * PPM);

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
        window.draw(diagMark);
        window.draw(fxMark);

        // HUD
        window.setView(defaultView);
        std::string s = std::string("PsychoMode: ") + (psychoMode ? "ON" : "OFF") + "\n" +
            "InputLock: " + (inputLocked ? std::string("LOCKED") : std::string("FREE")) + "\n" +
            "SplitMode: " + (splitMode ? std::string("ON") : std::string("OFF")) + "\n" +
            "Controls: A/D move, W jump (inverted when psycho)\n" +
            "P: force toggle psycho | M: toggle music vol | B: toggle bg vol\n" +
            "1: play dialogue one-shot | 2: play effect one-shot\n";
        debugText.setString(s);
        window.draw(debugText);

        window.display();
    }

    return 0;
}
