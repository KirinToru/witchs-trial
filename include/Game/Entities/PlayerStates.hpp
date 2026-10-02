#pragma once

#include <Game/Entities/PlayerState.hpp>
#include <SFML/System/Vector2.hpp>

class PlayerIdleState : public PlayerState {
public:
//------------[Virtual Destructor - Idle State Cleanup]-------------------
    ~PlayerIdleState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Idle State Transition]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Idle Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Idle Physics & Ground Check]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Idle State Name]-------------------
    std::string_view getName() const override { return "Idle"; }
//-------------------------------------------------------

//------------[Get Type - Return Idle State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Idle; }
//-------------------------------------------------------
};

class PlayerRunState : public PlayerState {
public:
//------------[Virtual Destructor - Run State Cleanup]-------------------
    ~PlayerRunState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Run State Transition]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Run Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Ground Acceleration and Kinematic Swept CCD]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Run State Name]-------------------
    std::string_view getName() const override { return "Run"; }
//-------------------------------------------------------

//------------[Get Type - Return Run State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Run; }
//-------------------------------------------------------
};

class PlayerAirborneState : public PlayerState {
public:
//------------[Virtual Destructor - Airborne State Cleanup]-------------------
    ~PlayerAirborneState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Airborne State Transition]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Aerial Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Airborne Physics, Gravity & Wall Slide]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Airborne State Name]-------------------
    std::string_view getName() const override { return "Airborne"; }
//-------------------------------------------------------

//------------[Get Type - Return Airborne State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Airborne; }
//-------------------------------------------------------
};

class PlayerDashState : public PlayerState {
public:
//------------[Virtual Destructor - Dash State Cleanup]-------------------
    ~PlayerDashState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Witch Air-Dash Burst & Freeze Timer]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Momentum]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Air-Dash Freeze, Burst & CCD Sweep]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Dash State Name]-------------------
    std::string_view getName() const override { return "Dash"; }
//-------------------------------------------------------

//------------[Get Type - Return Dash State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Dash; }
//-------------------------------------------------------
};

class PlayerPounceState : public PlayerState {
public:
//------------[Virtual Destructor - Pounce State Cleanup]-------------------
    ~PlayerPounceState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Beast Pounce or Ground Smash]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Beast Momentum]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Beast Pounce Velocity & Ground Smash Impact]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Pounce State Name]-------------------
    std::string_view getName() const override { return mIsGroundSmash ? "GroundSmash" : "Pounce"; }
//-------------------------------------------------------

//------------[Get Type - Return Pounce State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Pounce; }
//-------------------------------------------------------

//------------[Is Ground Smash - Query Smash Variant]-------------------
    bool isGroundSmash() const { return mIsGroundSmash; }
//-------------------------------------------------------

private:
    bool mIsGroundSmash{false};
    float mSmashRecoveryTimer{0.f};
    float mDurationTimer{0.f};
};

class PlayerMeleeAttackState : public PlayerState {
public:
//------------[Virtual Destructor - Melee Attack State Cleanup]-------------------
    ~PlayerMeleeAttackState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Witch Light Slash Attack]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Attack Hitbox]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Attack Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Attack Windup, Active Hitbox Window & Recovery]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Melee Attack State Name]-------------------
    std::string_view getName() const override { return "MeleeAttack"; }
//-------------------------------------------------------

//------------[Get Type - Return Melee Attack State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::MeleeAttack; }
//-------------------------------------------------------

private:
    float mAttackTimer{0.f};
    bool mHitboxActivated{false};
    bool mHitboxDeactivated{false};
    int mCurrentComboStep{0};
    bool mAttackBuffered{false};
};

class PlayerHeavyStrikeState : public PlayerState {
public:
//------------[Virtual Destructor - Heavy Strike State Cleanup]-------------------
    ~PlayerHeavyStrikeState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Beast Heavy Claw Strike]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Heavy Strike Hitbox]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Heavy Strike Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Heavy Strike Poise-Breaking Arc & Rage Generation]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Heavy Strike State Name]-------------------
    std::string_view getName() const override { return "HeavyStrike"; }
//-------------------------------------------------------

//------------[Get Type - Return Heavy Strike State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::HeavyStrike; }
//-------------------------------------------------------

private:
    float mAttackTimer{0.f};
    bool mHitboxActivated{false};
    bool mHitboxDeactivated{false};
};

class PlayerCastSpellState : public PlayerState {
public:
//------------[Virtual Destructor - Cast Spell State Cleanup]-------------------
    ~PlayerCastSpellState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Witch Spell Cast]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Spell Casting Flags]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Spell Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Spell Cast Animation, Mana Consumption & Projectile Spawn]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Cast Spell State Name]-------------------
    std::string_view getName() const override { return "CastSpell"; }
//-------------------------------------------------------

//------------[Get Type - Return Cast Spell State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::CastSpell; }
//-------------------------------------------------------

private:
    float mCastTimer{0.f};
    bool mProjectileSpawned{false};
};

class PlayerGunState : public PlayerState {
public:
//------------[Virtual Destructor - Gun State Cleanup]-------------------
    ~PlayerGunState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Witch Gun Shot & Auto-Aim Recoil]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Gun State Properties]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Gun Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Gun Recovery & Aerial Momentum]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Gun State Name]-------------------
    std::string_view getName() const override { return "Gun"; }
//-------------------------------------------------------

//------------[Get Type - Return Gun State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Gun; }
//-------------------------------------------------------

private:
    float mGunTimer{0.f};
    float mAirStaggerTimer{0.07f};
    bool mRecoilApplied{false};
    sf::Vector2f mRecoil{0.f, 0.f};
};

class PlayerParryState : public PlayerState {
public:
//------------[Virtual Destructor - Parry State Cleanup]-------------------
    ~PlayerParryState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Parry Stance & Counter Window]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Parry Stance]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Parry Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Parry Duration & Friction]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Parry State Name]-------------------
    std::string_view getName() const override { return "Parry"; }
//-------------------------------------------------------

//------------[Get Type - Return Parry State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Parry; }
//-------------------------------------------------------

//------------[Is Parry Window Active - Query Active Counter Stance Window]-------------------
    bool isParryActive() const { return mParryTimer <= mParryWindow; }
//-------------------------------------------------------

private:
    float mParryTimer{0.f};
    float mParryWindow{0.20f};
    float mTotalDuration{0.30f};
};

class PlayerRoarState : public PlayerState {
public:
//------------[Virtual Destructor - Roar State Cleanup]-------------------
    ~PlayerRoarState() override = default;
//-------------------------------------------------------

//------------[Enter - Initialize Beast Roar Stagger AoE]-------------------
    void enter(Player& player) override;
//-------------------------------------------------------

//------------[Exit - Reset Roar State]-------------------
    void exit(Player& player) override;
//-------------------------------------------------------

//------------[Handle Input - Process Roar Key Events]-------------------
    void handleInput(Player& player, const sf::Event& event) override;
//-------------------------------------------------------

//------------[Fixed Update - Step Roar Duration, Poise Break Sweep & Shockwave]-------------------
    void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Get Name - Return Roar State Name]-------------------
    std::string_view getName() const override { return "Roar"; }
//-------------------------------------------------------

//------------[Get Type - Return Roar State Type]-------------------
    PlayerStateType getType() const override { return PlayerStateType::Roar; }
//-------------------------------------------------------

private:
    float mRoarTimer{0.f};
    bool mRoarTriggered{false};
};


