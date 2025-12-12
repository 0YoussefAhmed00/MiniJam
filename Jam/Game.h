#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>
#include <memory>
#include <unordered_map>
#include "Units.h"
#include "AudioManager.h"
#include "Player.h"
#include "MainMenu.h"
#include <vector>

class World; // forward declaration

// Game-wide state
enum class GameState { MENU, PLAYING, WIN, LOSE, EXIT, CurrentLevel };

// near other includes in Game.h


struct Bus {
    sf::Sprite sprite;
    sf::Vector2f startPos;   // pixel coordinates
    sf::Vector2f endPos;     // pixel coordinates
    float duration = 2.f;    // seconds to cross (user requested 2s)
    float progress = 0.f;    // 0..1
    bool active = false;
    bool playedEmitter = false;
};


class Game {
public:
    Game();
    ~Game();
    void ResetGameplay(bool resetPlayerPosition = true);

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
    float m_lastPlayerReplyTime = -100.f;
    void processEvents();
    void update(float dt);
    void SpawnBus();
    void render();

private:
    // SFML
    sf::RenderWindow m_window;
    sf::View m_camera;
    sf::View m_defaultView;

    // Game.h (inside class Game private section)
    float m_transitionStartRotation =0.f;
    float m_transitionTargetRotation =0.f;


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

    //Grocery Man Variables
    // Grocery audio
    int m_groceryObstacleIndex = -1;
    std::shared_ptr<AudioEmitter> m_groceryA; // ambient line A
    std::shared_ptr<AudioEmitter> m_groceryB; // ambient line B
    std::shared_ptr<AudioEmitter> m_groceryCollision; // collision/callout line
    std::shared_ptr<AudioEmitter> m_playerReply; // add as a private member of Game
    bool m_groceryCollisionPlayed = false; // already used, but ensure it's declared as a member
    bool m_groceryWaitingPlayerReply = false;

    sf::Clock m_groceryClock;
    float m_nextGroceryLineTime =0.f;
    bool m_lastGroceryColliding = false;

    // Bus system
    sf::Texture m_busTexture;
    std::vector<Bus> m_buses;
    sf::Clock m_busSpawnClock;
    float m_busSpawnInterval = 10.f;   // every 30 seconds
    float m_busTravelTime = 3.f;       // how long it takes (2s)
    float m_busSpawnMargin = 800.f;    // how far outside the view the bus starts/ends (pixels)
    std::shared_ptr<AudioEmitter> m_busEmitter; // shared emitter used for bus pass sound
    std::string m_busAudioPath = "assets/Audio/bus_pass.wav"; // ensure this file exists


    std::shared_ptr<AudioEmitter> PlayerReply;
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
    static constexpr float TRANSITION_TIME =0.25f;
    static constexpr float PSYCHO_SHAKE_MAG =4.f;
    static constexpr float TRANSITION_SHAKE_MAG =20.f;

    bool inputLocked;
    bool inputLockPending;
    sf::Clock inputLockClock;
    static constexpr float INPUT_LOCK_DURATION =2.f;
    float nextInputLockCheck;

    // Game.h (inside class Game, private:)
    bool m_gameOver = false;                      // true while countdown active
    sf::Clock m_gameOverClock;
    float m_gameOverDelay = 3.f;
    sf::Text m_gameOverText;
    bool m_disableInputDuringGameOver = false;    // prevents handling input while counting down


    // Debug text / UI
    sf::Font m_font;
    sf::Text m_debugText;

    // Main Menu
    std::unique_ptr<MainMenu> m_mainMenu;

    // Pause UI
    bool m_paused{ false };
    std::unique_ptr<MenuButton> m_pauseResumeButton;
    std::unique_ptr<MenuButton> m_pauseBackButton;
    sf::RectangleShape m_pauseOverlay;
    std::unordered_map<std::string, sf::Sound::Status> m_emitterPrevStatus;

    // Frame clock
    sf::Clock m_frameClock;

    // other
    bool m_running;

    PlayerAudioState m_lastAppliedAudioState = PlayerAudioState::Neutral;

    sf::Clock m_groceryCooldownClock;
    float m_groceryCooldownDuration =4.f; // seconds
    bool m_groceryCooldownActive = false;
};

#endif // GAME_H