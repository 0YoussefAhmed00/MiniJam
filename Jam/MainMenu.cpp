#include "MainMenu.h"
#include <cmath>
#include <algorithm> // for std::min

static sf::Color ColorWithAlpha(sf::Color c, uint8_t a) { c.a = a; return c; }

// Persistent background assets for main menu
static sf::Texture s_mainBgTex;
static sf::Sprite s_mainBgSprite;
static bool s_mainBgLoaded = false;

// Fit a sprite to cover the given target size while preserving aspect ratio.
static void FitSpriteToSize(sf::Sprite& sprite, const sf::Vector2u& targetSize)
{
    const sf::Texture* tex = sprite.getTexture();
    if (!tex) return;
    const auto ts = tex->getSize();
    if (ts.x == 0 || ts.y == 0) return;

    // Compute scale to cover the entire window (like background cover)
    float scaleX = static_cast<float>(targetSize.x) / static_cast<float>(ts.x);
    float scaleY = static_cast<float>(targetSize.y) / static_cast<float>(ts.y);
    float scale = std::max(scaleX, scaleY);

    sprite.setScale(scale, scale);

    // Center it
    sprite.setOrigin(static_cast<float>(ts.x) * 0.5f, static_cast<float>(ts.y) * 0.5f);
    sprite.setPosition(static_cast<float>(targetSize.x) * 0.5f, static_cast<float>(targetSize.y) * 0.5f);
}

