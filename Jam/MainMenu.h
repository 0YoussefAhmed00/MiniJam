#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include <string>

// A simple button with hover/click behavior
class MenuButton {
public:
    MenuButton(const sf::Font& font, const std::string& label, const sf::Vector2f& center, const sf::Vector2f& size);

    void Draw(sf::RenderWindow& window);
    void UpdateHover(const sf::Vector2f& mousePos, float dt);

    bool IsHovered() const { return m_hovered; }
    bool Contains(const sf::Vector2f& p) const { return m_bg.getGlobalBounds().contains(p); }

    void SetOnClick(std::function<void()> cb) { m_onClick = std::move(cb); }
    void Click();
    void SetEnabled(bool enabled);

private:
    // Base geometry (stable reference for animation)
    sf::Vector2f m_baseCenter;
    sf::Vector2f m_baseSize;

    sf::RectangleShape m_bg;
    sf::Text m_text;

    // Visual accent
    sf::RectangleShape m_accent;
    float m_accentBaseHeight = 0.f;

    // Hover animation
    bool m_hovered = false;
    float m_hoverAnim = 0.f; // 0..1
    float m_hoverSpeed = 6.f;

    std::function<void()> m_onClick;
    bool m_enabled = true;
};

class MainMenu {
public:
    explicit MainMenu(const sf::Vector2u& windowSize);

    void SetFont(const sf::Font* font);
    void BuildLayout();

    void Update(float dt, const sf::RenderWindow& window);
    void Render(sf::RenderWindow& window);

    void OnMouseMoved(const sf::Vector2f& mousePos);
    void OnMousePressed(const sf::Vector2f& mousePos);

    std::function<void()> OnPlay;
    std::function<void()> OnExit;

private:
    const sf::Font* m_font = nullptr;
    sf::Vector2u m_windowSize;

    // Title + subtle background gradient
    sf::Text m_title;
    sf::RectangleShape m_bgGradientTop;
    sf::RectangleShape m_bgGradientBottom;

    // Buttons
    std::vector<MenuButton> m_buttons;

    // Mouse position cache
    sf::Vector2f m_mousePos = { 0.f, 0.f };

    // Simple neon line animation
    sf::RectangleShape m_neonLine;
    float m_neonPhase = 0.f;
};

#endif // MAIN_MENU_H