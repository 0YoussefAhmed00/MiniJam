#include "Game.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace sf;

constexpr float PPM = 30.f;
constexpr float INV_PPM = 1.f / PPM;

// random helpers (same as your originals)
static float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
static float randomOffset(float magnitude) {
    return ((rand() % 2000) / 1000.f - 1.f) * magnitude;
}

// --- MyContactListener implementation ---
void Game::MyContactListener::BeginContact(b2Contact* contact) {
    b2Fixture* a = contact->GetFixtureA();
    b2Fixture* b = contact->GetFixtureB();
    if (a == footFixture && !b->IsSensor()) footContacts++;
    if (b == footFixture && !a->IsSensor()) footContacts++;
}
void Game::MyContactListener::EndContact(b2Contact* contact) {
    b2Fixture* a = contact->GetFixtureA();
    b2Fixture* b = contact->GetFixtureB();
    if (a == footFixture && !b->IsSensor()) footContacts--;
    if (b == footFixture && !a->IsSensor()) footContacts--;
    if (footContacts < 0) footContacts = 0;
}

// --- Game methods ---
Game::Game()
    : m_window(VideoMode(1280, 720), "SFML + Box2D + AudioManager + Persona Demo"),
    m_camera(FloatRect(0, 0, 1280, 720)),
    m_defaultView(m_window.getDefaultView()),
    m_gravity(0.f, 9.8f),
    m_world(m_gravity),
    m_player(nullptr),
    m_diagMark(8.f),
    m_fxMark(8.f),
    psychoMode(false),
    nextPsychoSwitch(0.f),
    splitMode(false),
    splitDuration(0.f),
    nextSplitCheck(0.f),
    inTransition(false),
    pendingEnable(false),
    inputLocked(false),
    inputLockPending(false),
    nextInputLockCheck(0.f),
    m_running(true)
{
    srand(static_cast<unsigned>(time(nullptr)));
    m_window.setFramerateLimit(60);

    // Set contact listener
    m_world.SetContactListener(&m_contactListener);

    // Ground
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640 * INV_PPM, 680 * INV_PPM);
    b2Body* ground = m_world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(2000.0f * INV_PPM, 10.0f * INV_PPM);
    ground->CreateFixture(&groundBox, 0.f);

    m_groundShape = RectangleShape(Vector2f(4000.f, 40.f));
    m_groundShape.setOrigin(2000.f, 20.f);
    m_groundShape.setPosition(640.f, 680.f);
    m_groundShape.setFillColor(Color(80, 160, 80));

    // Create Player
    m_player = std::make_unique<Player>(&m_world, 640.f, 200.f);
    // Link foot fixture to contact listener
    m_contactListener.footFixture = m_player->GetFootFixture();

    // Audio setup
    m_diagMark.setFillColor(Color::Yellow);
    m_diagMark.setOrigin(8.f, 8.f);
    m_fxMark.setFillColor(Color::Cyan);
    m_fxMark.setOrigin(8.f, 8.f);

    m_dialogueEmitter = std::make_shared<AudioEmitter>();
    m_dialogueEmitter->id = "dialogue1";
    m_dialogueEmitter->category = AudioCategory::Dialogue;
    m_dialogueEmitter->position = b2Vec2((640 - 100) * INV_PPM, (680 - 50) * INV_PPM);
    m_dialogueEmitter->minDistance = 0.5f;
    m_dialogueEmitter->maxDistance = 20.f;
    m_dialogueEmitter->baseVolume = 1.f;

    m_effectEmitter = std::make_shared<AudioEmitter>();
    m_effectEmitter->id = "effect1";
    m_effectEmitter->category = AudioCategory::Effects;
    m_effectEmitter->position = b2Vec2((640 + 100) * INV_PPM, (680 - 50) * INV_PPM);
    m_effectEmitter->minDistance = 0.5f;
    m_effectEmitter->maxDistance = 20.f;
    m_effectEmitter->baseVolume = 1.f;

    m_audio.SetCrossfadeTime(1.2f);

    bool ok = m_audio.loadMusic("assets/Audio/music_neutral.ogg", "assets/Audio/music_crazy.ogg");
    if (!ok) std::cerr << "Warning: music not loaded. Replace file paths with your assets.\n";

    if (!m_dialogueEmitter->loadBuffer("assets/Audio/dialogue.wav"))
        std::cerr << "Warning: dialogue.wav not loaded." << std::endl;
    if (!m_effectEmitter->loadBuffer("assets/Audio/effect.wav"))
        std::cerr << "Warning: effect.wav not loaded." << std::endl;

    m_audio.RegisterEmitter(m_dialogueEmitter);
    m_audio.RegisterEmitter(m_effectEmitter);

    if (m_dialogueEmitter->buffer) {
        m_dialogueEmitter->sound.setLoop(true);
        m_dialogueEmitter->play();
    }
    if (m_effectEmitter->buffer) {
        m_effectEmitter->sound.setLoop(true);
        m_effectEmitter->play();
    }

    m_audio.SetMasterVolume(1.f);
    m_audio.SetMusicVolume(0.9f);
    m_audio.SetBackgroundVolume(0.6f);
    m_audio.SetDialogueVolume(1.f);
    m_audio.SetEffectsVolume(1.f);

    // Persona timers
    nextPsychoSwitch = randomFloat(6.f, 8.f);
    psychoClock.restart();

    nextSplitCheck = randomFloat(1.f, 3.f);
    splitClock.restart();

    nextInputLockCheck = randomFloat(3.f, 6.f);
    inputLockClock.restart();

    // Fonts / debug text
    if (!m_font.loadFromFile("assets/Font/Cairo-VariableFont_slnt,wght.ttf")) {
        std::cerr << "Warning: font not loaded (assets/arial.ttf)\n";
    }
    m_debugText.setFont(m_font);
    m_debugText.setCharacterSize(16);
    m_debugText.setFillColor(Color::White);
    m_debugText.setPosition(10.f, 10.f);

    m_frameClock.restart();
}

