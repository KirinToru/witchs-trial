#include <Game/Entities/GrandInquisitor.hpp>
#include <Game/Entities/Player.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Engine/Audio/AudioManager.hpp>
#include <cmath>
#include <algorithm>

//------------[Constructor - Initialize Grand Inquisitor Boss Stats & Position]-------------------
GrandInquisitor::GrandInquisitor(sf::Vector2f position)
    : Enemy(
          /*size=*/{46.f, 72.f},
          /*maxHealth=*/500.f,
          /*maxPosture=*/140.f,
          /*moveSpeed=*/85.f,
          /*attackRange=*/74.f,
          /*detectionRange=*/3000.f,
          /*telegraphDuration=*/0.85f,
          /*staggerDuration=*/2.0f) {

    setPosition(position);

    // Ornate Grand Inquisitor Armor: Dark Steel with Heavy Gold Accents
    mShape.setFillColor(sf::Color(45, 50, 65));
    mShape.setOutlineColor(sf::Color(215, 175, 45));
    mShape.setOutlineThickness(2.0f);
}
//-------------------------------------------------------

//------------[Engage - Wake Boss and Initiate Combat Arena Encounter]-------------------
void GrandInquisitor::engage() {
    mEngaged = true;
    if (getCurrentStateType() == EnemyStateType::Idle) {
        changeState(EnemyStateType::Chase);
    }
}
//-------------------------------------------------------

//------------[Start Phase Transition - Trigger Roar, Invulnerability & Visual Shift]-------------------
void GrandInquisitor::startPhaseTransition() {
    mPhase = BossPhase::Transitioning;
    mTransitionTimer = 0.f;
    mTriggerTransitionShake = true;
    mHealth = mMaxHealth * 0.5f;
    mPosture = mMaxPosture;

    deactivateAttackHitbox();
    sf::Vector2f vel = getVelocity();
    vel.x = 0.f;
    setVelocity(vel);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_roar.wav", 100.f);
}
//-------------------------------------------------------

//------------[Take Damage - Apply Damage with Phase Transition Safeguard]-------------------
bool GrandInquisitor::takeDamage(float damage, float poiseDamage, sf::Vector2f knockback) {
    if (mPhase == BossPhase::Transitioning) {
        return false;
    }

    bool broken = Enemy::takeDamage(damage, poiseDamage, knockback);

    if (mHealth <= mMaxHealth * 0.5f && mPhase == BossPhase::Phase1) {
        startPhaseTransition();
    }

    return broken;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Boss Multi-Phase AI, Transition & Shockwaves (60Hz)]-------------------
void GrandInquisitor::fixedUpdate(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    if (mShockwaveActive) {
        mShockwaveTimer -= dt;
        if (mShockwaveTimer <= 0.f) {
            mShockwaveActive = false;
        }
    }

    if (!mEngaged) {
        sf::Vector2f vel = getVelocity();
        vel.x = 0.f;
        vel.y += 980.f * dt;
        setVelocity(vel);
        moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());
        getAnimator().play("Idle");
        return;
    }

    // Phase transition sequence: Boss roars and gathers power
    if (mPhase == BossPhase::Transitioning) {
        mTransitionTimer += dt;

        sf::Vector2f vel = getVelocity();
        vel.y += 980.f * dt;
        vel.x = 0.f;
        setVelocity(vel);
        moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());

        if (mTransitionTimer >= 1.4f) {
            mPhase = BossPhase::Phase2;
            mMoveSpeed = 165.f;
            mTelegraphDuration = 0.35f;
            mAttackRange = 65.f;
            mPosture = mMaxPosture;
            mComboStep = 0;
            changeState(EnemyStateType::Chase);
        }
        return;
    }

    // Check health threshold in Phase 1
    if (mHealth <= mMaxHealth * 0.5f && mPhase == BossPhase::Phase1) {
        startPhaseTransition();
        return;
    }

    if (getCurrentStateType() == EnemyStateType::Idle && !isDead() && !isStaggered()) {
        changeState(EnemyStateType::Chase);
    }

    EnemyStateType prevState = getCurrentStateType();

    Enemy::fixedUpdate(dt, player, physicsWorld);

    if (getCurrentStateType() == EnemyStateType::Idle && !isDead() && !isStaggered()) {
        changeState(EnemyStateType::Chase);
    }

    // Phase 2 Combo Chaining: When an attack ends, chain into next combo step if close to player
    if (mPhase == BossPhase::Phase2) {
        EnemyStateType currentState = getCurrentStateType();
        if (prevState == EnemyStateType::ActiveAttack && currentState != EnemyStateType::ActiveAttack && !isStaggered() && !isDead()) {
            if (mComboStep < 2 && getDistanceToPlayer(player) <= 130.f && hasLineOfSightToPlayer(player, physicsWorld)) {
                mComboStep++;
                mAttackCooldownTimer = 0.06f;
                changeState(EnemyStateType::TelegraphAttack);
            } else {
                mComboStep = 0;
                mAttackCooldownTimer = 0.9f;
            }
        }
    }
}
//-------------------------------------------------------

