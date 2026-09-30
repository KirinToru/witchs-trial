#include <Engine/Audio/AudioManager.hpp>
#include <algorithm>
#include <iostream>

namespace Engine::Audio {

//------------[Get Instance - Singleton Access Point]-------------------
AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}
//-------------------------------------------------------

//------------[Constructor - Initialize Audio Subsystem and Pool]-------------------
AudioManager::AudioManager() {
    mSoundPool.reserve(mMaxPoolSize);
}
//-------------------------------------------------------

//------------[Destructor - Stop Sounds and Clean Up Stream Resources]-------------------
AudioManager::~AudioManager() {
    stopAllSounds();
    stopBGM();
}
//-------------------------------------------------------

//------------[Get Or Load Buffer - Fetch SoundBuffer from Cache or Disk]-------------------
const sf::SoundBuffer* AudioManager::getOrLoadBuffer(const std::string& filepath) {
    auto it = mSoundBuffers.find(filepath);
    if (it != mSoundBuffers.end()) {
        return &it->second;
    }

    if (mMissingBuffers.contains(filepath)) {
        return nullptr;
    }

    if (!std::filesystem::exists(filepath)) {
        mMissingBuffers.insert(filepath);
        return nullptr;
    }

    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(filepath)) {
        auto [insertedIt, success] = mSoundBuffers.emplace(filepath, std::move(buffer));
        return &insertedIt->second;
    }

    mMissingBuffers.insert(filepath);
    return nullptr;
}
//-------------------------------------------------------

//------------[Play Sound - Trigger Sound Effect from Cache or Disk]-------------------
void AudioManager::playSound(const std::string& filepath, float volume, float pitch) {
    const sf::SoundBuffer* buffer = getOrLoadBuffer(filepath);
    if (!buffer) {
        return;
    }

    float finalVolume = std::clamp(volume * (mSFXVolume / 100.f) * (mMasterVolume / 100.f), 0.f, 100.f);

    for (auto& sound : mSoundPool) {
        if (sound && sound->getStatus() == sf::SoundSource::Status::Stopped) {
            sound->setBuffer(*buffer);
            sound->setVolume(finalVolume);
            sound->setPitch(pitch);
            sound->play();
            return;
        }
    }

    if (mSoundPool.size() < mMaxPoolSize) {
        auto newSound = std::make_unique<sf::Sound>(*buffer);
        newSound->setVolume(finalVolume);
        newSound->setPitch(pitch);
        newSound->play();
        mSoundPool.push_back(std::move(newSound));
        return;
    }

    if (!mSoundPool.empty()) {
        auto& recycledSound = mSoundPool[mNextPoolIndex];
        mNextPoolIndex = (mNextPoolIndex + 1) % mSoundPool.size();
        recycledSound->stop();
        recycledSound->setBuffer(*buffer);
        recycledSound->setVolume(finalVolume);
        recycledSound->setPitch(pitch);
        recycledSound->play();
    }
}
//-------------------------------------------------------

//------------[Play BGM - Stream Background Music Track]-------------------
void AudioManager::playBGM(const std::string& filepath, bool loop, float volume) {
    mTargetBGMVolume = volume;

    if (mCurrentBGMPath == filepath && mMusic.getStatus() == sf::SoundSource::Status::Playing) {
        return;
    }

    mMusic.stop();
    mCurrentBGMPath = filepath;

    if (!std::filesystem::exists(filepath)) {
        return;
    }

    if (mMusic.openFromFile(filepath)) {
        mMusic.setLooping(loop);
        mMusic.setVolume(std::clamp(mTargetBGMVolume * (mBGMVolume / 100.f) * (mMasterVolume / 100.f), 0.f, 100.f));
        mMusic.play();
    }
}
//-------------------------------------------------------

//------------[Stop BGM - Halt Background Music Stream]-------------------
void AudioManager::stopBGM() {
    mMusic.stop();
    mCurrentBGMPath.clear();
}
//-------------------------------------------------------

//------------[Pause BGM - Suspend Background Music Stream]-------------------
void AudioManager::pauseBGM() {
    if (mMusic.getStatus() == sf::SoundSource::Status::Playing) {
        mMusic.pause();
    }
}
//-------------------------------------------------------

//------------[Resume BGM - Unpause Background Music Stream]-------------------
void AudioManager::resumeBGM() {
    if (mMusic.getStatus() == sf::SoundSource::Status::Paused) {
        mMusic.play();
    }
}
//-------------------------------------------------------

//------------[Stop All Sounds - Halt All Active Playing SFX in Pool]-------------------
void AudioManager::stopAllSounds() {
    for (auto& sound : mSoundPool) {
        if (sound) {
            sound->stop();
        }
    }
}
//-------------------------------------------------------

//------------[Set Master Volume - Global Volume Multiplier (0 to 100)]-------------------
void AudioManager::setMasterVolume(float volume) {
    mMasterVolume = std::clamp(volume, 0.f, 100.f);
    mMusic.setVolume(std::clamp(mTargetBGMVolume * (mBGMVolume / 100.f) * (mMasterVolume / 100.f), 0.f, 100.f));
}
//-------------------------------------------------------

//------------[Set SFX Volume - Sound Effects Multiplier (0 to 100)]-------------------
void AudioManager::setSFXVolume(float volume) {
    mSFXVolume = std::clamp(volume, 0.f, 100.f);
}
//-------------------------------------------------------

//------------[Set BGM Volume - Music Track Multiplier (0 to 100)]-------------------
void AudioManager::setBGMVolume(float volume) {
    mBGMVolume = std::clamp(volume, 0.f, 100.f);
    mMusic.setVolume(std::clamp(mTargetBGMVolume * (mBGMVolume / 100.f) * (mMasterVolume / 100.f), 0.f, 100.f));
}
//-------------------------------------------------------

//------------[Get Master Volume - Query Master Volume]-------------------
    float AudioManager::getMasterVolume() const {
    return mMasterVolume;
}
//-------------------------------------------------------

//------------[Get SFX Volume - Query SFX Volume]-------------------
float AudioManager::getSFXVolume() const {
    return mSFXVolume;
}
//-------------------------------------------------------

//------------[Get BGM Volume - Query BGM Volume]-------------------
float AudioManager::getBGMVolume() const {
    return mBGMVolume;
}
//-------------------------------------------------------

} // namespace Engine::Audio
