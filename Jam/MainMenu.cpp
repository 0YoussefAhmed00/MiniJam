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
    // local visual feedback (if any) can be placed here.
    // only call user callback if it's set — but in this Menu we will
    // not rely on m_onClick for delaying the Play/Exit actions.
    if (m_onClick) {
        m_onClick();
    }
}

void MenuButton::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    m_bg.setFillColor(enabled ? sf::Color(25, 25, 25) : sf::Color(60, 60, 60));
    m_text.setFillColor(enabled ? sf::Color::White : sf::Color(200, 200, 200));
}

/* ---------------- MainMenu ---------------- */

MainMenu::MainMenu(const sf::Vector2u& windowSize)
    : m_windowSize(windowSize)
{
    // white background (keeps it simple)
    m_bgGradientTop.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientTop.setPosition(0.f, 0.f);
    m_bgGradientTop.setFillColor(sf::Color::White);

    m_bgGradientBottom.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientBottom.setPosition(0.f, static_cast<float>(windowSize.y) * 0.5f);
    m_bgGradientBottom.setFillColor(sf::Color::White);

    // neon line intentionally not initialized/drawn per your request to remove it
}

void MainMenu::SetFont(const sf::Font* font)
{
    m_font = font;
}

void MainMenu::BuildLayout()
{
    if (!m_font) return;

    // Load mobile images once and center using their native size
    if (m_mobileTex1.loadFromFile("Assets/MainMenu/mobile1.png") &&
        m_mobileTex2.loadFromFile("Assets/MainMenu/mobile2.png"))
    {
        m_mobileLoaded = true;
        m_mobileTex1.setSmooth(true);
        m_mobileTex2.setSmooth(true);

        m_mobileSprite.setTexture(m_mobileTex1, true);
        const auto sz = m_mobileTex1.getSize();
        m_mobileSprite.setOrigin(static_cast<float>(sz.x) * 0.5f, static_cast<float>(sz.y) * 0.5f);
        m_mobileSprite.setPosition(static_cast<float>(m_windowSize.x) * 0.5f, static_cast<float>(m_windowSize.y) * 0.5f);
        m_mobileSprite.setScale(1.f, 1.f);
    }
    else {
        m_mobileLoaded = false;
    }

    // Title removed (no "MiniJam" word) — if you want the title back, set m_title here

    m_buttons.clear();
    const sf::Vector2f btnSize(320.f, 64.f);
    const float cx = static_cast<float>(m_windowSize.x) * 0.49f;

    // Position buttons relative to screen height
    float startY = static_cast<float>(m_windowSize.y) * 0.5f;   // move down depending on resolution
    float gap = static_cast<float>(m_windowSize.y) * 0.10f;      // gap scales automatically

    // Create buttons centered on fullscreen
    MenuButton play(*m_font, "Play", { cx, startY }, btnSize);
    MenuButton exit(*m_font, "Exit", { cx, startY + gap }, btnSize);


    // Do not attach play/exit callbacks to the buttons themselves.
    // We'll handle their actions from MainMenu::OnMousePressed so we can delay them.

    m_buttons.emplace_back(play);
    m_buttons.emplace_back(exit);
}

void MainMenu::TriggerMobilePressAnim()
{
    if (!m_mobileLoaded) return;

    // start the 1 second animation and immediately switch visual to mobile2
    m_mobilePressed = true;
    m_mobilePulse = 0.f;

    // show mobile2 texture while animation runs
    m_mobileSprite.setTexture(m_mobileTex2, true);
}

void MainMenu::Update(float dt, const sf::RenderWindow& /*window*/)
{
    // update hover animation for each button (even when pressing)
    for (auto& b : m_buttons) {
        b.UpdateHover(m_mousePos, dt);
    }

    // MOBILE PRESS ANIMATION: progress and call pending action after duration
    if (m_mobilePressed) {
        m_mobilePulse += dt;
        float t = std::min(m_mobilePulse / m_mobilePressDuration, 1.f);

        // small pop/scale effect for the duration
        float scale = 1.f + 0.10f * std::sin(t * 3.1415926f);
        m_mobileSprite.setScale(scale, scale);

        // optionally fade/other effects could be added here

        if (m_mobilePulse >= m_mobilePressDuration) {
            // animation finished
            m_mobilePressed = false;
            m_mobileSprite.setScale(1.f, 1.f);

            // run pending action (Play or Exit)
            if (m_pendingAction) {
                // store and clear before calling to avoid reentrancy issues
                auto action = m_pendingAction;
                m_pendingAction = nullptr;
                action();
            }
        }
    }

    // (If you had a neon animation, it would be updated here — removed earlier)
}

void MainMenu::Render(sf::RenderWindow& window)
{
    // draw white background
    window.draw(m_bgGradientTop);
    window.draw(m_bgGradientBottom);

    // Draw the mobile visual centered and at native size (behind buttons)
    if (m_mobileLoaded) {
        window.draw(m_mobileSprite);
    }

    // draw buttons
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
    // ignore clicks while the press animation is running
    if (m_mobilePressed) return;

    m_mousePos = mousePos;

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        MenuButton& b = m_buttons[i];
        if (b.Contains(mousePos)) {
            // visual/local click (if button has local handler)
            b.Click();

            // start 1 second animation BEFORE invoking the action
            // determine which button: index 0 => Play, index 1 => Exit
            if (i == 0 && OnPlay) {
                m_pendingAction = OnPlay;
                TriggerMobilePressAnim();
                // disable buttons while animation runs (prevent double clicks)
                for (auto& btn : m_buttons) btn.SetEnabled(false);
            }
            else if (i == 1 && OnExit) {
                m_pendingAction = OnExit;
                TriggerMobilePressAnim();
                for (auto& btn : m_buttons) btn.SetEnabled(false);
            }

            // handled the click; break so multiple buttons don't trigger
            break;
        }
    }
}

void MainMenu::ResetMobileVisual()
{
    // restore to mobile1 and reset animation state
    m_mobilePressed = false;
    m_mobilePulse = 0.f;
    m_pendingAction = nullptr;

    if (m_mobileLoaded) {
        m_mobileSprite.setTexture(m_mobileTex1, true);
        const auto sz1 = m_mobileTex1.getSize();
        m_mobileSprite.setOrigin(static_cast<float>(sz1.x) * 0.5f, static_cast<float>(sz1.y) * 0.5f);
        m_mobileSprite.setPosition(static_cast<float>(m_windowSize.x) * 0.5f, static_cast<float>(m_windowSize.y) * 0.5f);
        m_mobileSprite.setScale(1.f, 1.f);
    }

    // re-enable buttons
    for (auto& b : m_buttons) b.SetEnabled(true);
}
