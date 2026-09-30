#pragma once

#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <filesystem>

namespace Engine::Audio {

class AudioManager {
public:
//------------[Get Instance - Singleton Access Point]-------------------
    static AudioManager& getInstance();
//-------------------------------------------------------

//------------[Constructor - Initialize Audio Subsystem and Pool]-------------------
    AudioManager();
//-------------------------------------------------------

//------------[Destructor - Stop Sounds and Clean Up Stream Resources]-------------------
    ~AudioManager();
//-------------------------------------------------------

//------------[Play Sound - Trigger Sound Effect from Cache or Disk]-------------------
    void playSound(const std::string& filepath, float volume = 100.f, float pitch = 1.0f);
//-------------------------------------------------------

//------------[Play BGM - Stream Background Music Track]-------------------
    void playBGM(const std::string& filepath, bool loop = true, float volume = 50.f);
//-------------------------------------------------------

//------------[Stop BGM - Halt Background Music Stream]-------------------
    void stopBGM();
//-------------------------------------------------------

//------------[Pause BGM - Suspend Background Music Stream]-------------------
    void pauseBGM();
//-------------------------------------------------------

//------------[Resume BGM - Unpause Background Music Stream]-------------------
    void resumeBGM();
//-------------------------------------------------------

//------------[Stop All Sounds - Halt All Active Playing SFX in Pool]-------------------
    void stopAllSounds();
//-------------------------------------------------------

//------------[Set Master Volume - Global Volume Multiplier (0 to 100)]-------------------
    void setMasterVolume(float volume);
//-------------------------------------------------------

//------------[Set SFX Volume - Sound Effects Multiplier (0 to 100)]-------------------
    void setSFXVolume(float volume);
//-------------------------------------------------------

//------------[Set BGM Volume - Music Track Multiplier (0 to 100)]-------------------
    void setBGMVolume(float volume);
//-------------------------------------------------------

//------------[Get Master Volume - Query Master Volume]-------------------
    float getMasterVolume() const;
//-------------------------------------------------------

//------------[Get SFX Volume - Query SFX Volume]-------------------
    float getSFXVolume() const;
//-------------------------------------------------------

//------------[Get BGM Volume - Query BGM Volume]-------------------
    float getBGMVolume() const;
//-------------------------------------------------------

private:
//------------[Get Or Load Buffer - Fetch SoundBuffer from Cache or Disk]-------------------
    const sf::SoundBuffer* getOrLoadBuffer(const std::string& filepath);
//-------------------------------------------------------

    std::unordered_map<std::string, sf::SoundBuffer> mSoundBuffers;
    std::unordered_set<std::string> mMissingBuffers;
    std::vector<std::unique_ptr<sf::Sound>> mSoundPool;
    std::size_t mMaxPoolSize{32};
    std::size_t mNextPoolIndex{0};

    sf::Music mMusic;
    std::string mCurrentBGMPath;
    float mTargetBGMVolume{50.f};

    float mMasterVolume{100.f};
    float mSFXVolume{100.f};
    float mBGMVolume{50.f};
};

} // namespace Engine::Audio
