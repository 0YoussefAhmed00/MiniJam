#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "AudioEmitter.h"
#include <SFML/Audio.hpp>
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

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

    // explicit music playback control
    void StartMusic();   // start current track (neutral by default)
    void StopMusic();    // stop both tracks
    void PauseMusic();   // pause both tracks
    void ResumeMusic();  // resume current track

    // emitter control
    void RegisterEmitter(std::shared_ptr<AudioEmitter> e);
    void UnregisterEmitter(const std::string& id);

    // music crossfade control (driven externally, e.g., by Player state)
    void CrossfadeToNeutral();
    void CrossfadeToCrazy();

    // volume setters
    void SetMasterVolume(float v);
    void SetMusicVolume(float v);
    void SetBackgroundVolume(float v);
    void SetDialogueVolume(float v);
    void SetEffectsVolume(float v);

    // volume getters (0..1)
    float GetMasterVolume() const { return masterVolume; }
    float GetMusicVolume() const { return musicVolume; }
    float GetBackgroundVolume() const { return backgroundVolume; }
    float GetDialogueVolume() const { return dialogueVolume; }
    float GetEffectsVolume() const { return effectsVolume; }

    void SetCrossfadeTime(float t);

    void Update(float dt, const b2Vec2& listenerPos);
    void PrintVolumes();

    AudioSettings settings;

private:
    // music
    sf::Music neutralMusic;
    sf::Music crazyMusic;

    enum class MusicTrack { Neutral, Crazy };
    MusicTrack m_currentTrack = MusicTrack::Neutral;
    MusicTrack m_targetTrack = MusicTrack::Neutral;

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

    sf::Music* musicForTrack(MusicTrack t);
    void StartCrossfade(MusicTrack target);
    void updateCrossfade(float dt);

    float ComputeGain(float distanceMeters, float minD, float maxD);
    float GetCategoryMultiplier(AudioCategory cat) const;
    void applyMusicVolumes();
};

#endif 