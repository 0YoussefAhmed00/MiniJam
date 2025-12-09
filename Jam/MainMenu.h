#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>

class MenuButton {
public:
    MenuButton(const sf::Font& font, const std::string& label, const sf::Vector2f& center, const sf::Vector2f& size);

    void Draw(sf::RenderWindow& window);

    // Per-frame update (replaces separate hover/press-only calls).
    // Pass dt and current mouse position.
    void Update(float dt, const sf::Vector2f& mousePos);

    // Press animation API
    void TriggerPressAnim();

    // Optional: click callback
    void Click();
    void SetEnabled(bool enabled);

    bool Contains(const sf::Vector2f& p) const {
        return sf::FloatRect(
            m_baseCenter.x - m_baseSize.x * 0.5f,
            m_baseCenter.y - m_baseSize.y * 0.5f,
            m_baseSize.x,
            m_baseSize.y
        ).contains(p);
    }

    // Control persistent visuals from MainMenu
    void SetPersistentAccent(bool enabled) { m_persistentAccent = enabled; }
    void SetExternalScaleTarget(float t) { m_externalScaleTarget = t; }
    void RefreshLayout() { ApplyLayoutWithAnimation(); }

private:
    void UpdateHover(const sf::Vector2f& mousePos, float dt);
    void UpdatePress(float dt);
    void ApplyLayoutWithAnimation();

    sf::RectangleShape m_bg;
    sf::Text m_text;
    sf::RectangleShape m_accent; // colored rect

    sf::Vector2f m_baseCenter{};
    sf::Vector2f m_baseSize{};
    float m_accentBaseHeight{};

    bool m_enabled{ true };
    bool m_hovered{ false };
    float m_hoverAnim{ 0.f };
    float m_hoverSpeed{ 8.f };

    // Press animation state
    bool m_pressing{ false };
    float m_pressPulse{ 0.f };
    float m_pressDuration{ 1.0f }; // seconds

    std::function<void()> m_onClick;

    // Persistent accent state
    bool m_persistentAccent{ false };

    // External/global scale (used to make all buttons scale up smoothly)
    float m_externalScale{ 1.f };
    float m_externalScaleTarget{ 1.f };
    float m_externalScaleSpeed{ 8.f };

    // Accent animation param (0 = idle thin, 1 = full width)
    float m_accentAnim{ 0.f };
    float m_accentAnimSpeed{ 8.f };
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

    void ResetMobileVisual();

    // External actions
    std::function<void()> OnPlay;
    std::function<void()> OnOptions; // <-- NEW
    std::function<void()> OnExit;

    // Mobile visual press animation (existing)
    void TriggerMobilePressAnim();

private:
    sf::Vector2u m_windowSize;
    const sf::Font* m_font{ nullptr };

    sf::RectangleShape m_bgGradientTop;
    sf::RectangleShape m_bgGradientBottom;

    // Mobile visual
    bool m_mobileLoaded{ false };
    sf::Texture m_mobileTex1;
    sf::Texture m_mobileTex2;
    sf::Sprite m_mobileSprite;

    bool m_mobilePressed{ false };
    float m_mobilePulse{ 0.f };
    float m_mobilePressDuration{ 1.0f };
    std::function<void()> m_pendingAction{ nullptr };

    std::vector<MenuButton> m_buttons;
    sf::Vector2f m_mousePos{};
};
