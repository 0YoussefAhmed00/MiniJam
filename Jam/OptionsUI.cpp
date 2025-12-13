#include "OptionsUI.h"
#include <algorithm>

namespace OptionsUI {

	struct Slider {
		std::string label;
		float value;
		sf::Text labelText;
		sf::Text valueText;
		sf::RectangleShape bar;
		sf::RectangleShape fill;
		sf::CircleShape knob;
		float x, y, width, height, knobRadius;
		bool dragging{ false };
		std::function<void(float)> apply;
	};

	static void InitSlider(Slider& s, const std::string& label, float initial, float y, const sf::Font& font, const sf::VideoMode& desktop, std::function<void(float)> apply) {
		s.label = label;
		s.value = std::clamp(initial, 0.f, 1.f);
		s.x = desktop.width / 2 - 500;
		s.y = y;
		s.width = desktop.width - 1000.f;
		s.height = 10.f;
		s.knobRadius = 18.f;
		s.apply = apply;

		s.labelText = sf::Text(label, font, 42);
		s.labelText.setFillColor(sf::Color::White);
		s.labelText.setPosition(s.x, y - 60.f);

		s.valueText = sf::Text("", font, 32);
		s.valueText.setFillColor(sf::Color(200, 200, 200));
		s.valueText.setPosition((desktop.width / 2) - 570, y - 20.f);

		s.bar = sf::RectangleShape(sf::Vector2f(s.width, s.height));
		s.bar.setPosition(s.x, s.y);
		s.bar.setFillColor(sf::Color(100, 100, 140));

		s.fill = sf::RectangleShape(sf::Vector2f(s.width * s.value, s.height));
		s.fill.setPosition(s.x, s.y);
		s.fill.setFillColor(sf::Color(120, 180, 255));

		s.knob = sf::CircleShape(s.knobRadius);
		s.knob.setFillColor(sf::Color(240, 240, 240));
		s.knob.setOutlineThickness(2.f);
		s.knob.setOutlineColor(sf::Color(80, 80, 120));
		float cx = s.x + s.width * s.value;
		s.knob.setPosition(cx - s.knobRadius, s.y + s.height / 2.f - s.knobRadius);
	}

	static void UpdateSliderUI(Slider& sl, float v) {
		sl.value = std::clamp(v, 0.f, 1.f);
		if (sl.apply) sl.apply(sl.value);
		sl.fill.setSize(sf::Vector2f(sl.width * sl.value, sl.height));
		float cx2 = sl.x + sl.width * sl.value;
		sl.knob.setPosition(cx2 - sl.knobRadius, sl.y + sl.height / 2.f - sl.knobRadius);
		int percent = static_cast<int>(std::round(sl.value * 100.f));
		sl.valueText.setString(std::to_string(percent) + "%");
	}

