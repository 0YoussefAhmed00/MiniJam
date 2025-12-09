#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <memory>
#include "Units.h"
#include "AudioManager.h"
#include "Player.h"
#include "MainMenu.h"

class World; // forward declaration

// Game-wide state
enum class GameState { MENU, PLAYING, WIN, LOSE, EXIT, CurrentLevel };

class Game {
public:
    Game();
    ~Game();

    int Run();

private:
    class MyContactListener : public b2ContactListener {
    public:
        MyContactListener() : footContacts(0), footFixture(nullptr) {}
        int footContacts;
        b2Fixture* footFixture;

        void BeginContact(b2Contact* contact) override;
        void EndContact(b2Contact* contact) override;
    };

    void processEvents();
    void update(float dt);
    void render();

private:
    // SFML
    sf::RenderWindow m_window;
    sf::View m_camera;
    sf::View m_defaultView;

    // Game.h (inside class Game private section)
    float m_transitionStartRotation = 0.f;
    float m_transitionTargetRotation = 0.f;


    // Physics
    b2Vec2 m_gravity;
    b2World m_world;

    // Contact listener
    MyContactListener m_contactListener;

    // Game state
    GameState m_state = GameState::MENU;

    // Game objects
    std::unique_ptr<Player> m_player;

    // World/obstacles
    std::unique_ptr<World> m_worldView;

    // Ground visual
    sf::RectangleShape m_groundShape;

    // Audio
    AudioManager m_audio;
    std::shared_ptr<AudioEmitter> m_dialogueEmitter;
    std::shared_ptr<AudioEmitter> m_effectEmitter;
    sf::CircleShape m_diagMark;
    sf::CircleShape m_fxMark;

    // Persona variables
    bool psychoMode;
    float nextPsychoSwitch;
    sf::Clock psychoClock;

    bool splitMode;
    float splitDuration;
    float nextSplitCheck;
    sf::Clock splitClock;
    bool inTransition;
    bool pendingEnable;
    sf::Clock transitionClock;
    static constexpr float TRANSITION_TIME = 0.25f;
    static constexpr float PSYCHO_SHAKE_MAG = 4.f;
    static constexpr float TRANSITION_SHAKE_MAG = 20.f;

    bool inputLocked;
    bool inputLockPending;
    sf::Clock inputLockClock;
    static constexpr float INPUT_LOCK_DURATION = 2.f;
    float nextInputLockCheck;

    // Debug text / UI
    sf::Font m_font;
    sf::Text m_debugText;

    // Main Menu
    std::unique_ptr<MainMenu> m_mainMenu;

    // Frame clock
    sf::Clock m_frameClock;

    // other
    bool m_running;

    PlayerAudioState m_lastAppliedAudioState = PlayerAudioState::Neutral;
};

#endif // GAME_H