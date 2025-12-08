#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "AudioEmitter.h"
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

enum class GameState { Neutral, Crazy };
enum class DistanceModelEnum { Linear, InverseSquare, Logarithmic };

struct AudioSettings {
    DistanceModelEnum distanceModel = DistanceModelEnum::Linear;
    float smoothingTime = 0.08f; // seconds for emitter smoothing
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager() = default;

    // music loader
    bool loadMusic(const std::string& neutralPath, const std::string& crazyPath);

    // emitter control
    void RegisterEmitter(std::shared_ptr<AudioEmitter> e);
    void UnregisterEmitter(const std::string& id);

    // music state
    void SetMusicState(GameState s);

    // volume setters
    void SetMasterVolume(float v);
    void SetMusicVolume(float v);
    void SetBackgroundVolume(float v);
    void SetDialogueVolume(float v);
    void SetEffectsVolume(float v);

    void SetCrossfadeTime(float t);

    void Update(float dt, const b2Vec2& listenerPos);
    void PrintVolumes();

    AudioSettings settings;

private:
    // music
    sf::Music neutralMusic;
    sf::Music crazyMusic;
    GameState currentMusicState = GameState::Neutral;
    GameState musicStateTarget = GameState::Neutral;
    GameState musicState = GameState::Neutral;
    float crossfadeTime = 1.0f;
    float crossfadeTimer = 0.f;
    bool isCrossfading = false;

    // mixer volumes
    float masterVolume;
    float musicVolume;
    float backgroundVolume;
    float dialogueVolume;
    float effectsVolume;

    std::vector<std::shared_ptr<AudioEmitter>> emitters;

    sf::Music* musicForState(GameState s);
    void StartCrossfade(GameState target);
    void updateCrossfade(float dt);

    float ComputeGain(float distanceMeters, float minD, float maxD);
    float GetCategoryMultiplier(AudioCategory cat) const;
    void applyMusicVolumes();
};

#endif 