//------------[Spawn Attack Hitbox - Build Phase-Specific AoE Ground Smash or Combo Slashes]-------------------
void GrandInquisitor::spawnAttackHitbox() {
    sf::Vector2f size = mShape.getSize();

    if (mPhase == BossPhase::Phase1) {
        // Phase 1: Heavy Telegraphed AoE Ground Smash
        Combat::Hitbox smash;
        smash.damage = 32.f;
        smash.poiseDamage = 26.f;
        smash.knockback = mFacingRight ? sf::Vector2f(280.f, -160.f) : sf::Vector2f(-280.f, -160.f);
        smash.active = true;

        if (mFacingRight) {
            smash.localBounds = Physics::AABB::fromPositionSize({16.f, 20.f}, {110.f, 54.f});
        } else {
            smash.localBounds = Physics::AABB::fromPositionSize({-80.f, 20.f}, {110.f, 54.f});
        }
        setAttackHitbox(smash);

        mShockwaveActive = true;
        mShockwaveTimer = 0.38f;
        mShockwaveOrigin = mShape.getPosition() + sf::Vector2f(mFacingRight ? size.x : 0.f, size.y);
        mShockwaveFacingDir = mFacingRight ? 1.f : -1.f;
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_smash.wav", 95.f);

    } else {
        // Phase 2: Chained Multi-Hit Bloodflame Combos
        Combat::Hitbox comboHit;
        comboHit.active = true;

        if (mComboStep == 0) {
            // Combo Step 1: Rapid Forward Lunging Sweep
            comboHit.damage = 18.f;
            comboHit.poiseDamage = 14.f;
            comboHit.knockback = mFacingRight ? sf::Vector2f(220.f, -80.f) : sf::Vector2f(-220.f, -80.f);
            comboHit.localBounds = Physics::AABB::fromPositionSize(
                {mFacingRight ? 20.f : -44.f, 20.f}, {54.f, 44.f});

            sf::Vector2f vel = getVelocity();
            vel.x = mFacingRight ? 180.f : -180.f;
            setVelocity(vel);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_slash.wav", 85.f);

        } else if (mComboStep == 1) {
            // Combo Step 2: Rising Upward Cleave
            comboHit.damage = 22.f;
            comboHit.poiseDamage = 16.f;
            comboHit.knockback = mFacingRight ? sf::Vector2f(240.f, -180.f) : sf::Vector2f(-240.f, -180.f);
            comboHit.localBounds = Physics::AABB::fromPositionSize(
                {mFacingRight ? 24.f : -48.f, 6.f}, {56.f, 58.f});

            sf::Vector2f vel = getVelocity();
            vel.x = mFacingRight ? 200.f : -200.f;
            setVelocity(vel);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_slash.wav", 90.f);

        } else {
            // Combo Step 3: Leaping Bloodflame Ground Slam
            comboHit.damage = 36.f;
            comboHit.poiseDamage = 30.f;
            comboHit.knockback = mFacingRight ? sf::Vector2f(320.f, -200.f) : sf::Vector2f(-320.f, -200.f);
            comboHit.localBounds = Physics::AABB::fromPositionSize(
                {mFacingRight ? 14.f : -84.f, 18.f}, {100.f, 56.f});

            sf::Vector2f vel = getVelocity();
            vel.x = mFacingRight ? 240.f : -240.f;
            setVelocity(vel);

            mShockwaveActive = true;
            mShockwaveTimer = 0.35f;
            mShockwaveOrigin = mShape.getPosition() + sf::Vector2f(mFacingRight ? size.x : 0.f, size.y);
            mShockwaveFacingDir = mFacingRight ? 1.f : -1.f;
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_smash.wav", 100.f);
        }

        setAttackHitbox(comboHit);
    }
}
//-------------------------------------------------------

