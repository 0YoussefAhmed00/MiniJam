#include "Level.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

static float s_randomFloat(float min, float max) {
    return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
static float s_randomOffset(float magnitude) {
    return ((std::rand() % 2000) / 1000.f - 1.f) * magnitude;
}

float Level::randomFloat(float min, float max) { return s_randomFloat(min, max); }
float Level::randomOffset(float magnitude) { return s_randomOffset(magnitude); }

Level::Level(Stage initialStage)
    : m_world(m_gravity),
    m_stage(initialStage),
    m_diagMark(8.f),
    m_fxMark(8.f)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

Level::~Level() {
    exit();
}

void Level::enter(sf::RenderWindow& window) {
    m_defaultView = window.getDefaultView();
    m_camera = sf::View(sf::FloatRect(0.f, 0.f, m_defaultView.getSize().x, m_defaultView.getSize().y));

    switch (m_stage) {
    case Stage::Level1: enterLevel1(window); break;
    case Stage::Level2: enterLevel2(window); break;
    }
    m_frameClock.restart();
}

void Level::processEvents(sf::RenderWindow& window) {
    switch (m_stage) {
    case Stage::Level1: processEventsLevel1(window); break;
    case Stage::Level2: processEventsLevel2(window); break;
    }
}

void Level::update(float dt) {
    switch (m_stage) {
    case Stage::Level1: updateLevel1(dt); break;
    case Stage::Level2: updateLevel2(dt); break;
    }
}

void Level::render(sf::RenderWindow& window) {
    switch (m_stage) {
    case Stage::Level1: renderLevel1(window); break;
    case Stage::Level2: renderLevel2(window); break;
    }
}

void Level::exit() {
    switch (m_stage) {
    case Stage::Level1: exitLevel1(); break;
    case Stage::Level2: exitLevel2(); break;
    }
}

// ---------------- Level1 implementation (migrated from Game.cpp) ----------------

void Level::enterLevel1(sf::RenderWindow& window) {
    m_world.SetContactListener(&m_contactListener);

    // Ground physics
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640.f * Units::INV_PPM, 680.f * Units::INV_PPM);
    b2Body* ground = m_world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(2000.0f * Units::INV_PPM, 10.0f * Units::INV_PPM);
    ground->CreateFixture(&groundBox, 0.f);

    m_groundShape = sf::RectangleShape(sf::Vector2f(4000.f, 40.f));
    m_groundShape.setOrigin(2000.f, 20.f);
    m_groundShape.setPosition(640.f, 680.f);
    m_groundShape.setFillColor(sf::Color(80, 160, 80));

    // Player
    m_player = std::make_unique<Player>(&m_world, 640.f, 200.f);
    m_contactListener.footFixture = m_player->GetFootFixture();

    // Audio and emitters
    m_diagMark.setFillColor(sf::Color::Yellow);
    m_diagMark.setOrigin(8.f, 8.f);
    m_fxMark.setFillColor(sf::Color::Cyan);
    m_fxMark.setOrigin(8.f, 8.f);

    m_dialogueEmitter = std::make_shared<AudioEmitter>();
    m_dialogueEmitter->id = "dialogue1";
    m_dialogueEmitter->category = AudioCategory::Dialogue;
    m_dialogueEmitter->position = b2Vec2((640.f - 100.f) * Units::INV_PPM, (680.f - 50.f) * Units::INV_PPM);
    m_dialogueEmitter->minDistance = 0.5f;
    m_dialogueEmitter->maxDistance = 20.f;
    m_dialogueEmitter->baseVolume = 1.f;

    m_effectEmitter = std::make_shared<AudioEmitter>();
    m_effectEmitter->id = "effect1";
    m_effectEmitter->category = AudioCategory::Effects;
    m_effectEmitter->position = b2Vec2((640.f + 100.f) * Units::INV_PPM, (680.f - 50.f) * Units::INV_PPM);
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

    m_dialogueEmitter->sound.setLoop(true);
    m_effectEmitter->sound.setLoop(true);

    m_audio.SetMasterVolume(1.f);
    m_audio.SetMusicVolume(0.9f);
    m_audio.SetBackgroundVolume(0.6f);
    m_audio.SetDialogueVolume(1.f);
    m_audio.SetEffectsVolume(1.f);

    nextPsychoSwitch = randomFloat(6.f, 8.f);
    psychoClock.restart();

    nextSplitCheck = randomFloat(1.f, 3.f);
    splitClock.restart();

    nextInputLockCheck = randomFloat(3.f, 6.f);
    inputLockClock.restart();

    if (!m_font.loadFromFile("assets/Font/Cairo-VariableFont_slnt,wght.ttf")) {
        std::cerr << "Warning: font not loaded (assets/Font/Cairo-VariableFont_slnt,wght.ttf)\n";
    }
    m_debugText.setFont(m_font);
    m_debugText.setCharacterSize(16);
    m_debugText.setFillColor(sf::Color::White);
    m_debugText.setPosition(10.f, 10.f);

    // Start music and loop emitters on entering gameplay
    m_audio.StartMusic();
    if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
    if (m_effectEmitter->buffer) m_effectEmitter->sound.play();

    // Views
    m_defaultView = window.getDefaultView();
    m_camera = sf::View(sf::FloatRect(0.f, 0.f, m_defaultView.getSize().x, m_defaultView.getSize().y));
}