MenuButton::MenuButton(const sf::Font& font, const std::string& label, const sf::Vector2f& center, const sf::Vector2f& size)
    : m_baseCenter(center), m_baseSize(size)
{
    // Black button background
    m_bg.setSize(size);
    m_bg.setOrigin(size * 0.5f);
    m_bg.setPosition(center);
    m_bg.setFillColor(sf::Color(25, 25, 25));
    m_bg.setOutlineThickness(2.f);
    m_bg.setOutlineColor(sf::Color(60, 60, 60));

    // Label
    m_text.setFont(font);
    m_text.setString(label);
    m_text.setCharacterSize(28);
    m_text.setFillColor(sf::Color::White);
    sf::FloatRect tb = m_text.getLocalBounds();
    m_text.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    m_text.setPosition(center);

    // Colored accent bar (left side)
    m_accentBaseHeight = size.y - 8.f;
    m_accent.setSize({ 6.f, m_accentBaseHeight });
    m_accent.setOrigin({ 0.f, m_accentBaseHeight / 2.f }); // origin at left edge for easy width growth
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

void MenuButton::Update(float dt, const sf::Vector2f& mousePos)
{
    // 1) update hover state (doesn't call layout)
    UpdateHover(mousePos, dt);

    // 2) advance external/global scale toward its target smoothly
    m_externalScale += (m_externalScaleTarget - m_externalScale) * std::min(1.f, dt * m_externalScaleSpeed);

    // 3) determine desired accent target (pressed or persistent => full, otherwise idle)
    float desiredAccent = (m_pressing || m_persistentAccent) ? 1.f : 0.f;
    m_accentAnim += (desiredAccent - m_accentAnim) * std::min(1.f, dt * m_accentAnimSpeed);

    // 4) advance press pulse logic (only modifies press state; layout applied below)
    UpdatePress(dt);

    // 5) re-apply layout using the updated animation parameters
    ApplyLayoutWithAnimation();
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
    m_bg.setOutlineColor(outline);
}

void MenuButton::ApplyLayoutWithAnimation()
{
    // Hover expand for the whole button
    float expand = 4.f * m_hoverAnim;

    // Press scale on the whole button (a subtle pop)
    float pressT = m_pressing ? std::min(m_pressPulse / m_pressDuration, 1.f) : 0.f;
    float pressEase = std::sin(pressT * 3.1415926f); // smooth in/out
    float pressScale = 1.f + 0.08f * pressEase;

    // Apply external/global scale multiplier (smoothly driven externally)
    float totalScale = pressScale * m_externalScale;

    // Final background size/position
    sf::Vector2f expandedSize = { m_baseSize.x + expand, m_baseSize.y + expand };
    sf::Vector2f finalSize = { expandedSize.x * totalScale, expandedSize.y * totalScale };

    m_bg.setSize(finalSize);
    m_bg.setOrigin(finalSize * 0.5f);
    m_bg.setPosition(m_baseCenter);

    // ACCENT EFFECT:
    // We drive accent width by m_accentAnim (0..1) which animates smoothly to target
    float accentHeight = m_accentBaseHeight + 10.f * m_hoverAnim;
    m_accent.setOrigin({ 0.f, accentHeight * 0.5f }); // keep left-edge origin with current height

    float leftEdgeX = m_baseCenter.x - finalSize.x * 0.5f;
    float idleOffset = 10.f;

    float targetAccentWidthIdle = 6.f;
    float targetAccentWidthPressed = std::max(0.f, finalSize.x - idleOffset * 2.f);

    float accentWidth = targetAccentWidthIdle + (targetAccentWidthPressed - targetAccentWidthIdle) * m_accentAnim;

    float accentX = leftEdgeX + idleOffset;

    m_accent.setSize({ accentWidth, accentHeight });
    m_accent.setPosition(accentX, m_baseCenter.y);

    // Text: keep centered; during press, slightly tint for contrast
    m_text.setPosition(m_baseCenter);
    float textScale = 1.f + 0.04f * pressEase;
    // apply external scale to text as well (so it scales consistently)
    m_text.setScale(textScale * m_externalScale, textScale * m_externalScale);

    if (m_pressing) {
        m_text.setFillColor(sf::Color(240, 240, 255));
    }
    else {
        m_text.setFillColor(m_hovered ? sf::Color(230, 230, 255) : sf::Color::White);
    }
}

void MenuButton::TriggerPressAnim()
{
    m_pressing = true;
    m_pressPulse = 0.f;
}

void MenuButton::UpdatePress(float dt)
{
    if (!m_pressing) return;

    m_pressPulse += dt;

    // If press completed, stop the local press pulse
    if (m_pressPulse >= m_pressDuration) {
        m_pressing = false;
        m_pressPulse = 0.f;

        // If not persistent, accent will now animate back because m_accentAnim target becomes 0
    }
}

void MenuButton::Click()
{
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
    // white background
    m_bgGradientTop.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientTop.setPosition(0.f, 0.f);
    m_bgGradientTop.setFillColor(sf::Color::White);

    m_bgGradientBottom.setSize(sf::Vector2f(static_cast<float>(windowSize.x), windowSize.y * 0.5f));
    m_bgGradientBottom.setPosition(0.f, static_cast<float>(windowSize.y) * 0.5f);
    m_bgGradientBottom.setFillColor(sf::Color::White);

    // Load main background once
    if (!s_mainBgLoaded) {
        if (s_mainBgTex.loadFromFile("Assets/MainMenu/mainbg.jpg")) {
            s_mainBgTex.setSmooth(true);
            s_mainBgSprite.setTexture(s_mainBgTex, true);
            FitSpriteToSize(s_mainBgSprite, m_windowSize);
            s_mainBgLoaded = true;
        }
        else {
            s_mainBgLoaded = false;
        }
    }
    else {
        // Ensure it fits current window size if constructed with different dimensions
        FitSpriteToSize(s_mainBgSprite, m_windowSize);
    }
}

void MainMenu::SetFont(const sf::Font* font)
{
    m_font = font;
}

void MainMenu::BuildLayout()
{
    if (!m_font) return;

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

    // Ensure main background fits current window in case of resize
    if (s_mainBgLoaded) {
        FitSpriteToSize(s_mainBgSprite, m_windowSize);
    }

    m_buttons.clear();
    const sf::Vector2f btnSize(320.f, 64.f);
    const float cx = static_cast<float>(m_windowSize.x) * 0.49f;

    float startY = static_cast<float>(m_windowSize.y) * 0.5f;
    float gap = static_cast<float>(m_windowSize.y) * 0.10f;

    // Create three buttons: Play, Options, Exit
    MenuButton play(*m_font, "Play", { cx, startY }, btnSize);
    MenuButton options(*m_font, "Options", { cx, startY + gap }, btnSize); // NEW
    MenuButton exit(*m_font, "Exit", { cx, startY + gap * 2.f }, btnSize);

    m_buttons.emplace_back(play);     // index 0
    m_buttons.emplace_back(options);  // index 1
    m_buttons.emplace_back(exit);     // index 2
}

void MainMenu::TriggerMobilePressAnim()
{
    if (!m_mobileLoaded) return;
    m_mobilePressed = true;
    m_mobilePulse = 0.f;
    m_mobileSprite.setTexture(m_mobileTex2, true);
}

void MainMenu::Update(float dt, const sf::RenderWindow& /*window*/)
{
    // Update each button with the current mouse pos and dt (they will tween their own scale/accent)
    for (auto& b : m_buttons) {
        b.Update(dt, m_mousePos);
    }

    // Mobile visual scaling & completion
    if (m_mobilePressed) {
        m_mobilePulse += dt;
        float t = std::min(m_mobilePulse / m_mobilePressDuration, 1.f);
        float scale = 1.f + 0.10f * std::sin(t * 3.1415926f);
        m_mobileSprite.setScale(scale, scale);

        if (m_mobilePulse >= m_mobilePressDuration) {
            m_mobilePressed = false;
            m_mobileSprite.setScale(1.f, 1.f);

            if (m_pendingAction) {
                auto action = m_pendingAction;
                m_pendingAction = nullptr;
                action();
            }
        }
    }
}

void MainMenu::Render(sf::RenderWindow& window)
{
    // Draw the main background first (under mobile1/2 and buttons)
    if (s_mainBgLoaded) {
        window.draw(s_mainBgSprite);
    }
    else {
        // Fallback: white gradient background
        window.draw(m_bgGradientTop);
        window.draw(m_bgGradientBottom);
    }

    // Then draw the mobile illustration
    if (m_mobileLoaded) {
        window.draw(m_mobileSprite);
    }

    // Finally draw buttons
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
    // ignore clicks while the mobile press animation is running
    if (m_mobilePressed) return;

    m_mousePos = mousePos;

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        MenuButton& b = m_buttons[i];
        if (b.Contains(mousePos)) {
            // Local feedback
            b.Click();

            // Trigger pressed button animation (local pulse)
            b.TriggerPressAnim();

            // Make all buttons smoothly scale up to the pressed target
            // We'll tween every button's external scale toward 1.08
            constexpr float pressedGlobalScale = 1.08f;
            for (size_t j = 0; j < m_buttons.size(); ++j) {
                m_buttons[j].SetExternalScaleTarget(pressedGlobalScale);
                m_buttons[j].SetEnabled(false); // disable interaction while anim runs
            }

            // The pressed button should keep the accent filled until ResetMobileVisual()
            m_buttons[i].SetPersistentAccent(true);

            // Start mobile animation & pending action
            if (i == 0 && OnPlay) {
                m_pendingAction = OnPlay;
                TriggerMobilePressAnim();
            }
            else if (i == 1) { // Options
                if (OnOptions) m_pendingAction = OnOptions;
                else m_pendingAction = nullptr;
                TriggerMobilePressAnim();
            }
            else if (i == 2 && OnExit) {
                m_pendingAction = OnExit;
                TriggerMobilePressAnim();
            }

            break;
        }
    }
}

void MainMenu::ResetMobileVisual()
{
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

    // Reset visual state: re-enable buttons, reset external scale target to 1 and clear persistent accent
    for (auto& b : m_buttons) {
        b.SetEnabled(true);
        b.SetExternalScaleTarget(1.f);
        b.SetPersistentAccent(false);
        b.RefreshLayout();
    }
}