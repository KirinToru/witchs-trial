#pragma once

#include <Engine/Physics/AABB.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <algorithm>

namespace Physics {

enum class BodyType : uint8_t {
    Static,
    Kinematic,
    Dynamic
};

enum class ColliderTag : uint8_t {
    SolidWall,
    OneWayPlatform,
    Hazard,
    MothFloor = Hazard,
    Generic
};

struct RigidBodyDef {
    BodyType type = BodyType::Dynamic;
    ColliderTag tag = ColliderTag::Generic;
    bool isOneWay = false;
    sf::Vector2f position{0.f, 0.f};
    sf::Vector2f velocity{0.f, 0.f};
    AABB localAABB{sf::Vector2f(-16.f, -16.f), sf::Vector2f(16.f, 16.f)};
    float mass = 1.0f;
    float restitution = 0.2f;
    float friction = 0.4f;
    float gravityScale = 1.0f;
};

class RigidBody {
public:
//------------[Constructor - Initialize From Definition]-------------------
    explicit RigidBody(const RigidBodyDef& def = RigidBodyDef{})
        : mType(def.type),
          mTag(def.tag),
          mIsOneWay(def.isOneWay),
          mPosition(def.position),
          mVelocity(def.velocity),
          mForce(0.f, 0.f),
          mLocalAABB(def.localAABB),
          mMass(def.mass),
          mRestitution(def.restitution),
          mFriction(def.friction),
          mGravityScale(def.gravityScale) {
        updateInverseMass();
    }
//-------------------------------------------------------

//------------[Get Position - Query World Space Coordinates]-------------------
    sf::Vector2f getPosition() const {
        return mPosition;
    }
//-------------------------------------------------------

//------------[Set Position - Update World Space Coordinates]-------------------
    void setPosition(sf::Vector2f pos) {
        mPosition = pos;
    }
//-------------------------------------------------------

//------------[Get Velocity - Query Linear Velocity Vector]-------------------
    sf::Vector2f getVelocity() const {
        return mVelocity;
    }
//-------------------------------------------------------

//------------[Set Velocity - Update Linear Velocity Vector]-------------------
    void setVelocity(sf::Vector2f vel) {
        mVelocity = vel;
    }
//-------------------------------------------------------

//------------[Get Body Type - Query Motion Classification]-------------------
    BodyType getType() const {
        return mType;
    }
//-------------------------------------------------------

//------------[Set Body Type - Update Motion Classification]-------------------
    void setType(BodyType type) {
        mType = type;
        updateInverseMass();
    }
//-------------------------------------------------------

//------------[Get Collider Tag - Query Gameplay Classification]-------------------
    ColliderTag getTag() const {
        return mTag;
    }
//-------------------------------------------------------

//------------[Set Collider Tag - Update Gameplay Classification]-------------------
    void setTag(ColliderTag tag) {
        mTag = tag;
    }
//-------------------------------------------------------

//------------[Is One Way - Query Platform Pass-Through Property]-------------------
    bool isOneWay() const {
        return mIsOneWay;
    }
//-------------------------------------------------------

//------------[Set One Way - Toggle Platform Pass-Through Behavior]-------------------
    void setOneWay(bool oneWay) {
        mIsOneWay = oneWay;
    }
//-------------------------------------------------------

//------------[Get Mass - Query Mass Value]-------------------
    float getMass() const {
        return mMass;
    }
//-------------------------------------------------------

//------------[Set Mass - Update Mass and Recalculate Inverse]-------------------
    void setMass(float mass) {
        mMass = mass > 0.f ? mass : 0.f;
        updateInverseMass();
    }
//-------------------------------------------------------

//------------[Get Inverse Mass - Query 1/Mass Cached Value]-------------------
    float getInverseMass() const {
        return mInvMass;
    }
//-------------------------------------------------------

//------------[Set Inverse Mass - Directly Override Inverse Mass Parameter]-------------------
    void setInverseMass(float invMass) {
        mInvMass = invMass >= 0.f ? invMass : 0.f;
    }
//-------------------------------------------------------

//------------[Get Restitution - Query Coefficient of Restitution]-------------------
    float getRestitution() const {
        return mRestitution;
    }
//-------------------------------------------------------

//------------[Set Restitution - Update Bounciness Parameter]-------------------
    void setRestitution(float restitution) {
        mRestitution = std::clamp(restitution, 0.f, 1.f);
    }
//-------------------------------------------------------

//------------[Get Friction - Query Friction Coefficient]-------------------
    float getFriction() const {
        return mFriction;
    }
//-------------------------------------------------------

//------------[Set Friction - Update Surface Friction Parameter]-------------------
    void setFriction(float friction) {
        mFriction = std::max(0.f, friction);
    }
//-------------------------------------------------------

//------------[Get Gravity Scale - Query Gravity Multiplier]-------------------
    float getGravityScale() const {
        return mGravityScale;
    }
//-------------------------------------------------------

//------------[Set Gravity Scale - Update Gravity Multiplier]-------------------
    void setGravityScale(float scale) {
        mGravityScale = scale;
    }
//-------------------------------------------------------

//------------[Get Local AABB - Query Un-translated Bounding Box]-------------------
    AABB getLocalAABB() const {
        return mLocalAABB;
    }
//-------------------------------------------------------

//------------[Set Local AABB - Update Local Collider Geometry]-------------------
    void setLocalAABB(const AABB& aabb) {
        mLocalAABB = aabb;
    }
//-------------------------------------------------------

//------------[Get World AABB - Query Translated World-Space Bounds]-------------------
    AABB getWorldAABB() const {
        return mLocalAABB.translated(mPosition);
    }
//-------------------------------------------------------

//------------[Apply Force - Accumulate Continuous Force]-------------------
    void applyForce(sf::Vector2f force) {
        if (mType == BodyType::Dynamic) {
            mForce += force;
        }
    }
//-------------------------------------------------------

//------------[Apply Impulse - Modify Linear Velocity Instantly]-------------------
    void applyImpulse(sf::Vector2f impulse) {
        if (mType == BodyType::Dynamic) {
            mVelocity += mInvMass * impulse;
        }
    }
//-------------------------------------------------------

//------------[Integrate Forces - Integrate Acceleration into Velocity]-------------------
    void integrateForces(float dt, sf::Vector2f worldGravity) {
        if (mType != BodyType::Dynamic) return;

        sf::Vector2f effectiveGravity = worldGravity * mGravityScale;
        sf::Vector2f acceleration = effectiveGravity + (mForce * mInvMass);
        mVelocity += acceleration * dt;
        mForce = {0.f, 0.f};
    }
//-------------------------------------------------------

//------------[Integrate Velocity - Integrate Velocity into Position]-------------------
    void integrateVelocity(float dt) {
        if (mType == BodyType::Static) return;
        mPosition += mVelocity * dt;
    }
//-------------------------------------------------------

private:
//------------[Update Inverse Mass - Cache 1.0 / Mass Based on Body Type]-------------------
    void updateInverseMass() {
        if (mMass > 0.f && (mType == BodyType::Dynamic || mType == BodyType::Kinematic)) {
            mInvMass = 1.0f / mMass;
        } else {
            mInvMass = 0.0f;
        }
    }
//-------------------------------------------------------

    BodyType mType;
    ColliderTag mTag;
    bool mIsOneWay;

    sf::Vector2f mPosition;
    sf::Vector2f mVelocity;
    sf::Vector2f mForce;
    AABB mLocalAABB;

    float mMass;
    float mInvMass;
    float mRestitution;
    float mFriction;
    float mGravityScale;
};

} // namespace Physics
