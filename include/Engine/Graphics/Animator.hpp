#pragma once

#include <Engine/Graphics/Animation.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Engine::Graphics {

class Animator {
public:
//------------[Default Constructor - Initialize Idle Animator]-------------------
    Animator() = default;
//-------------------------------------------------------

//------------[Add Animation - Register New Animation In Library]-------------------
    void addAnimation(Animation animation) {
        std::string name = animation.getName();
        mAnimations.insert_or_assign(name, std::move(animation));
    }
//-------------------------------------------------------

//------------[Play - Start Playing Specified Animation]-------------------
    void play(const std::string& name, bool restartIfSame = false) {
        if (mCurrentAnimation && mCurrentAnimation->getName() == name && !restartIfSame) {
            return;
        }

        auto it = mAnimations.find(name);
        if (it != mAnimations.end()) {
            mCurrentAnimation = &it->second;
            mCurrentFrameIndex = 0;
            mTimer = 0.f;
            mIsPlaying = true;
            mHasFinished = false;
        }
    }
//-------------------------------------------------------

//------------[Stop - Halt Current Animation Playback]-------------------
    void stop() {
        mIsPlaying = false;
    }
//-------------------------------------------------------

//------------[Update - Advance Frame Timer and Step Animation Frames]-------------------
    void update(float dt, sf::Sprite* targetSprite = nullptr) {
        if (!mIsPlaying || !mCurrentAnimation) {
            return;
        }

        std::size_t count = mCurrentAnimation->getFrameCount();
        if (count == 0) {
            return;
        }

        mTimer += dt;
        float frameDur = mCurrentAnimation->getFrameDuration();
        if (frameDur > 0.f && mTimer >= frameDur) {
            std::size_t framesToAdvance = static_cast<std::size_t>(mTimer / frameDur);
            mTimer = std::fmod(mTimer, frameDur);

            mCurrentFrameIndex += framesToAdvance;
            if (mCurrentFrameIndex >= count) {
                if (mCurrentAnimation->isLooping()) {
                    mCurrentFrameIndex %= count;
                } else {
                    mCurrentFrameIndex = count - 1;
                    mHasFinished = true;
                    mIsPlaying = false;
                }
            }
        }

        if (targetSprite && !mCurrentAnimation->getFrames().empty()) {
            targetSprite->setTextureRect(mCurrentAnimation->getFrameRect(mCurrentFrameIndex));
        }
    }
//-------------------------------------------------------

//------------[Apply To Sprite - Synchronize Sprite Texture Rect with Current Frame]-------------------
    void applyToSprite(sf::Sprite& sprite) const {
        if (mCurrentAnimation && !mCurrentAnimation->getFrames().empty()) {
            sprite.setTextureRect(mCurrentAnimation->getFrameRect(mCurrentFrameIndex));
        }
    }
//-------------------------------------------------------

//------------[Get Current Frame - Query Active Frame Index]-------------------
    std::size_t getCurrentFrame() const {
        return mCurrentFrameIndex;
    }
//-------------------------------------------------------

//------------[Get Current Animation Name - Query Active Animation String]-------------------
    std::string_view getCurrentAnimationName() const {
        if (mCurrentAnimation) {
            return mCurrentAnimation->getName();
        }
        return "";
    }
//-------------------------------------------------------

//------------[Is Finished - Query Whether Non-Looping Animation Has Completed]-------------------
    bool isFinished() const {
        return mHasFinished;
    }
//-------------------------------------------------------

//------------[Is Playing - Query Active Playback State]-------------------
    bool isPlaying() const {
        return mIsPlaying;
    }
//-------------------------------------------------------

//------------[Has Animation - Check If Animation Name Exists]-------------------
    bool hasAnimation(const std::string& name) const {
        return mAnimations.contains(name);
    }
//-------------------------------------------------------

//------------[Get Progress - Return Normalized Animation Progress [0, 1]]-------------------
    float getProgress() const {
        if (!mCurrentAnimation || mCurrentAnimation->getFrameCount() == 0) {
            return 0.f;
        }
        std::size_t count = mCurrentAnimation->getFrameCount();
        float totalDur = mCurrentAnimation->getTotalDuration();
        if (totalDur <= 0.f) return 1.f;
        float elapsed = static_cast<float>(mCurrentFrameIndex) * mCurrentAnimation->getFrameDuration() + mTimer;
        return std::clamp(elapsed / totalDur, 0.f, 1.f);
    }
//-------------------------------------------------------

private:
    std::unordered_map<std::string, Animation> mAnimations;
    const Animation* mCurrentAnimation{nullptr};
    float mTimer{0.f};
    std::size_t mCurrentFrameIndex{0};
    bool mIsPlaying{false};
    bool mHasFinished{false};
};

} // namespace Engine::Graphics
