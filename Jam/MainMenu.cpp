#include "MainMenu.h"
#include <cmath>

static sf::Color ColorWithAlpha(sf::Color c, uint8_t a) { c.a = a; return c; }

MenuButton::MenuButton(const sf::Font& font, const std::string& label, const sf::Vector2f& center, const sf::Vector2f& size)
    : m_baseCenter(center), m_baseSize(size)
{
    m_bg.setSize(size);
    m_bg.setOrigin(size * 0.5f);
    m_bg.setPosition(center);
    m_bg.setFillColor(sf::Color(25, 25, 25));
    m_bg.setOutlineThickness(2.f);
    m_bg.setOutlineColor(sf::Color(60, 60, 60));

    m_text.setFont(font);
    m_text.setString(label);
    m_text.setCharacterSize(28);
    m_text.setFillColor(sf::Color::White);
    sf::FloatRect tb = m_text.getLocalBounds();
    m_text.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    m_text.setPosition(center);

    // Accent bar on the left
    m_accentBaseHeight = size.y - 8.f;
    m_accent.setSize({ 6.f, m_accentBaseHeight });
    m_accent.setOrigin({ 3.f, m_accentBaseHeight / 2.f });
    m_accent.setPosition(center.x - size.x / 2.f + 10.f, center.y);
    m_accent.setFillColor(sf::Color(120, 70, 255));
}

void MenuButton::Draw(sf::RenderWindow& window)
{
    window.draw(m_bg);

    if (m_hoverAnim > 0.f) {
        sf::RectangleShape glow;
        glow.setSize(m_bg.getSize());
        glow.setOrigin(m_bg.getOrigin());
        glow.setPosition(m_bg.getPosition());
        glow.setFillColor(ColorWithAlpha(sf::Color(120, 70, 255), static_cast<uint8_t>(25 * m_hoverAnim)));
        window.draw(glow);
    }

    window.draw(m_accent);
    window.draw(m_text);
}

void MenuButton::UpdateHover(const sf::Vector2f& mousePos, float dt)
{
    m_hovered = m_enabled && sf::FloatRect(
        m_baseCenter.x - m_baseSize.x * 0.5f,
        m_baseCenter.y - m_baseSize.y * 0.5f,
        m_baseSize.x,
        m_baseSize.y
    ).contains(mousePos);

    const float target = m_hovered ? 1.f : 0.f;
    m_hoverAnim += (target - m_hoverAnim) * std::min(1.f, dt * m_hoverSpeed);

    // Outline color based on hover
    sf::Color outline = m_hovered ? sf::Color(150, 120, 255) : sf::Color(60, 60, 60);

    // Recompute background from base size/center
    float expand = 4.f * m_hoverAnim;
    sf::Vector2f expandedSize = { m_baseSize.x + expand, m_baseSize.y + expand };
    m_bg.setSize(expandedSize);
    m_bg.setOrigin(expandedSize * 0.5f);
    m_bg.setPosition(m_baseCenter);
    m_bg.setOutlineColor(outline);

    // Accent pulsate based on base height
    float accentHeight = m_accentBaseHeight + 10.f * m_hoverAnim;
    m_accent.setSize({ m_accent.getSize().x, accentHeight });
    m_accent.setOrigin({ m_accent.getSize().x * 0.5f, accentHeight * 0.5f });
    m_accent.setPosition(m_baseCenter.x - expandedSize.x / 2.f + 10.f, m_baseCenter.y);

    // Text color and recenter
    m_text.setFillColor(m_hovered ? sf::Color(230, 230, 255) : sf::Color::White);
    m_text.setPosition(m_baseCenter);
}

void MenuButton::Click()
{
    if (m_enabled && m_hovered && m_onClick) {
        m_onClick();
    }
}

void MenuButton::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    m_bg.setFillColor(enabled ? sf::Color(25, 25, 25) : sf::Color(60, 60, 60));
    m_text.setFillColor(enabled ? sf::Color::White : sf::Color(200, 200, 200));
}

MainMenu::MainMenu(const sf::Vector2u& windowSize)
    : m_windowSize(windowSize)
{
    m_bgGradientTop.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientTop.setPosition(0.f, 0.f);
    m_bgGradientTop.setFillColor(sf::Color(20, 20, 30));

    m_bgGradientBottom.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientBottom.setPosition(0.f, static_cast<float>(windowSize.y) * 0.5f);
    m_bgGradientBottom.setFillColor(sf::Color(10, 10, 15));

    m_neonLine.setSize({ static_cast<float>(windowSize.x), 3.f });
    m_neonLine.setOrigin({ m_neonLine.getSize().x / 2.f, 1.5f });
    m_neonLine.setPosition(windowSize.x / 2.f, windowSize.y * 0.25f);
    m_neonLine.setFillColor(sf::Color(120, 70, 255));
}

void MainMenu::SetFont(const sf::Font* font)
{
    m_font = font;
}

void MainMenu::BuildLayout()
{
    if (!m_font) return;

    m_title.setFont(*m_font);
    m_title.setString("MiniJam");
    m_title.setCharacterSize(64);
    m_title.setFillColor(sf::Color(220, 220, 255));
    sf::FloatRect tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    m_title.setPosition(static_cast<float>(m_windowSize.x) / 2.f, 140.f);

    m_buttons.clear();
    const sf::Vector2f btnSize(320.f, 64.f);
    const float cx = static_cast<float>(m_windowSize.x) / 2.f;
    float y = 280.f;
    const float gap = 90.f;

    MenuButton play(*m_font, "Play", { cx, y }, btnSize);
    MenuButton exit(*m_font, "Exit", { cx, y + gap }, btnSize);

    play.SetOnClick([this]() { if (OnPlay) OnPlay(); });
    exit.SetOnClick([this]() { if (OnExit) OnExit(); });

    m_buttons.emplace_back(play);
    m_buttons.emplace_back(exit);
}

void MainMenu::Update(float dt, const sf::RenderWindow& /*window*/)
{
    for (auto& b : m_buttons) {
        b.UpdateHover(m_mousePos, dt);
    }

    m_neonPhase += dt * 2.f;
    float alpha = 140.f + 60.f * std::sin(m_neonPhase);
    m_neonLine.setFillColor(sf::Color(120, 70, 255, static_cast<sf::Uint8>(std::clamp(alpha, 0.f, 255.f))));
}

void MainMenu::Render(sf::RenderWindow& window)
{
    window.draw(m_bgGradientTop);
    window.draw(m_bgGradientBottom);
    window.draw(m_title);
    window.draw(m_neonLine);
    for (auto& b : m_buttons) {
        b.Draw(window);
    }
}

void MainMenu::OnMouseMoved(const sf::Vector2f& mousePos)
{
    m_mousePos = mousePos;
}

void MainMenu::OnMousePressed(const sf::Vector2f& mousePos)
{
    m_mousePos = mousePos;
    for (auto& b : m_buttons) {
        if (b.Contains(mousePos)) {
            b.Click();
        }
    }
}