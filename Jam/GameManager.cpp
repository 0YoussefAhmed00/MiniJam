#include "GameManager.h"
#include <iostream>

GameManager::GameManager()
    : m_window(sf::VideoMode(1280, 720), "SFML + Box2D + AudioManager + Persona Demo"),
    m_defaultView(m_window.getDefaultView())
{
    m_window.setFramerateLimit(60);

    // Simple menu text (replace with your `MainMenu` later)
    if (!m_font.loadFromFile("assets/Font/Cairo-VariableFont_slnt,wght.ttf")) {
        std::cerr << "Warning: font not loaded (assets/Font/Cairo-VariableFont_slnt,wght.ttf)\n";
    }
    m_menuText.setFont(m_font);
    m_menuText.setCharacterSize(24);
    m_menuText.setFillColor(sf::Color::White);
    m_menuText.setPosition(40.f, 40.f);
    m_menuText.setString("Main Menu\n\nEnter: Play Level 1\nEsc: Quit");

    m_frameClock.restart();
}

void GameManager::showMenu() { m_inMenu = true; }
void GameManager::hideMenu() { m_inMenu = false; }

void GameManager::registerLevel(const std::string& id, std::function<LevelPtr()> factory) {
    m_levelFactories[id] = std::move(factory);
}

bool GameManager::switchLevel(const std::string& id) {
    auto it = m_levelFactories.find(id);
    if (it == m_levelFactories.end()) return false;

    // Exit current level
    if (m_activeLevel) {
        m_activeLevel->exit(*this);
        m_activeLevel.reset();
    }

    // Create and enter new level
    m_activeLevel = it->second();
    if (m_activeLevel) {
        hideMenu();
        m_activeLevel->enter(*this);
        return true;
    }
    return false;
}

int GameManager::run() {
    while (m_window.isOpen() && m_running) {
        float dt = m_frameClock.restart().asSeconds();

        if (m_inMenu) {
            processMenuEvents();
            renderMenu();
            continue;
        }

        if (!m_activeLevel) {
            // If no active level, send user back to menu
            showMenu();
            continue;
        }

        // Level loop
        m_activeLevel->processEvents(*this);
        m_activeLevel->update(*this, dt);
        m_activeLevel->render(*this);

        // Level completion check
        if (m_activeLevel->isComplete()) {
            std::string nextId = m_activeLevel->nextLevelId();
            if (!nextId.empty() && switchLevel(nextId)) {
                continue;
            }
            else {
                // No next level -> back to menu
                m_activeLevel->exit(*this);
                m_activeLevel.reset();
                showMenu();
            }
        }
    }
    return 0;
}

void GameManager::processMenuEvents() {
    sf::Event ev;
    while (m_window.pollEvent(ev)) {
        if (ev.type == sf::Event::Closed) {
            m_window.close();
        }
        if (ev.type == sf::Event::KeyPressed) {
            if (ev.key.code == sf::Keyboard::Escape) {
                m_window.close();
            }
            if (ev.key.code == sf::Keyboard::Enter) {
                // Start Level 1 by default
                switchLevel("Level1");
            }
        }
    }
}

void GameManager::renderMenu() {
    m_window.setView(m_defaultView);
    m_window.clear(sf::Color(20, 20, 30));
    m_window.draw(m_menuText);
    m_window.display();
}