Game::~Game()
{
    // nothing special; Box2D world will be destroyed automatically
}

int Game::Run()
{
    while (m_window.isOpen() && m_running) {
        float dt = m_frameClock.restart().asSeconds();

        processEvents();

        // Step physics
        m_world.Step(1.f / 60.f, 8, 3);

        update(dt);
        render();
    }
    return 0;
}

void Game::processEvents()
{
    Event ev;
    while (m_window.pollEvent(ev)) {
        if (ev.type == Event::Closed || (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape))
            m_window.close();

        // Quick manual toggles for testing
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::M) {
            static bool mToggle = false; mToggle = !mToggle; m_audio.SetMusicVolume(mToggle ? 0.0f : 1.f);
        }
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::B) {
            static bool bToggle = false; bToggle = !bToggle; m_audio.SetBackgroundVolume(bToggle ? 0.1f : 1.f);
        }
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::P) {
            // manual psycho toggle
            psychoMode = !psychoMode;
            m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
            m_audio.SetMusicState(psychoMode ? GameState::Crazy : GameState::Neutral);

            // reset split/input locks on manual switch
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
            if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
        }
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Num2) {
            if (m_effectEmitter->buffer) m_effectEmitter->sound.play();
        }
    }
}

void Game::update(float dt)
{
    // Grounded check
    bool isGrounded = (m_contactListener.footContacts > 0);

    // Persona timed switching (random)
    if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
        psychoMode = !psychoMode;
        m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
        nextPsychoSwitch = randomFloat(6.f, 12.f);
        psychoClock.restart();

        // inform audio manager to change music
        m_audio.SetMusicState(psychoMode ? GameState::Crazy : GameState::Neutral);

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

    // Input-lock & split mode logic (only active in psycho mode)
    if (psychoMode) {
        // ---- input-lock logic ----
        float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

        if (inputLockPending) {
            if (isGrounded) {
                inputLockPending = false;
                inputLocked = true;
                inputLockClock.restart();
                m_player->SetColor(Color::Cyan); // indicate lock visually
            }
        }
        else if (!inputLocked) {
            if (elapsedLock >= nextInputLockCheck) {
                if (rand() % 2 == 0) {
                    if (isGrounded) {
                        inputLocked = true;
                        inputLockClock.restart();
                        m_player->SetColor(Color::Cyan);
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
                m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
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
                    m_camera.setRotation(180.f); // flip view
                    splitClock.restart();
                }
                else {
                    splitMode = false;
                    m_camera.setRotation(0.f);
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
        m_camera.setRotation(0.f);

        inputLocked = false;
        inputLockPending = false;
        inputLockClock.restart();
        nextInputLockCheck = randomFloat(3.f, 6.f);
    }

    // Player input & movement (affected by psycho/inversion/input-lock)
    b2Vec2 vel = m_player->GetLinearVelocity();
    float moveSpeed = 5.f;

    bool leftKey = Keyboard::isKeyPressed(Keyboard::A);
    bool rightKey = Keyboard::isKeyPressed(Keyboard::D);
    bool jumpKeyW = Keyboard::isKeyPressed(Keyboard::W);
    bool jumpKeyS = Keyboard::isKeyPressed(Keyboard::S);
    bool shiftKey = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift);

    moveSpeed = (shiftKey ? 5.f : 10.f);

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

    if (leftKey) vel.x = -moveSpeed;
    else if (rightKey) vel.x = moveSpeed;
    else vel.x = 0;

    bool isGroundedNow = (m_contactListener.footContacts > 0);
    bool jumpKey = jumpKeyW || jumpKeyS;
    if (jumpKey && isGroundedNow) {
        vel.y = -10.f;
    }

    m_player->SetLinearVelocity(vel);
    m_player->Update(dt, isGroundedNow);

    // Audio update (listener = player)
    b2Vec2 playerPos = m_player->GetBody()->GetPosition();
    m_audio.Update(dt, playerPos);
}

void Game::render()
{
    // Sync visuals
    // player draws itself
    m_player->SyncGraphics();

    // update emitter marks
    m_diagMark.setPosition(m_dialogueEmitter->position.x * PPM, m_dialogueEmitter->position.y * PPM);
    m_fxMark.setPosition(m_effectEmitter->position.x * PPM, m_effectEmitter->position.y * PPM);

    // camera center & shake
    b2Vec2 pos = m_player->GetBody()->GetPosition();
    Vector2f playerPosPixels(pos.x * PPM, pos.y * PPM);
    Vector2f camCenter = playerPosPixels;
    if (inTransition) {
        camCenter.x += randomOffset(TRANSITION_SHAKE_MAG);
        camCenter.y += randomOffset(TRANSITION_SHAKE_MAG);
    }
    else if (psychoMode) {
        camCenter.x += randomOffset(PSYCHO_SHAKE_MAG);
        camCenter.y += randomOffset(PSYCHO_SHAKE_MAG);
    }
    m_camera.setCenter(camCenter);
    m_window.setView(m_camera);

    m_window.clear(Color::Black);
    m_window.draw(m_groundShape);
    m_player->Draw(m_window);
    m_window.draw(m_diagMark);
    m_window.draw(m_fxMark);

    // HUD
    m_window.setView(m_defaultView);
    std::string s = std::string("PsychoMode: ") + (psychoMode ? "ON" : "OFF") + "\n" +
        "InputLock: " + (inputLocked ? std::string("LOCKED") : std::string("FREE")) + "\n" +
        "SplitMode: " + (splitMode ? std::string("ON") : std::string("OFF")) + "\n" +
        "Controls: A/D move, W jump (inverted when psycho)\n" +
        "P: force toggle psycho | M: toggle music vol | B: toggle bg vol\n" +
        "1: play dialogue one-shot | 2: play effect one-shot\n";
    m_debugText.setString(s);
    m_window.draw(m_debugText);

    m_window.display();
}
