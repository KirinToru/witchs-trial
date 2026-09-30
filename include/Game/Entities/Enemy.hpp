#pragma once

#include <Engine/Graphics/Animator.hpp>
#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/RigidBody.hpp>
#include <Game/Combat/CombatBoxes.hpp>
#include <Game/Entities/EnemyState.hpp>
#include <Game/Entities/EnemyStates.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <string_view>

class Player;

namespace Physics {
class PhysicsWorld;
}

class Enemy {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    virtual ~Enemy();
//-------------------------------------------------------

//------------[Fixed Update - Step AI FSM and Kinematic Physics (60Hz)]-------------------
    virtual void fixedUpdate(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Render - Draw Enemy Visuals, Telegraph Indicators, Overhead Gauges & Debug Boxes]-------------------
    virtual void render(sf::RenderWindow& window, bool showHitbox = false);
//-------------------------------------------------------

//------------[Change State - Transition to Specified AI State]-------------------
    void changeState(EnemyStateType newType);
//-------------------------------------------------------

//------------[Get State Name - Return Human-Readable State String]-------------------
    std::string_view getStateName() const;
//-------------------------------------------------------

//------------[Get Current State Type - Return State Type Enum]-------------------
    EnemyStateType getCurrentStateType() const;
//-------------------------------------------------------

//------------[Take Damage - Apply Damage, Poise Damage, and Knockback]-------------------
    virtual bool takeDamage(float damage, float poiseDamage, sf::Vector2f knockback);
//-------------------------------------------------------

//------------[Has Line Of Sight - Perform Swept Raycast to Player]-------------------
    bool hasLineOfSightToPlayer(const Player& player, const Physics::PhysicsWorld& physicsWorld) const;
//-------------------------------------------------------

//------------[Get Distance To Player - Calculate Center-to-Center Distance]-------------------
    float getDistanceToPlayer(const Player& player) const;
//-------------------------------------------------------

//------------[Get Direction To Player - Return Horizontal Unit Direction (-1 or 1)]-------------------
    float getDirectionToPlayer(const Player& player) const;
//-------------------------------------------------------

//------------[Move With Swept CCD - Step Kinematic Movement with Continuous Collision Detection]-------------------
    void moveWithSweptCCD(sf::Vector2f displacement, const Physics::PhysicsWorld& physicsWorld, const Physics::RigidBody* ignoreBody = nullptr);
//-------------------------------------------------------

//------------[Get Position - Query World Space Position]-------------------
    sf::Vector2f getPosition() const;
//-------------------------------------------------------

//------------[Set Position - Update World Position and Hitboxes]-------------------
    void setPosition(sf::Vector2f pos);
//-------------------------------------------------------

//------------[Get Velocity - Query Current Velocity Vector]-------------------
    sf::Vector2f getVelocity() const;
//-------------------------------------------------------

//------------[Set Velocity - Update Current Velocity Vector]-------------------
    void setVelocity(sf::Vector2f vel);
//-------------------------------------------------------

//------------[Get Size - Query Bounding Box Size]-------------------
    sf::Vector2f getSize() const;
//-------------------------------------------------------

//------------[Get AABB - Query Physics Collision AABB]-------------------
    Physics::AABB getAABB() const;
//-------------------------------------------------------

//------------[Is Facing Right - Query Facing Direction]-------------------
    bool isFacingRight() const;
//-------------------------------------------------------

//------------[Set Facing Right - Update Facing Direction]-------------------
    void setFacingRight(bool right);
//-------------------------------------------------------

//------------[Get Health - Query Current Health Points]-------------------
    float getHealth() const;
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Health Capacity]-------------------
    float getMaxHealth() const;
//-------------------------------------------------------

//------------[Get Posture - Query Current Posture / Poise Gauge]-------------------
    float getPosture() const;
//-------------------------------------------------------

//------------[Get Max Posture - Query Maximum Posture Capacity]-------------------
    float getMaxPosture() const;
//-------------------------------------------------------

//------------[Is Staggered - Query If Enemy Is In Stunned Posture Break]-------------------
    bool isStaggered() const;
//-------------------------------------------------------

//------------[Is Dead - Query If Enemy Is Dead]-------------------
    bool isDead() const;
//-------------------------------------------------------

//------------[Get Hurtbox - Access Defensive Hurtbox Const]-------------------
    const Combat::Hurtbox& getHurtbox() const;
//-------------------------------------------------------

//------------[Get Hurtbox Mutable - Access Defensive Hurtbox]-------------------
    Combat::Hurtbox& getHurtbox();
//-------------------------------------------------------

//------------[Get Attack Hitbox - Access Offensive Attack Hitbox Const]-------------------
    const Combat::Hitbox& getAttackHitbox() const;
