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

    crossfadeTime = 1.0f;
    crossfadeTimer = 0.f;
    isCrossfading = false;
    m_currentTrack = MusicTrack::Neutral;
    m_targetTrack = MusicTrack::Neutral;

    // ensure the new music member exists (no extra code required beyond declaration)
}

bool AudioManager::loadMusic(const std::string& neutralPath, const std::string& crazyPath, const std::string& winPath) {
    if (!neutralMusic.openFromFile(neutralPath)) {
        std::cerr << "Failed to open neutral music: " << neutralPath << std::endl;
        return false;
    }
    if (!crazyMusic.openFromFile(crazyPath)) {
        std::cerr << "Failed to open crazy music: " << crazyPath << std::endl;
        return false;
    }
    if (!winMusic.openFromFile(winPath)) {
        std::cerr << "Failed to open win music: " << winPath << std::endl;
        return false;
    }

    neutralMusic.setLoop(true);
    crazyMusic.setLoop(true);
    winMusic.setLoop(true);

    // Initialize volumes, but DO NOT start playback here
    neutralMusic.setVolume(masterVolume * musicVolume * 100.f);
    crazyMusic.setVolume(0.f);
    winMusic.setVolume(0.f);

    //// Debug: print durations & initial status
    //std::cerr << "Loaded music files: neutral(" << neutralPath << ") duration="
    //    << neutralMusic.getDuration().asSeconds()
    //    << "s, crazy(" << crazyPath << ") duration="
    //    << crazyMusic.getDuration().asSeconds()
    //    << "s, win(" << winPath << ") duration="
    //    << winMusic.getDuration().asSeconds() << "s\n";

    //std::cerr << "Music initial statuses: neutral=" << neutralMusic.getStatus()
    //    << " crazy=" << crazyMusic.getStatus()
    //    << " win=" << winMusic.getStatus() << "\n";

    return true;
}


void AudioManager::StartMusic() {
    sf::Music* cur = musicForTrack(m_currentTrack);
    if (cur->getStatus() != sf::Music::Playing) {
        cur->play();
        applyMusicVolumes();
    }
}


void AudioManager::StopMusic() {
    neutralMusic.stop();
    crazyMusic.stop();
    winMusic.stop();
    // stop background too
    backgroundMusic.stop();
    isCrossfading = false;
    crossfadeTimer = 0.f;
    m_targetTrack = m_currentTrack;
}

void AudioManager::PauseMusic() {
    if (neutralMusic.getStatus() == sf::Music::Playing) neutralMusic.pause();
    if (crazyMusic.getStatus() == sf::Music::Playing) crazyMusic.pause();
    if (winMusic.getStatus() == sf::Music::Playing) winMusic.pause();
    if (backgroundMusic.getStatus() == sf::Music::Playing) backgroundMusic.pause(); // <--- added
}

void AudioManager::ResumeMusic() {
    sf::Music* cur = musicForTrack(m_currentTrack);
    if (cur->getStatus() == sf::Music::Paused) {
        cur->play();
        applyMusicVolumes();
    }
    // resume background if paused
    if (backgroundMusic.getStatus() == sf::Music::Paused) {
        backgroundMusic.play();
        applyBackgroundVolume();
    }
}



void AudioManager::RegisterEmitter(std::shared_ptr<AudioEmitter> e) {
    emitters.push_back(e);
}

void AudioManager::UnregisterEmitter(const std::string& id) {
    emitters.erase(std::remove_if(emitters.begin(), emitters.end(),
        [&](const std::shared_ptr<AudioEmitter>& em) { return em->id == id; }), emitters.end());
}

void AudioManager::CrossfadeToNeutral() {
    if (isCrossfading && m_targetTrack == MusicTrack::Neutral) return;
    if (!isCrossfading && m_currentTrack == MusicTrack::Neutral) return;
    StartCrossfade(MusicTrack::Neutral);
}

void AudioManager::CrossfadeToCrazy() {
    if (isCrossfading && m_targetTrack == MusicTrack::Crazy) return;
    if (!isCrossfading && m_currentTrack == MusicTrack::Crazy) return;
    StartCrossfade(MusicTrack::Crazy);
}

void AudioManager::SetMasterVolume(float v) { masterVolume = std::clamp(v, 0.f, 1.f); applyMusicVolumes(); }
void AudioManager::SetMusicVolume(float v) { musicVolume = std::clamp(v, 0.f, 1.f); applyMusicVolumes(); }
void AudioManager::SetBackgroundVolume(float v) {
    backgroundVolume = std::clamp(v, 0.f, 1.f);
    applyBackgroundVolume();
}

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

