#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <SFML/Graphics.hpp>
#include "Level.h"

// Central game manager: holds window, menu state, and active level.
// It owns the main loop and swaps levels based on `Level::isComplete`.
class GameManager {
public:
    GameManager();
    ~GameManager() = default;

    int run();

    // Register a level factory by id. Example: "Level1", "Level2".
    void registerLevel(const std::string& id, std::function<LevelPtr()> factory);

    // Switch to a specific level id (calls exit/enter appropriately).
    bool switchLevel(const std::string& id);

    // Accessors
    sf::RenderWindow& window() { return m_window; }
    const sf::View& defaultView() const { return m_defaultView; }
    sf::View& defaultView() { return m_defaultView; }

    // Menu handling (simple boolean for now). You can replace with your `MainMenu`.
    void showMenu();
    void hideMenu();
    bool inMenu() const { return m_inMenu; }

private:
    sf::RenderWindow m_window;
    sf::View m_defaultView;

    bool m_running = true;
    bool m_inMenu = true;

    LevelPtr m_activeLevel;
    std::unordered_map<std::string, std::function<LevelPtr()>> m_levelFactories;

    sf::Clock m_frameClock;

    // Minimal text HUD when in menu
    sf::Font m_font;
    sf::Text m_menuText;

    void processMenuEvents();
    void renderMenu();
};