//-------------------------------------------------------

//------------[Get Attack Hitbox Mutable - Access Offensive Attack Hitbox]-------------------
    Combat::Hitbox& getAttackHitbox();
//-------------------------------------------------------

//------------[Set Attack Hitbox - Activate Offensive Hitbox]-------------------
    void setAttackHitbox(const Combat::Hitbox& hitbox);
//-------------------------------------------------------

//------------[Deactivate Attack Hitbox - Disable Offensive Hitbox]-------------------
    void deactivateAttackHitbox();
//-------------------------------------------------------

//------------[Get Move Speed - Query Base Movement Speed]-------------------
    float getMoveSpeed() const;
//-------------------------------------------------------

//------------[Get Attack Range - Query Distance Threshold For Initiating Attacks]-------------------
    float getAttackRange() const;
//-------------------------------------------------------

//------------[Get Detection Range - Query Vision Radius For Target Acquisition]-------------------
    float getDetectionRange() const;
//-------------------------------------------------------

//------------[Get Telegraph Duration - Query Windup Time in Seconds]-------------------
    float getTelegraphDuration() const;
//-------------------------------------------------------

//------------[Get Telegraph Timer - Query Current Telegraph Progress Timer]-------------------
    float getTelegraphTimer() const;
//-------------------------------------------------------

//------------[Set Telegraph Timer - Update Telegraph Progress Timer]-------------------
    void setTelegraphTimer(float t);
//-------------------------------------------------------

//------------[Get Attack Cooldown Timer - Query Attack Cooldown Remaining]-------------------
    float getAttackCooldownTimer() const;
//-------------------------------------------------------

//------------[Set Attack Cooldown Timer - Update Attack Cooldown Remaining]-------------------
    void setAttackCooldownTimer(float t);
//-------------------------------------------------------

//------------[Get Stagger Duration - Query Stun Duration Upon Posture Break]-------------------
    float getStaggerDuration() const;
//-------------------------------------------------------

//------------[Get Stagger Timer - Query Stagger Elapsed Time]-------------------
    float getStaggerTimer() const;
//-------------------------------------------------------

//------------[Set Stagger Timer - Update Stagger Elapsed Time]-------------------
    void setStaggerTimer(float t);
//-------------------------------------------------------

//------------[Reset Posture - Restore Posture Meter To Max]-------------------
    void resetPosture();
//-------------------------------------------------------

//------------[Spawn Attack Hitbox - Build and Activate Concrete Enemy Hitbox]-------------------
    virtual void spawnAttackHitbox() = 0;
//-------------------------------------------------------

//------------[Get Animator - Access Enemy Animator]-------------------
    Engine::Graphics::Animator& getAnimator();
//-------------------------------------------------------

//------------[Get Animator Const - Access Enemy Animator Const]-------------------
    const Engine::Graphics::Animator& getAnimator() const;
//-------------------------------------------------------

protected:
//------------[Protected Constructor - Initialize Enemy Base Properties]-------------------
    Enemy(sf::Vector2f size, float maxHealth, float maxPosture, float moveSpeed,
          float attackRange, float detectionRange, float telegraphDuration, float staggerDuration);
//-------------------------------------------------------

    sf::RectangleShape mShape;
    sf::Vector2f mVelocity{0.f, 0.f};
    bool mIsGrounded{false};
    bool mFacingRight{false};
    Engine::Graphics::Animator mAnimator;

    float mHealth{100.f};
    float mMaxHealth{100.f};
    float mPosture{50.f};
    float mMaxPosture{50.f};
    float mPostureRegenDelayTimer{0.f};
    float mPostureRegenRate{15.f};

    float mMoveSpeed{100.f};
    float mAttackRange{50.f};
    float mDetectionRange{380.f};
    float mTelegraphDuration{0.65f};
    float mTelegraphTimer{0.f};
    float mAttackCooldownTimer{0.f};
    float mStaggerDuration{2.5f};
    float mStaggerTimer{0.f};

    float mGravity{980.f};
    float mFriction{800.f};
    float mHitFlashTimer{0.f};

    Combat::Hurtbox mHurtbox;
    Combat::Hitbox mAttackHitbox;

    std::unique_ptr<EnemyIdleState> mIdleState;
    std::unique_ptr<EnemyChaseState> mChaseState;
    std::unique_ptr<EnemyTelegraphAttackState> mTelegraphAttackState;
    std::unique_ptr<EnemyActiveAttackState> mActiveAttackState;
    std::unique_ptr<EnemyStaggeredState> mStaggeredState;
    std::unique_ptr<EnemyDeadState> mDeadState;
    EnemyState* mCurrentState{nullptr};
};
