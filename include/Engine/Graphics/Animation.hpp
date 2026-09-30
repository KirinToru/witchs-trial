#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>

namespace Engine::Graphics {

class Animation {
public:
//------------[Default Constructor - Create Empty Animation]-------------------
    Animation() = default;
//-------------------------------------------------------

//------------[Constructor - Initialize Grid Row Animation]-------------------
    Animation(std::string name, int row, int startCol, int frameCount,
              sf::Vector2i frameSize, float frameDuration, bool looping = true)
        : mName(std::move(name)),
          mFrameDuration(frameDuration),
          mLooping(looping) {
        mFrames.reserve(frameCount);
        for (int i = 0; i < frameCount; ++i) {
            mFrames.emplace_back(
                sf::Vector2i{(startCol + i) * frameSize.x, row * frameSize.y},
                frameSize);
        }
    }
//-------------------------------------------------------

//------------[Constructor - Initialize Custom Rect Frames Animation]-------------------
    Animation(std::string name, std::vector<sf::IntRect> frames, float frameDuration, bool looping = true)
        : mName(std::move(name)),
          mFrames(std::move(frames)),
          mFrameDuration(frameDuration),
          mLooping(looping) {}
//-------------------------------------------------------

//------------[Constructor - Initialize Frame Count Virtual Animation]-------------------
    Animation(std::string name, int frameCount, float frameDuration, bool looping = true)
        : mName(std::move(name)),
          mFrameDuration(frameDuration),
          mLooping(looping),
          mVirtualFrameCount(frameCount) {}
//-------------------------------------------------------

//------------[Get Name - Return Animation Identifier]-------------------
    const std::string& getName() const { return mName; }
//-------------------------------------------------------

//------------[Get Frame Count - Query Total Number of Frames]-------------------
    std::size_t getFrameCount() const {
        return mFrames.empty() ? mVirtualFrameCount : mFrames.size();
    }
//-------------------------------------------------------

//------------[Get Frame Duration - Query Duration Per Frame in Seconds]-------------------
    float getFrameDuration() const { return mFrameDuration; }
//-------------------------------------------------------

//------------[Is Looping - Query Whether Animation Loops]-------------------
    bool isLooping() const { return mLooping; }
//-------------------------------------------------------

//------------[Get Frame Rect - Query Texture Rect for Given Frame Index]-------------------
    sf::IntRect getFrameRect(std::size_t index) const {
        if (mFrames.empty()) {
            return sf::IntRect({0, 0}, {0, 0});
        }
        if (index >= mFrames.size()) {
            return mFrames.back();
        }
        return mFrames[index];
    }
//-------------------------------------------------------

//------------[Get Frames - Access All Frame Rectangles]-------------------
    const std::vector<sf::IntRect>& getFrames() const { return mFrames; }
//-------------------------------------------------------

//------------[Get Total Duration - Query Full Animation Cycle Length]-------------------
    float getTotalDuration() const {
        return static_cast<float>(getFrameCount()) * mFrameDuration;
    }
//-------------------------------------------------------

private:
    std::string mName;
    std::vector<sf::IntRect> mFrames;
    float mFrameDuration{0.1f};
    bool mLooping{true};
    std::size_t mVirtualFrameCount{0};
};

} // namespace Engine::Graphics
