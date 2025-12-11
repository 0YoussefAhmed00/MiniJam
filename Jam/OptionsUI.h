#pragma once
#include <SFML/Graphics.hpp>
#include "AudioManager.h"
#include "MainMenu.h"

namespace OptionsUI {
 void Show(AudioManager& audio, const sf::Font& font, MainMenu* menu);
}
