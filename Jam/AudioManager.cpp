#include "AudioManager.h"
#include <iostream>

AudioManager::AudioManager() {
    settings.distanceModel = DistanceModelEnum::Linear;
    settings.smoothingTime = 0.08f;

    masterVolume = 1.f;
    musicVolume = 1.f;
    backgroundVolume = 1.f;
    dialogueVolume = 1.f;
    effectsVolume = 1.f;

    musicState = GameState::Neutral;
    crossfadeTime = 1.0f;
    crossfadeTimer = 0.f;
    isCrossfading = false;
}

bool AudioManager::loadMusic(const std::string& neutralPath, const std::string& crazyPath) {
    if (!neutralMusic.openFromFile(neutralPath)) {
        std::cerr << "Failed to open neutral music: " << neutralPath << std::endl;
        return false;
    }
    if (!crazyMusic.openFromFile(crazyPath)) {
        std::cerr << "Failed to open crazy music: " << crazyPath << std::endl;
        return false;
    }

    neutralMusic.setLoop(true);
    crazyMusic.setLoop(true);

    neutralMusic.setVolume(masterVolume * musicVolume * 100.f);
    crazyMusic.setVolume(0.f);

    neutralMusic.play();
    return true;
}

void AudioManager::RegisterEmitter(std::shared_ptr<AudioEmitter> e) {
    emitters.push_back(e);
}

void AudioManager::UnregisterEmitter(const std::string& id) {
    emitters.erase(std::remove_if(emitters.begin(), emitters.end(),
        [&](const std::shared_ptr<AudioEmitter>& em) { return em->id == id; }), emitters.end());
}

void AudioManager::SetMusicState(GameState s) {
    if (s == musicStateTarget && isCrossfading) return;
    if (s == currentMusicState && !isCrossfading) return;

    musicStateTarget = s;
    StartCrossfade(s);
}

void AudioManager::SetMasterVolume(float v) { masterVolume = std::clamp(v, 0.f, 1.f); applyMusicVolumes(); }
void AudioManager::SetMusicVolume(float v) { musicVolume = std::clamp(v, 0.f, 1.f); applyMusicVolumes(); }
void AudioManager::SetBackgroundVolume(float v) { backgroundVolume = std::clamp(v, 0.f, 1.f); }
void AudioManager::SetDialogueVolume(float v) { dialogueVolume = std::clamp(v, 0.f, 1.f); }
void AudioManager::SetEffectsVolume(float v) { effectsVolume = std::clamp(v, 0.f, 1.f); }

void AudioManager::SetCrossfadeTime(float t) { crossfadeTime = std::max(0.01f, t); }

void AudioManager::Update(float dt, const b2Vec2& listenerPos) {
    updateCrossfade(dt);

    for (auto& e : emitters) {
        float distance = (e->position - listenerPos).Length();
        float targetGain = ComputeGain(distance, e->minDistance, e->maxDistance);
        float alpha = 1.f - std::exp(-dt / std::max(0.0001f, settings.smoothingTime));
        e->currentGain = e->currentGain + (targetGain - e->currentGain) * alpha;

        float categoryMultiplier = GetCategoryMultiplier(e->category);
        float finalVolume = e->baseVolume * e->currentGain * categoryMultiplier * masterVolume;
        e->applyVolume(finalVolume);
    }
}

void AudioManager::PrintVolumes() {
    std::cout << "Master: " << masterVolume
        << " Music: " << musicVolume
        << " BG: " << backgroundVolume
        << " Dialogue: " << dialogueVolume
        << " Effects: " << effectsVolume << std::endl;
}

sf::Music* AudioManager::musicForState(GameState s) {
    return (s == GameState::Neutral) ? &neutralMusic : &crazyMusic;
}

void AudioManager::StartCrossfade(GameState target) {
    sf::Music* targetMusic = musicForState(target);
    sf::Music* sourceMusic = musicForState(currentMusicState);

    if (targetMusic->getStatus() != sf::Music::Playing)
        targetMusic->play();

    sourceMusic->setVolume(masterVolume * musicVolume * 100.f);
    targetMusic->setVolume(0.f);

    isCrossfading = true;
    crossfadeTimer = 0.f;
    musicStateTarget = target;
    musicState = target;
}

void AudioManager::updateCrossfade(float dt) {
    if (!isCrossfading) return;

    crossfadeTimer += dt;
    float t = std::clamp(crossfadeTimer / crossfadeTime, 0.f, 1.f);
    float smoothT = t * t * (3.f - 2.f * t);

    sf::Music* targetMusic = musicForState(musicStateTarget);
    sf::Music* sourceMusic = musicForState(currentMusicState);

    float sourceVol = (1.f - smoothT) * masterVolume * musicVolume * 100.f;
    float targetVol = (smoothT)*masterVolume * musicVolume * 100.f;

    sourceMusic->setVolume(sourceVol);
    targetMusic->setVolume(targetVol);

    if (t >= 1.f - 1e-6f) {
        isCrossfading = false;
        if (sourceMusic != targetMusic) sourceMusic->stop();
        currentMusicState = musicStateTarget;
    }
}

float AudioManager::ComputeGain(float distanceMeters, float minD, float maxD) {
    if (distanceMeters <= minD) return 1.f;
    if (distanceMeters >= maxD) return 0.f;
    return 1.f - (distanceMeters - minD) / (maxD - minD);
}

float AudioManager::GetCategoryMultiplier(AudioCategory cat) const {
    switch (cat) {
    case AudioCategory::Music: return musicVolume;
    case AudioCategory::Background: return backgroundVolume;
    case AudioCategory::Dialogue: return dialogueVolume;
    case AudioCategory::Effects: return effectsVolume;
    default: return 1.f;
    }
}

void AudioManager::applyMusicVolumes() {
    if (!isCrossfading) {
        musicForState(currentMusicState)->setVolume(masterVolume * musicVolume * 100.f);
    }
}