#ifndef AUDIO_EMITTER_H
#define AUDIO_EMITTER_H


#include <SFML/Audio.hpp>
#include <box2d/box2d.h>
#include <memory>
#include <string>
#include <iostream>


// Enums shared by emitters and manager
enum class AudioCategory { Music, Background, Dialogue, Effects };


struct AudioEmitter {
	std::string id;
	AudioCategory category = AudioCategory::Effects;
	b2Vec2 position = { 0.f, 0.f }; // Box2D meters
	float minDistance = 0.5f; // meters
	float maxDistance = 10.f; // meters
	float baseVolume = 1.f; // 0..1
	float currentGain = 0.f; // 0..1
	sf::Sound sound;
	std::shared_ptr<sf::SoundBuffer> buffer;


	AudioEmitter() = default;


	bool loadBuffer(const std::string& path) {
		buffer = std::make_shared<sf::SoundBuffer>();
		if (!buffer->loadFromFile(path)) {
			std::cerr << "Failed to load sound buffer: " << path << std::endl;
			buffer.reset();
			return false;
		}
		sound.setBuffer(*buffer);
		return true;
	}


	void play() {
		if (buffer) sound.play();
	}
	void stop() { sound.stop(); }


	void applyVolume(float finalVolume) {
		float v = std::clamp(finalVolume * 100.f, 0.f, 100.f);
		sound.setVolume(v);
	}
};


#endif
