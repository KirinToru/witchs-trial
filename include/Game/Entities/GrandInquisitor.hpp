#pragma once

#include <Game/Entities/Enemy.hpp>
#include <string_view>

enum class BossPhase : uint8_t {
    Phase1,
    Transitioning,
    Phase2
};

class GrandInquisitor : public Enemy {
public:
//------------[Constructor - Initialize Grand Inquisitor Boss Stats & Position]-------------------
    explicit GrandInquisitor(sf::Vector2f position);
//-------------------------------------------------------

//------------[Virtual Destructor - Cleanup Grand Inquisitor Resources]-------------------
    ~GrandInquisitor() override = default;
//-------------------------------------------------------

//------------[Fixed Update - Step Boss Multi-Phase AI, Transition & Shockwaves (60Hz)]-------------------
    void fixedUpdate(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) override;
//-------------------------------------------------------

//------------[Spawn Attack Hitbox - Build Phase-Specific AoE Ground Smash or Combo Slashes]-------------------
    void spawnAttackHitbox() override;
//-------------------------------------------------------

//------------[Take Damage - Apply Damage with Phase Transition Safeguard]-------------------
    bool takeDamage(float damage, float poiseDamage, sf::Vector2f knockback) override;
//-------------------------------------------------------

//------------[Render - Draw Ornate Boss Armor, Golden Telegraphs & Bloodflame Phase 2]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) override;
//-------------------------------------------------------

//------------[Engage - Wake Boss and Initiate Combat Arena Encounter]-------------------
    void engage();
//-------------------------------------------------------

//------------[Get Phase - Query Active Boss Phase Enum]-------------------
    BossPhase getPhase() const {
        return mPhase;
    }
//-------------------------------------------------------

//------------[Get Phase Number - Return Integer Phase for HUD Display (1 or 2)]-------------------
    int getPhaseNumber() const {
        return (mPhase == BossPhase::Phase2) ? 2 : 1;
    }
//-------------------------------------------------------

//------------[Get Boss Name - Return Ornate Encounter Title]-------------------
    std::string_view getBossName() const {
        return (mPhase == BossPhase::Phase2) ? "Grand Inquisitor Malichi - Enraged" : "Grand Inquisitor Malichi";
    }
//-------------------------------------------------------

//------------[Is Transitioning - Query If Boss Is In Phase Shift Roar]-------------------
    bool isTransitioning() const {
        return mPhase == BossPhase::Transitioning;
    }
//-------------------------------------------------------

//------------[Consume Transition Shake - Query and Clear Phase Transition Camera Shake Flag]-------------------
    bool consumePhaseTransitionShake() {
        bool val = mTriggerTransitionShake;
        mTriggerTransitionShake = false;
        return val;
    }
//-------------------------------------------------------

private:
//------------[Start Phase Transition - Trigger Roar, Invulnerability & Visual Shift]-------------------
    void startPhaseTransition();
//-------------------------------------------------------

    BossPhase mPhase{BossPhase::Phase1};
    float mTransitionTimer{0.f};
    bool mTriggerTransitionShake{false};
    bool mEngaged{false};

    // Phase 2 Combo State Tracking
    int mComboStep{0};
    float mComboChainTimer{0.f};

    // Ground Smash AoE Shockwave Visuals
    bool mShockwaveActive{false};
    float mShockwaveTimer{0.f};
    sf::Vector2f mShockwaveOrigin{0.f, 0.f};
    float mShockwaveFacingDir{1.f};
};
