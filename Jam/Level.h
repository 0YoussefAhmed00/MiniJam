#pragma once

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "Units.h"
#include "AudioManager.h"
#include "Player.h"
#include "MyContactListener.h"

// Unified Level that will host multiple stages (Level1 now, Level2 later).
class Level {
public:
    enum class Stage {
        Level1,
        Level2 // not implemented yet
    };

    explicit Level(Stage initialStage = Stage::Level1);
    ~Level();

    // Lifecycle called by Game manager
    void enter(sf::RenderWindow& window);
    void processEvents(sf::RenderWindow& window);
    void update(float dt);
    void render(sf::RenderWindow& window);
    void exit();

    // Completion and progression
    bool isComplete() const { return m_complete; }
    Stage nextStage() const { return Stage::Level2; } // placeholder for future progression

    // Back-to-menu request
    bool wantsReturnToMenu() const { return m_returnToMenu; }

private:
    // Views
    sf::View m_camera{};
    sf::View m_defaultView{};

    // Ground graphics
    sf::RectangleShape m_groundShape;

    // Physics
    b2Vec2 m_gravity{ 0.f, 9.8f };
    b2World m_world;
    MyContactListener m_contactListener;

    // Player
    std::unique_ptr<Player> m_player;

    // Audio
    AudioManager m_audio;
    std::shared_ptr<AudioEmitter> m_dialogueEmitter;
    std::shared_ptr<AudioEmitter> m_effectEmitter;

    // UI/debug
    sf::Font m_font;
    sf::Text m_debugText;
    sf::CircleShape m_diagMark;
    sf::CircleShape m_fxMark;

    // Stage control
    Stage m_stage;
    bool m_complete = false;
    bool m_returnToMenu = false;

    // Level1 state (migrated from Game.cpp)
    bool psychoMode = false;
    float nextPsychoSwitch = 0.f;

    bool splitMode = false;
    float splitDuration = 0.f;
    float nextSplitCheck = 0.f;
    bool inTransition = false;
    bool pendingEnable = false;

    bool inputLocked = false;
    bool inputLockPending = false;
    float nextInputLockCheck = 0.f;

    sf::Clock m_frameClock;
    sf::Clock psychoClock;
    sf::Clock splitClock;
    sf::Clock transitionClock;
    sf::Clock inputLockClock;

    PlayerAudioState m_lastAppliedAudioState = PlayerAudioState::Neutral;

    // Local constants (tuned to match previous Game.cpp usage)
    static constexpr float INPUT_LOCK_DURATION = 1.0f;
    static constexpr float TRANSITION_TIME = 0.6f;
    static constexpr float TRANSITION_SHAKE_MAG = 8.f;
    static constexpr float PSYCHO_SHAKE_MAG = 8.f;

    // Helpers
    static float randomFloat(float min, float max);
    static float randomOffset(float magnitude);

    // Stage-specific implementations
    void enterLevel1(sf::RenderWindow& window);
    void processEventsLevel1(sf::RenderWindow& window);
    void updateLevel1(float dt);
    void renderLevel1(sf::RenderWindow& window);
    void exitLevel1();

    // Stubs for Level2 (future)
    void enterLevel2(sf::RenderWindow& window);
    void processEventsLevel2(sf::RenderWindow& window);
    void updateLevel2(float dt);
    void renderLevel2(sf::RenderWindow& window);
    void exitLevel2();
};