sf::Music* AudioManager::musicForTrack(MusicTrack t) {
    if (t == MusicTrack::Neutral) return &neutralMusic;
    if (t == MusicTrack::Crazy) return &crazyMusic;
    return &winMusic; // MusicTrack::Win
}


void AudioManager::StartCrossfade(MusicTrack target) {
    sf::Music* targetMusic = musicForTrack(target);
    sf::Music* sourceMusic = musicForTrack(m_currentTrack);

    // Ensure both are playing for crossfade (or at least opened)
    if (sourceMusic->getStatus() != sf::Music::Playing) {
        sourceMusic->play();
        sourceMusic->setVolume(masterVolume * musicVolume * 100.f);
    }
    if (targetMusic->getStatus() != sf::Music::Playing) {
        targetMusic->play();
        targetMusic->setVolume(0.f);
    }

    // Special-case: when transitioning to Win, make it audible immediately.
    // This avoids the "zero-volume forever" if the crossfade timer doesn't advance.
    if (target == MusicTrack::Win) {
        //std::cerr << "[AudioManager] Immediate switch to WIN track\n";
        targetMusic->setVolume(masterVolume * musicVolume * 100.f);
        if (sourceMusic != targetMusic) {
            sourceMusic->stop();
        }
        isCrossfading = false;
        crossfadeTimer = 0.f;
        m_currentTrack = target;
        m_targetTrack = target;
        return;
    }

    // Normal crossfade path
    isCrossfading = true;
    crossfadeTimer = 0.f;
    m_targetTrack = target;

    /*std::cerr << "[AudioManager] StartCrossfade: from " << static_cast<int>(m_currentTrack)
        << " to " << static_cast<int>(m_targetTrack)
        << " crossfadeTime=" << crossfadeTime << "s\n";*/
}

void AudioManager::updateCrossfade(float dt) {
    if (!isCrossfading) return;

    crossfadeTimer += dt;
    float t = std::clamp(crossfadeTimer / crossfadeTime, 0.f, 1.f);
    float smoothT = t * t * (3.f - 2.f * t);

    sf::Music* targetMusic = musicForTrack(m_targetTrack);
    sf::Music* sourceMusic = musicForTrack(m_currentTrack);

    float fullVol = masterVolume * musicVolume * 100.f;
    float sourceVol = (1.f - smoothT) * fullVol;
    float targetVol = smoothT * fullVol;

    sourceMusic->setVolume(sourceVol);
    targetMusic->setVolume(targetVol);

    /*std::cerr << "[AudioManager] updateCrossfade dt=" << dt
        << " t=" << t << " smoothT=" << smoothT
        << " sourceVol=" << sourceVol << " targetVol=" << targetVol << "\n";*/

    if (t >= 1.f - 1e-6f) {
        isCrossfading = false;
        if (sourceMusic != targetMusic) sourceMusic->stop();
        m_currentTrack = m_targetTrack;
       // std::cerr << "[AudioManager] Crossfade finished. currentTrack=" << static_cast<int>(m_currentTrack) << "\n";
    }
}


void AudioManager::CrossfadeToWin() {
    if (isCrossfading && m_targetTrack == MusicTrack::Win) return;
    if (!isCrossfading && m_currentTrack == MusicTrack::Win) return;
    StartCrossfade(MusicTrack::Win);
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
        musicForTrack(m_currentTrack)->setVolume(masterVolume * musicVolume * 100.f);
    }
}

// Load an ambient/background music file (looping)
bool AudioManager::loadBackground(const std::string& path) {
    if (!backgroundMusic.openFromFile(path)) {
        //std::cerr << "Failed to open background music: " << path << std::endl;
        return false;
    }
    backgroundMusic.setLoop(true);
    // initialize at configured volume but do NOT auto-play
    backgroundMusic.setVolume(masterVolume * backgroundVolume * 100.f);
   // std::cerr << "Loaded background music: " << path
       // << " duration=" << backgroundMusic.getDuration().asSeconds() << "s\n";
    return true;
}

void AudioManager::StartBackground() {
    if (backgroundMusic.getStatus() != sf::Music::Playing) {
        backgroundMusic.play();
    }
    applyBackgroundVolume();
}

void AudioManager::StopBackground() {
    if (backgroundMusic.getStatus() != sf::Music::Stopped) {
        backgroundMusic.stop();
    }
}

// make sure background obeys the mixer sliders
void AudioManager::applyBackgroundVolume() {
    float vol = masterVolume * backgroundVolume * 100.f;
    backgroundMusic.setVolume(vol);
}
