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
        1920,
        1080)),
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
    m_worldView = std::make_unique<World>(m_world);

    // Store World (creates obstacles and holds category bits)
    m_worldView = std::make_unique<World>(m_world);

    // Ground (Box2D)
    b2BodyDef groundDef;
    groundDef.type = b2_staticBody;
    groundDef.position.Set(640 * INV_PPM, 880 * INV_PPM);
    b2Body* ground = m_world.CreateBody(&groundDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(2000.0f *10* INV_PPM, 10.0f * INV_PPM);

    b2FixtureDef groundFix;
    groundFix.shape = &groundBox;
    groundFix.filter.categoryBits = m_worldView->CATEGORY_GROUND;
    groundFix.filter.maskBits = m_worldView->CATEGORY_PLAYER | m_worldView->CATEGORY_SENSOR | m_worldView->CATEGORY_OBSTACLE;
    ground->CreateFixture(&groundFix);

    // Player
    m_player = std::make_unique<Player>(&m_world, 140.f, 800.f);
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


    // player reply emitter (just like refuse)
    m_playerReply = std::make_shared<AudioEmitter>();
    m_playerReply->id = "playerReply";
    m_playerReply->category = AudioCategory::Dialogue;
    m_playerReply->minDistance = 0.5f;
    m_playerReply->maxDistance = 20.f;
    m_playerReply->baseVolume = 1.f;
    m_playerReply->position = b2Vec2(0.f, 0.f);  // optional: track player position if you want
    if (!m_playerReply->loadBuffer("assets/Audio/player_reply.wav")) {
        std::cerr << "Warning: player reply audio not loaded\n";
    }
    m_playerReply->sound.setLoop(false);
    m_audio.RegisterEmitter(m_playerReply);

    // find the grocery obstacle by filename substring (change "grocery" to match your filename)
    m_groceryObstacleIndex = m_worldView->findObstacleByTextureSubstring("grocery");

    if (m_groceryObstacleIndex >= 0)
    {
        auto makeGroceryEmitter = [&](const std::string& id, const std::string& filePath)->std::shared_ptr<AudioEmitter> {
            auto e = std::make_shared<AudioEmitter>();
            e->id = id;
            e->category = AudioCategory::Dialogue; // grocery is dialogue-like
            e->minDistance = 0.5f;
            e->maxDistance = 50.f;
            e->baseVolume = 1.f;
            // initial position (will be updated each frame in update())
            b2Vec2 p = m_worldView->getObstacleBodyPosition(m_groceryObstacleIndex);
            e->position = p;
            if (!e->loadBuffer(filePath)) {
                std::cerr << "Warning: grocery audio not loaded: " << filePath << "\n";
            }
            e->sound.setLoop(false);
            m_audio.RegisterEmitter(e);
            return e;
            };

        // filenames — create these WAV/OGG files in your assets folder
        m_groceryA = makeGroceryEmitter("groceryA", "assets/Audio/grocery_line1.wav");
        m_groceryB = makeGroceryEmitter("groceryB", "assets/Audio/grocery_line2.wav");
        m_groceryCollision = makeGroceryEmitter("groceryCollision", "assets/Audio/grocery_collision.wav");

        m_groceryClock.restart();
        m_nextGroceryLineTime = randomFloat(5.f, 10.f);
    }




    m_audio.SetCrossfadeTime(1.2f);
    bool ok = m_audio.loadMusic("Assets/Audio/music_neutral.ogg", "Assets/Audio/music_crazy.ogg");
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
    m_audio.SetDialogueVolume(1.0f);
    m_audio.SetEffectsVolume(1.f);

    // --- Player emitters ---
    auto createPlayerEmitter = [&](const std::string& id, const std::string& filePath) {
        auto emitter = std::make_shared<AudioEmitter>();
        emitter->id = id;
        emitter->category = AudioCategory::Dialogue; // <-- make it dialogue so dialogue volume affects it
        emitter->position = m_player->GetBody()->GetPosition();
        emitter->minDistance = 0.5f;
        emitter->maxDistance = 20.f;
        emitter->baseVolume = 1.f;

        if (!emitter->loadBuffer(filePath))
            std::cerr << "Warning: " << filePath << " not loaded.\n";

        emitter->sound.setLoop(false);
        m_audio.RegisterEmitter(emitter);
        m_playerEmitters[id] = emitter;
        };

    createPlayerEmitter("refuse", "assets/Audio/refuse.wav");
    createPlayerEmitter("player_reply", "assets/Audio/player_reply.wav");
    auto dbgIt = m_playerEmitters.find("player_reply");
    if (dbgIt == m_playerEmitters.end() || !dbgIt->second || !dbgIt->second->buffer) {
        std::cerr << "DEBUG: player_reply emitter missing or buffer not loaded. Check path & case sensitivity.\n";
    }
    else {
        std::cerr << "DEBUG: player_reply emitter loaded successfully.\n";
    }

    //createPlayerEmitter("jump", "assets/Audio/jump.wav");
    //createPlayerEmitter("attack", "assets/Audio/attack.wav");
    //// Add more player sounds as needed

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
        // Reset gameplay state and timers before starting
        psychoMode = false;
        m_player->SetColor(sf::Color::Red);
        m_player->SetAudioState(PlayerAudioState::Neutral);
        m_lastAppliedAudioState = PlayerAudioState::Neutral;

        splitMode = false;
        inTransition = false;
        pendingEnable = false;
        m_camera.setRotation(0.f);

        nextPsychoSwitch = randomFloat(6.f, 8.f);
        psychoClock.restart();

        nextSplitCheck = randomFloat(1.f, 3.f);
        splitClock.restart();

        inputLocked = false;
        inputLockPending = false;
        nextInputLockCheck = randomFloat(3.f, 6.f);
        inputLockClock.restart();

        m_state = GameState::PLAYING;
        m_audio.StartMusic();
        if (m_dialogueEmitter->buffer) m_dialogueEmitter->sound.play();
        if (m_effectEmitter->buffer) m_effectEmitter->sound.play();
        };
    m_mainMenu->OnExit = [this]() {
        m_window.close();
        };
    m_mainMenu->OnOptions = [this]() {
    // Fullscreen modal Options window
    const auto desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow opts(desktop, "Options", sf::Style::Fullscreen);
    opts.setFramerateLimit(60);

    // ------------------------------------
    // Load Controls image
    // ------------------------------------
    sf::Texture controlsTex;
    controlsTex.loadFromFile("Assets/MainMenu/Controls.png");
    sf::Sprite controlsSprite(controlsTex);

    // Scale + center image (keeps aspect ratio)
    float maxWidth = desktop.width *0.5f;
    float scale = maxWidth / controlsTex.getSize().x;
    controlsSprite.setScale(scale, scale);
    controlsSprite.setPosition(
        desktop.width *0.5f - controlsSprite.getGlobalBounds().width *0.5f,
        desktop.height *0.1f
    );

    // ------------------------------------
    // Movement Controls text (added)
    // ------------------------------------
    sf::Text moveText("Movement Controls", m_font,48);
    moveText.setFillColor(sf::Color::White);

    sf::FloatRect imgBounds = controlsSprite.getGlobalBounds();
    moveText.setPosition(
        imgBounds.left + imgBounds.width *0.5f - moveText.getGlobalBounds().width *0.5f,
        imgBounds.top - moveText.getGlobalBounds().height -10.f
    );

    // Volume sliders
    struct Slider {
        std::string label;
        float value;
        sf::Text labelText;
        sf::Text valueText;
        sf::RectangleShape bar;
        sf::RectangleShape fill;
        sf::CircleShape knob;
        float x, y, width, height, knobRadius;
        bool dragging{ false };
        std::function<void(float)> apply;
    };

    std::vector<Slider> sliders;

    auto makeSlider = [&](const std::string& label, float initial, float y, std::function<void(float)> apply){
        Slider s;
        s.label = label;
        s.value = std::clamp(initial,0.f,1.f);
        s.x =60.f;
        s.y = y;
        s.width = desktop.width -120.f;
        s.height =10.f;
        s.knobRadius =18.f;
        s.apply = apply;

        s.labelText = sf::Text(label, m_font,42);
        s.labelText.setFillColor(sf::Color::White);
        s.labelText.setPosition(s.x, y -60.f);

        s.valueText = sf::Text("", m_font,32);
        s.valueText.setFillColor(sf::Color(200,200,200));
        s.valueText.setPosition(s.x, y -20.f);

        s.bar = sf::RectangleShape(sf::Vector2f(s.width, s.height));
        s.bar.setPosition(s.x, s.y);
        s.bar.setFillColor(sf::Color(100,100,140));

        s.fill = sf::RectangleShape(sf::Vector2f(s.width * s.value, s.height));
        s.fill.setPosition(s.x, s.y);
        s.fill.setFillColor(sf::Color(120,180,255));

        s.knob = sf::CircleShape(s.knobRadius);
        s.knob.setFillColor(sf::Color(240,240,240));
        s.knob.setOutlineThickness(2.f);
        s.knob.setOutlineColor(sf::Color(80,80,120));
        float cx = s.x + s.width * s.value;
        s.knob.setPosition(cx - s.knobRadius, s.y + s.height /2.f - s.knobRadius);

        auto updateUI = [&](Slider& sl, float v){
            sl.value = std::clamp(v,0.f,1.f);
            if (sl.apply) sl.apply(sl.value);
            sl.fill.setSize(sf::Vector2f(sl.width * sl.value, sl.height));
            float cx2 = sl.x + sl.width * sl.value;
            sl.knob.setPosition(cx2 - sl.knobRadius, sl.y + sl.height /2.f - sl.knobRadius);
            int percent = static_cast<int>(std::round(sl.value *100.f));
            sl.valueText.setString(std::to_string(percent) + "%");
        };
        updateUI(s, s.value);
        sliders.push_back(s);
        return updateUI; // not used externally
    };

    // Initialize sliders with current volumes (AudioManager tracks values internally)
    makeSlider("Master",1.0f, desktop.height *0.55f, [&](float v){ m_audio.SetMasterVolume(v); });
    makeSlider("Music",0.9f, desktop.height *0.62f, [&](float v){ m_audio.SetMusicVolume(v); });
    makeSlider("Background",0.6f, desktop.height *0.69f, [&](float v){ m_audio.SetBackgroundVolume(v); });
    makeSlider("Dialogue",1.0f, desktop.height *0.76f, [&](float v){ m_audio.SetDialogueVolume(v); });
    makeSlider("Effects",1.0f, desktop.height *0.83f, [&](float v){ m_audio.SetEffectsVolume(v); });

    bool exiting = false;
    sf::Clock exitClock;
    const float exitDuration =0.35f;
    sf::RectangleShape fadeOverlay(sf::Vector2f((float)desktop.width, (float)desktop.height));
    fadeOverlay.setFillColor(sf::Color(0,0,0,0));

    while (opts.isOpen()) {
        sf::Event ev;
        while (opts.pollEvent(ev)) {
            if (exiting) continue;

            if (ev.type == sf::Event::Closed ||
                (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape)) {
                exiting = true;
                exitClock.restart();
            }

            auto handleMouseToSlider = [&](Slider& s, const sf::Vector2f& mp){
                sf::FloatRect bounds(s.x, s.y -20.f, s.width, s.height +40.f);
                if (bounds.contains(mp)) {
                    s.dragging = true;
                    float newVal = (mp.x - s.x) / s.width;
                    float clamped = std::clamp(newVal,0.f,1.f);
                    // update
                    s.fill.setSize(sf::Vector2f(s.width * clamped, s.height));
                    float cx = s.x + s.width * clamped;
                    s.knob.setPosition(cx - s.knobRadius, s.y + s.height /2.f - s.knobRadius);
                    int percent = static_cast<int>(std::round(clamped *100.f));
                    s.valueText.setString(std::to_string(percent) + "%");
                    if (s.apply) s.apply(clamped);
                    s.value = clamped;
                }
            };

            if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mp((float)ev.mouseButton.x, (float)ev.mouseButton.y);
                for (auto& s : sliders) handleMouseToSlider(s, mp);
            }

            if (ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Left) {
                for (auto& s : sliders) s.dragging = false;
            }

            if (ev.type == sf::Event::MouseMoved) {
                sf::Vector2f mp((float)ev.mouseMove.x, (float)ev.mouseMove.y);
                for (auto& s : sliders) {
                    if (s.dragging) {
                        float newVal = (mp.x - s.x) / s.width;
                        float clamped = std::clamp(newVal,0.f,1.f);
                        s.fill.setSize(sf::Vector2f(s.width * clamped, s.height));
                        float cx = s.x + s.width * clamped;
                        s.knob.setPosition(cx - s.knobRadius, s.y + s.height /2.f - s.knobRadius);
                        int percent = static_cast<int>(std::round(clamped *100.f));
                        s.valueText.setString(std::to_string(percent) + "%");
                        if (s.apply) s.apply(clamped);
                        s.value = clamped;
                    }
                }
            }
        }

        // fade out animation
        if (exiting) {
            float t = std::min(exitClock.getElapsedTime().asSeconds() / exitDuration, 1.f);
            float ease = t * t * (3.f - 2.f * t);
            uint8_t alpha = (uint8_t)(255.f * ease);
            fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha));

            if (t >= 1.f) {
                opts.close();
                break;
            }
        }

        opts.clear(sf::Color(30,30,40));

        opts.draw(moveText);
        opts.draw(controlsSprite);
        // draw sliders
        for (auto& s : sliders) {
            opts.draw(s.labelText);
            opts.draw(s.valueText);
            opts.draw(s.bar);
            opts.draw(s.fill);
            opts.draw(s.knob);
        }
        if (exiting) opts.draw(fadeOverlay);

        opts.display();
    }

    if (m_mainMenu) {
        m_mainMenu->ResetMobileVisual();
    }
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
            if (m_worldView) m_worldView->update(dt, m_camera.getCenter());
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

                // Reset gameplay state and timers while in menu so they don't accumulate
                psychoMode = false;
                m_player->SetColor(sf::Color::Red);
                m_player->SetAudioState(PlayerAudioState::Neutral);
                m_lastAppliedAudioState = PlayerAudioState::Neutral;

                splitMode = false;
                inTransition = false;
                pendingEnable = false;
                m_camera.setRotation(0.f);

                nextPsychoSwitch = randomFloat(6.f, 8.f);
                psychoClock.restart();

                nextSplitCheck = randomFloat(1.f, 3.f);
                splitClock.restart();

                inputLocked = false;
                inputLockPending = false;
                nextInputLockCheck = randomFloat(3.f, 6.f);
                inputLockClock.restart();
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
        if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Y) {
            if (m_playerReply && m_playerReply->buffer) {
                m_playerReply->sound.play();
            }
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
    bool wavePlayed = false;


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
                if (rand() % 3 == 0) {
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
                auto it = m_playerEmitters.find("refuse");
                if (it != m_playerEmitters.end() && it->second) {
                    it->second->sound.play();
                }
                if (inputLocked && !wavePlayed)
                {
                    m_player->PlayWave();
                    wavePlayed = true;
                }
                else if (!inputLocked)
                {
                    wavePlayed = false;
                }
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

    bool leftKey = Keyboard::isKeyPressed(Keyboard::A) || Keyboard::isKeyPressed(Keyboard::Left);
    bool rightKey = Keyboard::isKeyPressed(Keyboard::D) || Keyboard::isKeyPressed(Keyboard::Right);
    bool jumpKeyW = Keyboard::isKeyPressed(Keyboard::W) || Keyboard::isKeyPressed(Keyboard::Up);
    bool jumpKeyS = Keyboard::isKeyPressed(Keyboard::S)|| Keyboard::isKeyPressed(Keyboard::Down) ;
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
        m_worldView->checkCollision(playerShape);
        // ----- Grocery: update emitter positions -----
        // ----- Grocery: update emitter positions -----
        if (m_groceryObstacleIndex >= 0)
        {
            b2Vec2 gpos = m_worldView->getObstacleBodyPosition(m_groceryObstacleIndex);
            if (m_groceryA) m_groceryA->position = gpos;
            if (m_groceryB) m_groceryB->position = gpos;
            if (m_groceryCollision) m_groceryCollision->position = gpos;
        }

        // ---- Grocery: collision vs ambient behavior ----
        bool collidingWithGrocery = (m_worldView->getLastCollidedObstacleIndex() == m_groceryObstacleIndex);

        if (m_groceryObstacleIndex >= 0)
        {
            if (collidingWithGrocery)
            {
                // Stop ambient lines so collision line is clean
                if (m_groceryA && m_groceryA->sound.getStatus() == sf::Sound::Playing) m_groceryA->sound.stop();
                if (m_groceryB && m_groceryB->sound.getStatus() == sf::Sound::Playing) m_groceryB->sound.stop();

                // If we haven't yet started the collision sequence for this contact, start it
                if (!m_groceryCollisionPlayed)
                {
                    if (m_groceryCollision && m_groceryCollision->buffer) {
                        m_groceryCollision->sound.stop(); // ensure restart
                        m_groceryCollision->sound.play();
                        m_groceryWaitingPlayerReply = true;  // wait until grocery line finishes (persist even if player leaves)
                        std::cerr << "DEBUG: grocery collision line started\n";
                    }
                    m_groceryCollisionPlayed = true;
                }
            }
            else
            {
                // NOT colliding: do NOT clear m_groceryWaitingPlayerReply — we still want the player reply
                // Ambient random chatter should only occur when we're not waiting for a reply
                if (!m_groceryWaitingPlayerReply)
                {
                    if (m_groceryClock.getElapsedTime().asSeconds() >= m_nextGroceryLineTime)
                    {
                        m_groceryClock.restart();
                        m_nextGroceryLineTime = randomFloat(5.f, 10.f);

                        // skip playing if any ambient is already playing
                        if (m_groceryA && m_groceryA->buffer && m_groceryA->sound.getStatus() != sf::Sound::Playing &&
                            m_groceryB && m_groceryB->buffer && m_groceryB->sound.getStatus() != sf::Sound::Playing)
                        {
                            if (rand() % 2 == 0)
                            {
                                if (m_groceryA && m_groceryA->buffer) m_groceryA->sound.play();
                            }
                            else
                            {
                                if (m_groceryB && m_groceryB->buffer) m_groceryB->sound.play();
                            }
                        }
                    }
                }
                // else: we're waiting for grocery collision line to finish -> do nothing here
            }

            // ---- waiting-for-reply handler (run regardless of collision state) ----
            if (m_groceryWaitingPlayerReply)
            {
                // Guard: if grocery emitter exists, wait until it reports Stopped
                bool groceryStopped = true; // default true if no emitter (fallback)
                if (m_groceryCollision && m_groceryCollision->buffer) {
                    groceryStopped = (m_groceryCollision->sound.getStatus() == sf::Sound::Stopped);
                }

                if (groceryStopped)
                {
                    // Play player reply exactly like "refuse" (from map)
                    auto itReply = m_playerEmitters.find("player_reply");
                    if (itReply != m_playerEmitters.end() && itReply->second && itReply->second->buffer) {
                        // restart to ensure we always hear it
                        itReply->second->sound.stop();
                        itReply->second->sound.play();
                        std::cerr << "DEBUG: played player_reply after grocery finished\n";
                    }
                    else {
                        std::cerr << "DEBUG: player_reply emitter not found / buffer missing\n";
                    }

                    // Done waiting for this collision — reset flags so future collisions can re-trigger
                    m_groceryWaitingPlayerReply = false;
                    m_groceryCollisionPlayed = false;
                }
            }
        }
        else
        {
            // no collision — reset collision sequence and enable ambient random chatter
            m_groceryCollisionPlayed = false;
            m_groceryWaitingPlayerReply = false;

            if (m_groceryClock.getElapsedTime().asSeconds() >= m_nextGroceryLineTime)
            {
                m_groceryClock.restart();
                m_nextGroceryLineTime = randomFloat(5.f, 10.f);

                // skip playing if grocery collision emitter is mid-play (unlikely since not colliding)
                if (m_groceryA && m_groceryA->buffer && m_groceryA->sound.getStatus() != sf::Sound::Playing &&
                    m_groceryB && m_groceryB->buffer && m_groceryB->sound.getStatus() != sf::Sound::Playing)
                {
                    if (rand() % 2 == 0)
                    {
                        if (m_groceryA && m_groceryA->buffer) m_groceryA->sound.play();
                    }
                    else
                    {
                        if (m_groceryB && m_groceryB->buffer) m_groceryB->sound.play();
                    }
                }
                else
                {
                    // If one ambient is playing, just skip this tick and let timer pick the next.
                }
            }
        }


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
    Vector2f camCenter = {playerPosPixels.x, 540};
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
        // Draw background layers and obstacles
        m_worldView->drawParallaxBackground(m_window);
        m_worldView->draw(m_window); // draw obstacles
    }

    // Draw player
    m_player->Draw(m_window);

    // Draw foreground layers
    if (m_worldView) {
        m_worldView->drawParallaxForeground(m_window);
    }

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
