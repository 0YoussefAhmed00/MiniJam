#include "Game.h"
#include "World.h"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <unordered_map>

using namespace sf;

constexpr float PPM = 30.f;
constexpr float INV_PPM = 1.f / PPM;
std::unordered_map<std::string, std::shared_ptr<AudioEmitter>> m_playerEmitters;

static float randomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
static float randomOffset(float magnitude) {
    return ((rand() % 2000) / 1000.f - 1.f) * magnitude;
}

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

Game::Game()
    : m_window(VideoMode::getDesktopMode(),
        "SFML + Box2D + AudioManager + Persona Demo",
        Style::Fullscreen),
    m_camera(FloatRect(0, 0,
        VideoMode::getDesktopMode().width,
        VideoMode::getDesktopMode().height)),
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
    m_running(true),
    m_state(GameState::MENU),
    m_lastAppliedAudioState(PlayerAudioState::Neutral)
{
    srand(static_cast<unsigned>(time(nullptr)));
    m_window.setFramerateLimit(60);
    m_world.SetContactListener(&m_contactListener);

    // Store World (creates obstacles and holds category bits)
    m_worldView = std::make_unique<World>(m_world);

    // Ground (Box2D)
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640 * INV_PPM, 680 * INV_PPM);
    b2Body* ground = m_world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(2000.0f * INV_PPM, 10.0f * INV_PPM);

    b2FixtureDef groundFix;
    groundFix.shape = &groundBox;
    groundFix.filter.categoryBits = m_worldView->CATEGORY_GROUND;
    groundFix.filter.maskBits = m_worldView->CATEGORY_PLAYER | m_worldView->CATEGORY_SENSOR | m_worldView->CATEGORY_OBSTACLE;
    ground->CreateFixture(&groundFix);

    // Ground visual
    m_groundShape = RectangleShape(Vector2f(4000.f, 40.f));
    m_groundShape.setOrigin(2000.f, 20.f);
    m_groundShape.setPosition(640.f, 680.f);
    m_groundShape.setFillColor(Color(80, 160, 80));

    // Player
    m_player = std::make_unique<Player>(&m_world, 640.f, 200.f);
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
        std::cerr << "Warning: dialogue.wav not loaded.\n";
    if (!m_effectEmitter->loadBuffer("assets/Audio/effect.wav"))
        std::cerr << "Warning: effect.wav not loaded.\n";

    m_audio.RegisterEmitter(m_dialogueEmitter);
    m_audio.RegisterEmitter(m_effectEmitter);
    m_dialogueEmitter->sound.setLoop(true);
    m_effectEmitter->sound.setLoop(true);

    m_audio.SetMasterVolume(1.f);
    m_audio.SetMusicVolume(0.9f);
    m_audio.SetBackgroundVolume(0.6f);
    m_audio.SetDialogueVolume(1.f);
    m_audio.SetEffectsVolume(1.f);

    // --- Player emitters ---
    auto createPlayerEmitter = [&](const std::string& id, const std::string& filePath) {
        auto emitter = std::make_shared<AudioEmitter>();
        emitter->id = id;
        emitter->category = AudioCategory::Effects;
        emitter->position = m_player->GetBody()->GetPosition();
        emitter->minDistance = 0.5f;
        emitter->maxDistance = 20.f;
        emitter->baseVolume = 1.f;

        if (!emitter->loadBuffer(filePath))
            std::cerr << "Warning: " << filePath << " not loaded.\n";

        m_audio.RegisterEmitter(emitter);
        m_playerEmitters[id] = emitter;
        };

    createPlayerEmitter("refuse", "assets/Audio/refuse.wav");
    createPlayerEmitter("jump", "assets/Audio/jump.wav");
    createPlayerEmitter("attack", "assets/Audio/attack.wav");
    // Add more player sounds as needed

    nextPsychoSwitch = randomFloat(6.f, 8.f);
    psychoClock.restart();

    nextSplitCheck = randomFloat(1.f, 3.f);
    splitClock.restart();

    nextInputLockCheck = randomFloat(3.f, 6.f);
    inputLockClock.restart();

    if (!m_font.loadFromFile("assets/Font/Myriad Arabic Regular.ttf")) {
        std::cerr << "Warning: font not loaded (assets/arial.ttf)\n";
    }
    m_debugText.setFont(m_font);
    m_debugText.setCharacterSize(16);
    m_debugText.setFillColor(Color::White);
    m_debugText.setPosition(10.f, 10.f);

    m_mainMenu = std::make_unique<MainMenu>(m_window.getSize());
    m_mainMenu->SetFont(&m_font);
    m_mainMenu->BuildLayout();

    m_mainMenu->OnPlay = [this]() {
        m_state = GameState::PLAYING;
        m_audio.StartMusic();
        if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
        if (m_effectEmitter->buffer) m_effectEmitter->sound.play();
        };
    m_mainMenu->OnExit = [this]() {
        m_window.close();
        };

    m_frameClock.restart();
}

