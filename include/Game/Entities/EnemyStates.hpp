#pragma once

#include <Game/Entities/EnemyState.hpp>
#include <string_view>

//=============================================================================
// EnemyIdleState - Enemy Paused or Patrolling Until Player Detected
//=============================================================================
class EnemyIdleState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyIdleState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Idle State & Zero Velocity]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Cleanup Idle State]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Check Line Of Sight & Detection Range (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "Idle"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::Idle; }
//-------------------------------------------------------
};

//=============================================================================
// EnemyChaseState - Enemy Closes Distance Toward Player Kinematically
//=============================================================================
class EnemyChaseState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyChaseState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Chase Behavior]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Cleanup Chase State]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Pursue Player & Check Attack Range (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "Chase"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::Chase; }
//-------------------------------------------------------

private:
    float mLostLoSTimer{0.f};
};

//=============================================================================
// EnemyTelegraphAttackState - Souls-Like Attack Windup with Visual Warning
//=============================================================================
class EnemyTelegraphAttackState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyTelegraphAttackState() override = default;
//-------------------------------------------------------

//------------[Enter - Lock Facing Direction & Start Windup Timer]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Cleanup Telegraph Visuals]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Windup Progress & Trigger Attack (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "TelegraphAttack"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::TelegraphAttack; }
//-------------------------------------------------------
};

//=============================================================================
// EnemyActiveAttackState - Active Hitbox Window & Weapon Swing Recovery
//=============================================================================
class EnemyActiveAttackState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyActiveAttackState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Weapon Swing & Forward Momentum]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Deactivate Weapon Hitbox]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Hitbox Activation Window & Recovery (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "ActiveAttack"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::ActiveAttack; }
//-------------------------------------------------------

private:
    float mAttackTimer{0.f};
    bool mHitboxActivated{false};
    bool mHitboxDeactivated{false};
};

//=============================================================================
// EnemyStaggeredState - Posture Broken Stun State (Vulnerable to Finishers)
//=============================================================================
class EnemyStaggeredState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyStaggeredState() override = default;
//-------------------------------------------------------

//------------[Enter - Trigger Posture Break Stun & Disable Movement]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Restore Enemy Poise On Wakeup]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Count Down Stagger Stun Duration (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "Staggered"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::Staggered; }
//-------------------------------------------------------
};

//=============================================================================
// EnemyDeadState - Enemy Defeated / Corpse Handling
//=============================================================================
class EnemyDeadState : public EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    ~EnemyDeadState() override = default;
//-------------------------------------------------------

//------------[Enter - Deactivate Collision & Play Defeat Sequence]-------------------
    void enter(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Exit - Finalize Enemy Removal]-------------------
    void exit(Enemy& enemy) override;
//-------------------------------------------------------

//------------[Fixed Update - Settle Corpse On Ground (60Hz)]-------------------
    void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    std::string_view getName() const override { return "Dead"; }
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    EnemyStateType getType() const override { return EnemyStateType::Dead; }
//-------------------------------------------------------
};