//------------[Render - Draw Ornate Boss Armor, Golden Telegraphs & Bloodflame Phase 2]-------------------
void GrandInquisitor::render(sf::RenderWindow& window, bool showHitbox) {
    Enemy::render(window, showHitbox);

    if (isDead()) return;

    sf::Vector2f pos = mShape.getPosition();
    sf::Vector2f size = mShape.getSize();

    // 1. Ornate Pauldrons (Heavy Golden Shoulder Armor)
    sf::RectangleShape leftShoulder({12.f, 16.f});
    leftShoulder.setFillColor(sf::Color(215, 175, 45));
    leftShoulder.setOutlineColor(sf::Color(30, 30, 40));
    leftShoulder.setOutlineThickness(1.5f);
    leftShoulder.setPosition({pos.x - 4.f, pos.y + 12.f});
    window.draw(leftShoulder);

    sf::RectangleShape rightShoulder({12.f, 16.f});
    rightShoulder.setFillColor(sf::Color(215, 175, 45));
    rightShoulder.setOutlineColor(sf::Color(30, 30, 40));
    rightShoulder.setOutlineThickness(1.5f);
    rightShoulder.setPosition({pos.x + size.x - 8.f, pos.y + 12.f});
    window.draw(rightShoulder);

    // 2. Grand Inquisitor Crimson & White Tabard
    sf::RectangleShape tabard({size.x * 0.6f, size.y * 0.55f});
    if (mPhase == BossPhase::Phase2 || mPhase == BossPhase::Transitioning) {
        tabard.setFillColor(sf::Color(140, 20, 20));
    } else {
        tabard.setFillColor(sf::Color(180, 35, 35));
    }
    tabard.setPosition({pos.x + size.x * 0.2f, pos.y + size.y * 0.28f});
    window.draw(tabard);

    // Golden Cross Emblem on Chest
    sf::RectangleShape crossV({4.f, 22.f});
    crossV.setFillColor(sf::Color(245, 205, 50));
    crossV.setPosition({pos.x + size.x * 0.5f - 2.f, pos.y + size.y * 0.32f});
    window.draw(crossV);

    sf::RectangleShape crossH({14.f, 4.f});
    crossH.setFillColor(sf::Color(245, 205, 50));
    crossH.setPosition({pos.x + size.x * 0.5f - 7.f, pos.y + size.y * 0.37f});
    window.draw(crossH);

    // 3. Menacing Helmet Visor Slit
    sf::RectangleShape visor({12.f, 4.f});
    if (mPhase == BossPhase::Phase2 || mPhase == BossPhase::Transitioning) {
        visor.setFillColor(sf::Color(255, 30, 20));
    } else {
        visor.setFillColor(sf::Color(255, 215, 40));
    }
    float visorX = mFacingRight ? (pos.x + size.x * 0.52f) : (pos.x + size.x * 0.22f);
    visor.setPosition({visorX, pos.y + 14.f});
    window.draw(visor);

    // 4. Phase Transition / Phase 2 Fiery Crimson Aura
    if (mPhase == BossPhase::Transitioning || mPhase == BossPhase::Phase2) {
        float pulse = std::abs(std::sin(mTransitionTimer * 8.f));
        sf::RectangleShape aura({size.x + 16.f, size.y + 12.f});
        aura.setOrigin({8.f, 6.f});
        aura.setPosition(pos);
        aura.setFillColor(sf::Color(0, 0, 0, 0));
        aura.setOutlineColor(sf::Color(255, static_cast<std::uint8_t>(40 + 60 * pulse), 20, static_cast<std::uint8_t>(140 + 90 * pulse)));
        aura.setOutlineThickness(3.5f);
        window.draw(aura);
    }

    // 5. Colossal Boss Greatsword
    EnemyStateType state = getCurrentStateType();
    sf::RectangleShape sword;

    if (state == EnemyStateType::TelegraphAttack) {
        sword.setSize({8.f, 54.f});
        sword.setOrigin({4.f, 50.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.75f) : (pos.x + size.x * 0.25f), pos.y + 14.f});
        sword.setRotation(sf::degrees(mFacingRight ? -45.f : 45.f));

        if (mPhase == BossPhase::Phase2) {
            sword.setFillColor(sf::Color(255, 60, 30));
            sword.setOutlineColor(sf::Color(255, 230, 50));
            sword.setOutlineThickness(3.f);
        } else {
            sword.setFillColor(sf::Color(255, 220, 50));
            sword.setOutlineColor(sf::Color(255, 120, 20));
            sword.setOutlineThickness(2.5f);
        }
        window.draw(sword);

    } else if (state == EnemyStateType::ActiveAttack) {
        sword.setSize({56.f, 8.f});
        sword.setOrigin({4.f, 4.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.5f) : (pos.x + size.x * 0.5f), pos.y + 36.f});
        sword.setRotation(sf::degrees(mFacingRight ? 25.f : 155.f));

        if (mPhase == BossPhase::Phase2) {
            sword.setFillColor(sf::Color(255, 90, 40));
            sword.setOutlineColor(sf::Color(255, 30, 20));
            sword.setOutlineThickness(2.f);
        } else {
            sword.setFillColor(sf::Color(230, 235, 245));
            sword.setOutlineColor(sf::Color(245, 190, 40));
            sword.setOutlineThickness(2.f);
        }
        window.draw(sword);

    } else if (state == EnemyStateType::Staggered) {
        sword.setSize({8.f, 46.f});
        sword.setOrigin({4.f, 4.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x + 8.f) : (pos.x - 8.f), pos.y + 38.f});
        sword.setRotation(sf::degrees(mFacingRight ? 60.f : -60.f));
        sword.setFillColor(sf::Color(110, 115, 130));
        window.draw(sword);

    } else {
        sword.setSize({7.f, 50.f});
        sword.setOrigin({3.5f, 44.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.2f) : (pos.x + size.x * 0.8f), pos.y + 34.f});
        sword.setRotation(sf::degrees(mFacingRight ? -12.f : 12.f));
        sword.setFillColor(sf::Color(190, 195, 210));
        sword.setOutlineColor(sf::Color(200, 160, 40));
        sword.setOutlineThickness(1.5f);
        window.draw(sword);
    }

    // 6. Ground AoE Shockwave Visual
    if (mShockwaveActive) {
        float progress = 1.f - (mShockwaveTimer / 0.38f);
        float shockW = 120.f * progress;
        float shockH = 14.f * (1.f - progress * 0.5f);

        sf::RectangleShape shock({shockW, shockH});
        shock.setOrigin({0.f, shockH * 0.5f});
        float startX = mShockwaveOrigin.x;
        if (mShockwaveFacingDir < 0.f) {
            shock.setScale({-1.f, 1.f});
        }
        shock.setPosition({startX, mShockwaveOrigin.y - 8.f});

        if (mPhase == BossPhase::Phase2) {
            shock.setFillColor(sf::Color(255, 80, 30, static_cast<std::uint8_t>(200 * (1.f - progress))));
            shock.setOutlineColor(sf::Color(255, 230, 80, static_cast<std::uint8_t>(240 * (1.f - progress))));
        } else {
            shock.setFillColor(sf::Color(255, 215, 60, static_cast<std::uint8_t>(200 * (1.f - progress))));
            shock.setOutlineColor(sf::Color(255, 140, 30, static_cast<std::uint8_t>(240 * (1.f - progress))));
        }
        shock.setOutlineThickness(2.f);
        window.draw(shock);
    }
}
//-------------------------------------------------------