Game::~Game() {}

int Game::Run()
{
    while (m_window.isOpen() && m_running) {
        float dt = m_frameClock.restart().asSeconds();

        processEvents();

        if (m_state == GameState::PLAYING) {
            if (m_worldView) m_worldView->update(dt);
            update(dt);
        }
        else {
            m_audio.StopMusic();
            m_mainMenu->Update(dt, m_window);
        }

        render();
    }
    return 0;
}

void Game::processEvents()
{
    Event ev;
    while (m_window.pollEvent(ev)) {
        if (ev.type == Event::Closed)
            m_window.close();

        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Escape) {
            if (m_state == GameState::MENU) {
                m_window.close();
            }
            else {
                m_state = GameState::MENU;
                m_audio.StopMusic();
                m_dialogueEmitter->stop();
                m_effectEmitter->stop();
                if (m_mainMenu) m_mainMenu->ResetMobileVisual();
            }
        }

        if (m_state == GameState::MENU) {
            if (ev.type == Event::MouseMoved) {
                sf::Vector2i mp = Mouse::getPosition(m_window);
                m_mainMenu->OnMouseMoved(m_window.mapPixelToCoords(mp));
            }
            if (ev.type == Event::MouseButtonPressed && ev.mouseButton.button == Mouse::Left) {
                sf::Vector2i mp = Mouse::getPosition(m_window);
                m_mainMenu->OnMousePressed(m_window.mapPixelToCoords(mp));
            }
            continue;
        }

        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::M) {
            static bool mToggle = false; mToggle = !mToggle; m_audio.SetMusicVolume(mToggle ? 0.0f : 1.f);
        }
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::B) {
            static bool bToggle = false; bToggle = !bToggle; m_audio.SetBackgroundVolume(bToggle ? 0.1f : 1.f);
        }
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::P) {
            psychoMode = !psychoMode;
            m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
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
    bool isGrounded = (m_contactListener.footContacts > 0);

    // Psycho mode switch
    if (psychoClock.getElapsedTime().asSeconds() >= nextPsychoSwitch) {
        psychoMode = !psychoMode;
        m_player->SetColor(psychoMode ? Color::Magenta : Color::Red);
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

    static bool refusePlayed = false; // for "refuse" sound once

    if (psychoMode) {
        float elapsedLock = inputLockClock.getElapsedTime().asSeconds();

        if (inputLockPending) {
            if (isGrounded) {
                inputLockPending = false;
                inputLocked = true;
                inputLockClock.restart();
                m_player->SetColor(Color::Cyan);
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

        // Play "refuse" once when input locked
        if (inputLocked) {
            if (!refusePlayed) {
                m_playerEmitters["refuse"]->sound.play();
                refusePlayed = true;
            }
        }
        else {
            refusePlayed = false;
        }

        // Split mode handling remains unchanged...
        // --- Split mode handling (replace the existing split-mode / inTransition code) ---

        float elapsedSplit = splitClock.getElapsedTime().asSeconds();
        if (!splitMode && !inTransition) {
            if (elapsedSplit >= nextSplitCheck) {
                if (rand() % 2 == 0) {
                    inTransition = true;
                    pendingEnable = true;
                    transitionClock.restart();

                    // Start/target rotation for smooth lerp
                    m_transitionStartRotation = m_camera.getRotation();
                    m_transitionTargetRotation = 180.f; // rotating 0 -> 180
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

                // Start/target rotation for smooth lerp
                m_transitionStartRotation = m_camera.getRotation();
                m_transitionTargetRotation = 0.f; // rotating 180 -> 0
            }
        }

        if (inTransition) {
            float t = transitionClock.getElapsedTime().asSeconds();
            float alpha = t / TRANSITION_TIME;
            if (alpha > 1.f) alpha = 1.f;

            // Smoothstep easing for a nicer feel: ease = 3a^2 - 2a^3
            float ease = alpha * alpha * (3.f - 2.f * alpha);

            // Interpolate rotation and apply every frame while transitioning
            float curRot = m_transitionStartRotation + (m_transitionTargetRotation - m_transitionStartRotation) * ease;
            m_camera.setRotation(curRot);

            if (alpha >= 1.f) {
                // transition finished — set final state exactly and reset flags
                if (pendingEnable) {
                    splitMode = true;
                    splitDuration = randomFloat(2.f, 4.f);
                    m_camera.setRotation(180.f); // ensure exact final value
                    splitClock.restart();
                }
                else {
                    splitMode = false;
                    m_camera.setRotation(0.f); // ensure exact final value
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

    b2Vec2 vel = m_player->GetLinearVelocity();
    float moveSpeed = 5.f;

    bool leftKey = Keyboard::isKeyPressed(Keyboard::A);
    bool rightKey = Keyboard::isKeyPressed(Keyboard::D);
    bool jumpKeyW = Keyboard::isKeyPressed(Keyboard::W);
    bool jumpKeyS = Keyboard::isKeyPressed(Keyboard::S);
    bool shiftKey = Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift);

    // Shift = Walk (slower), else Run (faster)
    moveSpeed = (shiftKey ? 5.f : 10.f);
    m_player->SetWalking(shiftKey);

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


    // Update player emitter positions
    b2Vec2 playerPos = m_player->GetBody()->GetPosition();
    for (auto& [id, emitter] : m_playerEmitters) {
        emitter->position = playerPos;
    }

    // Collision detection
    if (m_worldView && m_player) {
        sf::RectangleShape playerShape;
        b2Body* body = m_player->GetBody();
        if (body) {
            const b2Transform& xf = body->GetTransform();
            float minx = 1e9f, miny = 1e9f, maxx = -1e9f, maxy = -1e9f;

            for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()) {
                if (f->IsSensor()) continue;
                const b2Shape* s = f->GetShape();
                if (s->GetType() != b2Shape::e_polygon) continue;

                const b2PolygonShape* poly = static_cast<const b2PolygonShape*>(s);
                for (int i = 0; i < poly->m_count; ++i) {
                    b2Vec2 v = b2Mul(xf, poly->m_vertices[i]);
                    if (v.x < minx) minx = v.x;
                    if (v.x > maxx) maxx = v.x;
                    if (v.y < miny) miny = v.y;
                    if (v.y > maxy) maxy = v.y;
                }
                break;
            }

            if (maxx > minx && maxy > miny) {
                float w = (maxx - minx) * PPM;
                float h = (maxy - miny) * PPM;
                playerShape.setSize(sf::Vector2f(w, h));
                playerShape.setOrigin(w * 0.5f, h * 0.5f);
                playerShape.setPosition(((minx + maxx) * 0.5f) * PPM, ((miny + maxy) * 0.5f) * PPM);
            }
            else {
                b2Vec2 p = body->GetPosition();
                playerShape.setSize(sf::Vector2f(40.f, 40.f));
                playerShape.setOrigin(20.f, 20.f);
                playerShape.setPosition(p.x * PPM, p.y * PPM);
            }
        }
        m_worldView->checkCollision(playerShape, m_window);
    }

    // Audio crossfade logic
    PlayerAudioState cur = m_player->GetAudioState();
    if (cur != m_lastAppliedAudioState) {
        if (cur == PlayerAudioState::Crazy) m_audio.CrossfadeToCrazy();
        else m_audio.CrossfadeToNeutral();
        m_lastAppliedAudioState = cur;
    }

    m_audio.Update(dt, playerPos);
}

void Game::render()
{
    if (m_state == GameState::MENU) {
        m_window.setView(m_defaultView);
        m_window.clear(Color(20, 20, 30));
        m_mainMenu->Render(m_window);
        m_window.display();
        return;
    }

    m_player->SyncGraphics();

    m_diagMark.setPosition(m_dialogueEmitter->position.x * PPM, m_dialogueEmitter->position.y * PPM);
    m_fxMark.setPosition(m_effectEmitter->position.x * PPM, m_effectEmitter->position.y * PPM);

    b2Vec2 pos = m_player->GetBody()->GetPosition();
    Vector2f playerPosPixels(pos.x * PPM, pos.y * PPM);
    Vector2f camCenter = {playerPosPixels.x, playerPosPixels.y -150};
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

    if (m_worldView) {
        m_worldView->draw(m_window);
    }

    m_window.draw(m_groundShape);
    m_player->Draw(m_window);

    m_window.draw(m_diagMark);
    m_window.draw(m_fxMark);

    m_window.setView(m_defaultView);
    std::string s = std::string("State: PLAYING\n") +
        "PsychoMode: " + (psychoMode ? "ON" : "OFF") + "\n" +
        "InputLock: " + (inputLocked ? std::string("LOCKED") : std::string("FREE")) + "\n" +
        "SplitMode: " + (splitMode ? std::string("ON") : std::string("OFF")) + "\n" +
        "Controls: A/D move, W jump (inverted when psycho)\n" +
        "P: force toggle psycho | M: toggle music vol | B: toggle bg vol\n" +
        "1: play dialogue one-shot | 2: play effect one-shot\n" +
        "ESC: back to menu";
    m_debugText.setString(s);
    m_window.draw(m_debugText);

    m_window.display();
}