void Level::processEventsLevel1(sf::RenderWindow& window) {
    sf::Event ev;
    while (window.pollEvent(ev)) {
        if (ev.type == sf::Event::Closed) {
            window.close();
        }

        // Escape: request return to menu
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape) {
            m_returnToMenu = true;
            m_complete = true;
        }

        // Audio toggles and psycho toggle
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::M) {
            static bool mToggle = false; mToggle = !mToggle;
            m_audio.SetMusicVolume(mToggle ? 0.0f : 1.f);
        }
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::B) {
            static bool bToggle = false; bToggle = !bToggle;
            m_audio.SetBackgroundVolume(bToggle ? 0.1f : 1.f);
        }
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::P) {
            psychoMode = !psychoMode;
            m_player->SetColor(psychoMode ? sf::Color::Magenta : sf::Color::Red);
            m_player->SetAudioState(psychoMode ? PlayerAudioState::Crazy : PlayerAudioState::Neutral);

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
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Num1) {
            if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
        }
        if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Num2) {
            if (m_effectEmitter->buffer) m_effectEmitter->sound.play();
        }
    }
}

void Level::updateLevel1(float dt) {
    // Physics step
    m_world.Step(1.f / 60.f, 8, 3);

    bool isGrounded = (m_contactListener.footContacts > 0);

    if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
        psychoMode = !psychoMode;
        m_player->SetColor(psychoMode ? sf::Color::Magenta : sf::Color::Red);
        m_player->SetAudioState(psychoMode ? PlayerAudioState::Crazy : PlayerAudioState::Neutral);

        nextPsychoSwitch = randomFloat(6.f, 12.f);
        psychoClock.restart();

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

    if (psychoMode) {
        float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

        if (inputLockPending) {
            if (isGrounded) {
                inputLockPending = false;
                inputLocked = true;
                inputLockClock.restart();
                m_player->SetColor(sf::Color::Cyan);
            }
        }
        else if (!inputLocked) {
            if (elapsedLock >= nextInputLockCheck) {
                if (std::rand() % 2 == 0) {
                    if (isGrounded) {
                        inputLocked = true;
                        inputLockClock.restart();
                        m_player->SetColor(sf::Color::Cyan);
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
                m_player->SetColor(psychoMode ? sf::Color::Magenta : sf::Color::Red);
            }
        }

        float elapsedSplit = splitClock.getElapsedTime().asSeconds();

        if (!splitMode && !inTransition) {
            if (elapsedSplit >= nextSplitCheck) {
                if (std::rand() % 2 == 0) {
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
                    m_camera.setRotation(180.f);
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
        splitMode = false;
        inTransition = false;
        pendingEnable = false;
        m_camera.setRotation(0.f);

        inputLocked = false;
        inputLockPending = false;
        inputLockClock.restart();
        nextInputLockCheck = randomFloat(3.f, 6.f);
    }

    // Input and player movement
    b2Vec2 vel = m_player->GetLinearVelocity();
    float moveSpeed = 5.f;

    bool leftKey = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
    bool rightKey = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
    bool jumpKeyW = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
    bool jumpKeyS = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
    bool shiftKey = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

    moveSpeed = (shiftKey ? 5.f : 10.f);

    if (inputLocked) {
        leftKey = rightKey = false;
        jumpKeyW = jumpKeyS = false;
    }
    else {
        if (psychoMode) {
            std::swap(leftKey, rightKey);
            jumpKeyW = false;
        }
        else {
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

    PlayerAudioState cur = m_player->GetAudioState();
    if (cur != m_lastAppliedAudioState) {
        if (cur == PlayerAudioState::Crazy) m_audio.CrossfadeToCrazy();
        else m_audio.CrossfadeToNeutral();
        m_lastAppliedAudioState = cur;
    }

    b2Vec2 playerPos = m_player->GetBody()->GetPosition();
    m_audio.Update(dt, playerPos);
}

void Level::renderLevel1(sf::RenderWindow& window) {
    m_player->SyncGraphics();

    m_diagMark.setPosition(m_dialogueEmitter->position.x * Units::PPM, m_dialogueEmitter->position.y * Units::PPM);
    m_fxMark.setPosition(m_effectEmitter->position.x * Units::PPM, m_effectEmitter->position.y * Units::PPM);

    b2Vec2 pos = m_player->GetBody()->GetPosition();
    sf::Vector2f playerPosPixels(pos.x * Units::PPM, pos.y * Units::PPM);
    sf::Vector2f camCenter = playerPosPixels;
    if (inTransition) {
        camCenter.x += randomOffset(TRANSITION_SHAKE_MAG);
        camCenter.y += randomOffset(TRANSITION_SHAKE_MAG);
    }
    else if (psychoMode) {
        camCenter.x += randomOffset(PSYCHO_SHAKE_MAG);
        camCenter.y += randomOffset(PSYCHO_SHAKE_MAG);
    }
    m_camera.setCenter(camCenter);
    window.setView(m_camera);

    window.clear(sf::Color::Black);
    window.draw(m_groundShape);
    m_player->Draw(window);
    window.draw(m_diagMark);
    window.draw(m_fxMark);

    window.setView(m_defaultView);
    std::string s = std::string("State: PLAYING\n") +
        "PsychoMode: " + (psychoMode ? "ON" : "OFF") + "\n" +
        "InputLock: " + (inputLocked ? std::string("LOCKED") : std::string("FREE")) + "\n" +
        "SplitMode: " + (splitMode ? std::string("ON") : std::string("OFF")) + "\n" +
        "Controls: A/D move, W jump (inverted when psycho)\n" +
        "P: force toggle psycho | M: toggle music vol | B: toggle bg vol\n" +
        "1: play dialogue one-shot | 2: play effect one-shot\n" +
        "ESC: back to menu";
    m_debugText.setString(s);
    window.draw(m_debugText);

    window.display();
}

void Level::exitLevel1() {
    m_audio.StopMusic();
    if (m_dialogueEmitter) m_dialogueEmitter->stop();
    if (m_effectEmitter) m_effectEmitter->stop();
    m_player.reset();
}

// ---------------- Level2 stubs (for later) ----------------
void Level::enterLevel2(sf::RenderWindow& window) {
    (void)window;
    // TODO: implement Level2 later
}
void Level::processEventsLevel2(sf::RenderWindow& window) {
    (void)window;
}
void Level::updateLevel2(float dt) {
    (void)dt;
}
void Level::renderLevel2(sf::RenderWindow& window) {
    (void)window;
}
void Level::exitLevel2() {
}