	void Show(AudioManager& audio, const sf::Font& font, MainMenu* menu) {
		const auto desktop = sf::VideoMode::getDesktopMode();
		sf::RenderWindow opts(desktop, "Options", sf::Style::Fullscreen);
		opts.setFramerateLimit(60);

		sf::Texture controlsTex;
		controlsTex.loadFromFile("Assets/MainMenu/Controls.png");
		sf::Sprite controlsSprite(controlsTex);
		float maxWidth = desktop.width * 0.5f;
		float scale = maxWidth / controlsTex.getSize().x;
		controlsSprite.setScale(scale, scale);
		controlsSprite.setPosition(
			desktop.width * 0.5f - controlsSprite.getGlobalBounds().width * 0.5f,
			desktop.height * 0.1f
		);

		sf::Text moveText("Movement Controls", font, 48);
		moveText.setFillColor(sf::Color::White);
		sf::FloatRect imgBounds = controlsSprite.getGlobalBounds();
		moveText.setPosition(
			imgBounds.left + imgBounds.width * 0.5f - moveText.getGlobalBounds().width * 0.5f,
			imgBounds.top - moveText.getGlobalBounds().height - 10.f
		);

		// Hint text under the Movement Controls heading
		sf::Text hintText("Press Shift to walk quietly", font, 28);
		hintText.setFillColor(sf::Color(200, 200, 200));
		// center under the heading
		hintText.setPosition(
			imgBounds.left + imgBounds.width * 0.5f - hintText.getGlobalBounds().width * 0.5f,
			moveText.getPosition().y + moveText.getGlobalBounds().height + 20.f
		);

		std::vector<Slider> sliders;
		sliders.reserve(5);

		auto addSlider = [&](const std::string& label, float initial, float y, std::function<void(float)> apply) {
			Slider s; InitSlider(s, label, initial, y, font, desktop, apply); UpdateSliderUI(s, s.value); sliders.push_back(s);
			};

		// Initialize from current audio state instead of hardcoded defaults
		addSlider("Master", audio.GetMasterVolume(), desktop.height * 0.55f, [&](float v) { audio.SetMasterVolume(v); });
		addSlider("Music", audio.GetMusicVolume(), desktop.height * 0.62f, [&](float v) { audio.SetMusicVolume(v); });
		addSlider("Background", audio.GetBackgroundVolume(), desktop.height * 0.69f, [&](float v) { audio.SetBackgroundVolume(v); });
		addSlider("Dialogue", audio.GetDialogueVolume(), desktop.height * 0.76f, [&](float v) { audio.SetDialogueVolume(v); });
		addSlider("Effects", audio.GetEffectsVolume(), desktop.height * 0.83f, [&](float v) { audio.SetEffectsVolume(v); });

		bool exiting = false;
		sf::Clock exitClock;
		const float exitDuration = 0.35f;
		sf::RectangleShape fadeOverlay(sf::Vector2f((float)desktop.width, (float)desktop.height));
		fadeOverlay.setFillColor(sf::Color(0, 0, 0, 0));

		while (opts.isOpen()) {
			sf::Event ev;
			while (opts.pollEvent(ev)) {
				if (exiting) continue;
				if (ev.type == sf::Event::Closed || (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Escape)) {
					exiting = true;
					exitClock.restart();
				}
				auto handleMouseToSlider = [&](Slider& s, const sf::Vector2f& mp) {
					sf::FloatRect bounds(s.x, s.y - 20.f, s.width, s.height + 40.f);
					if (bounds.contains(mp)) {
						s.dragging = true;
						float newVal = (mp.x - s.x) / s.width;
						float clamped = std::clamp(newVal, 0.f, 1.f);
						UpdateSliderUI(s, clamped);
					}
					};
				if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
					sf::Vector2f mp((float)ev.mouseButton.x, (float)ev.mouseButton.y);
					for (auto& s : sliders) handleMouseToSlider(s, mp);
				}
				if (ev.type == sf::Event::MouseButtonReleased && ev.mouseButton.button == sf::Mouse::Left) {
					for (auto& s : sliders) s.dragging = false;
				}
				if (ev.type == sf::Event::MouseMoved) {
					sf::Vector2f mp((float)ev.mouseMove.x, (float)ev.mouseMove.y);
					for (auto& s : sliders) {
						if (s.dragging) {
							float newVal = (mp.x - s.x) / s.width;
							float clamped = std::clamp(newVal, 0.f, 1.f);
							UpdateSliderUI(s, clamped);
						}
					}
				}
			}
			if (exiting) {
				float t = std::min(exitClock.getElapsedTime().asSeconds() / exitDuration, 1.f);
				float ease = t * t * (3.f - 2.f * t);
				uint8_t alpha = (uint8_t)(255.f * ease);
				fadeOverlay.setFillColor(sf::Color(0, 0, 0, alpha));
				if (t >= 1.f) { opts.close(); break; }
			}
			opts.clear(sf::Color(30, 30, 40));
			opts.draw(moveText);
			opts.draw(hintText);
			opts.draw(controlsSprite);
			for (auto& s : sliders) {
				opts.draw(s.labelText);
				opts.draw(s.valueText);
				opts.draw(s.bar);
				opts.draw(s.fill);
				opts.draw(s.knob);
			}
			if (exiting) opts.draw(fadeOverlay);
			opts.display();
		}
		if (menu) menu->ResetMobileVisual();
	}

} // namespace OptionsUI
