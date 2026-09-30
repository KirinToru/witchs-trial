#pragma once

#include <Engine/Physics/AABB.hpp>
#include <SFML/System/Vector2.hpp>

class ArenaTrigger {
public:
//------------[Constructor - Initialize Arena Trigger Bounds & Camera Anchor]-------------------
    ArenaTrigger(const Physics::AABB& triggerBounds, sf::Vector2f arenaCenter, sf::Vector2f arenaSize)
        : mBounds(triggerBounds),
          mArenaCenter(arenaCenter),
          mArenaSize(arenaSize),
          mTriggered(false),
          mCompleted(false) {}
//-------------------------------------------------------

//------------[Default Constructor]-------------------
    ArenaTrigger() = default;
//-------------------------------------------------------

//------------[Check - Test Overlap with Player AABB and Trigger Arena Lock]-------------------
    bool check(const Physics::AABB& playerAABB) {
        if (!mTriggered && !mCompleted && mBounds.intersects(playerAABB)) {
            mTriggered = true;
            return true;
        }
        return false;
    }
//-------------------------------------------------------

//------------[Is Triggered - Query If Arena Lock Is Active]-------------------
    bool isTriggered() const {
        return mTriggered;
    }
//-------------------------------------------------------

//------------[Set Triggered - Manually Toggle Trigger State]-------------------
    void setTriggered(bool triggered) {
        mTriggered = triggered;
    }
//-------------------------------------------------------

//------------[Is Completed - Query If Boss Encounter Was Cleared]-------------------
    bool isCompleted() const {
        return mCompleted;
    }
//-------------------------------------------------------

//------------[Set Completed - Mark Arena Cleared and Unlock Camera & Walls]-------------------
    void setCompleted(bool completed) {
        mCompleted = completed;
        if (completed) {
            mTriggered = false;
        }
    }
//-------------------------------------------------------

//------------[Reset - Revert Arena Trigger to Initial State]-------------------
    void reset() {
        mTriggered = false;
        mCompleted = false;
    }
//-------------------------------------------------------

//------------[Get Bounds - Query World Space AABB Trigger Volume]-------------------
    const Physics::AABB& getBounds() const {
        return mBounds;
    }
//-------------------------------------------------------

//------------[Get Arena Center - Query Fixed Camera Center Coordinate]-------------------
    sf::Vector2f getArenaCenter() const {
        return mArenaCenter;
    }
//-------------------------------------------------------

//------------[Get Arena Size - Query Arena View Dimensions]-------------------
    sf::Vector2f getArenaSize() const {
        return mArenaSize;
    }
//-------------------------------------------------------

private:
    Physics::AABB mBounds;
    sf::Vector2f mArenaCenter{0.f, 0.f};
    sf::Vector2f mArenaSize{1280.f, 720.f};
    bool mTriggered{false};
    bool mCompleted{false};
};
