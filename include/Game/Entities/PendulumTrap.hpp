#pragma once

#include <Engine/Physics/AABB.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

class PendulumTrap {
public:
//------------[Constructor - Initialize Pendulum Trap Physics Parameters & Visuals]-------------------
    explicit PendulumTrap(sf::Vector2f pivot, float length = 140.f, float initialAngle = 1.05f);
//-------------------------------------------------------

//------------[Destructor - Default Virtual Cleanup]-------------------
    virtual ~PendulumTrap() = default;
//-------------------------------------------------------

    PendulumTrap(const PendulumTrap&) = delete;
    PendulumTrap& operator=(const PendulumTrap&) = delete;
    PendulumTrap(PendulumTrap&&) noexcept = default;
    PendulumTrap& operator=(PendulumTrap&&) noexcept = default;

//------------[Update - Step Euler Physical Pendulum Integration]-------------------
    void update(float dt);
//-------------------------------------------------------

//------------[Render - Draw Suspension Chain and Spiked Bob Sprite]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) const;
//-------------------------------------------------------

//------------[Get AABB - Calculate World Space Hazard Bounding Box]-------------------
    Physics::AABB getAABB() const;
//-------------------------------------------------------

//------------[Get Bob Position - Query Current Spiked Ball Center]-------------------
    sf::Vector2f getBobPosition() const { return mBobPosition; }
//-------------------------------------------------------

//------------[Get Bob Velocity - Query Current Linear Velocity Vector]-------------------
    sf::Vector2f getBobVelocity() const { return mBobVelocity; }
//-------------------------------------------------------

//------------[Get Pivot - Query Suspension Anchor Coordinate]-------------------
    sf::Vector2f getPivot() const { return mPivot; }
//-------------------------------------------------------

//------------[Get Damage - Query Hazard Contact Damage Value]-------------------
    float getDamage() const { return mDamage; }
//-------------------------------------------------------

private:
//------------[Ensure Shared Texture - Initialize Spiked Bob Sprite Texture]-------------------
    static void ensureSharedTexture();
//-------------------------------------------------------

    static sf::Texture sBobTexture;
    static bool sTextureInitialized;

    sf::Vector2f mPivot{0.f, 0.f};
    float mLength{140.f};
    float mAngle{1.05f};
    float mAngularVelocity{0.f};
    float mAngularAcceleration{0.f};
    float mGravity{980.f};
    float mDamping{0.005f};

    sf::Vector2f mBobPosition{0.f, 0.f};
    sf::Vector2f mBobVelocity{0.f, 0.f};
    float mBobRadius{18.f};
    float mDamage{25.f};

    sf::Sprite mBobSprite{sBobTexture};
};
