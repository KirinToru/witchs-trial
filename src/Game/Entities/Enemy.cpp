#include <Game/Entities/Enemy.hpp>
#include <Game/Entities/Player.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <cmath>
#include <algorithm>
#include <cstdint>

//------------[Constructor - Initialize Enemy Base Properties]-------------------
Enemy::Enemy(sf::Vector2f size, float maxHealth, float maxPosture, float moveSpeed,
             float attackRange, float detectionRange, float telegraphDuration, float staggerDuration)
    : mHealth(maxHealth),
      mMaxHealth(maxHealth),
      mPosture(maxPosture),
      mMaxPosture(maxPosture),
      mMoveSpeed(moveSpeed),
      mAttackRange(attackRange),
      mDetectionRange(detectionRange),
      mTelegraphDuration(telegraphDuration),
      mStaggerDuration(staggerDuration) {

    mShape.setSize(size);
    mShape.setFillColor(sf::Color(140, 140, 150));
    mShape.setOutlineThickness(1.5f);
    mShape.setOutlineColor(sf::Color(40, 40, 45));

    mHurtbox.localBounds = Physics::AABB::fromPositionSize({0.f, 0.f}, size);
    mHurtbox.active = true;
    mHurtbox.invulnerable = false;

    mIdleState = std::make_unique<EnemyIdleState>();
    mChaseState = std::make_unique<EnemyChaseState>();
    mTelegraphAttackState = std::make_unique<EnemyTelegraphAttackState>();
    mActiveAttackState = std::make_unique<EnemyActiveAttackState>();
    mStaggeredState = std::make_unique<EnemyStaggeredState>();
    mDeadState = std::make_unique<EnemyDeadState>();

    mAnimator.addAnimation(Engine::Graphics::Animation("Idle", 4, 0.25f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Chase", 4, 0.12f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Telegraph", 4, 0.16f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("ActiveAttack", 5, 0.08f, false));
    mAnimator.addAnimation(Engine::Graphics::Animation("Staggered", 4, 0.25f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Dead", 1, 1.0f, false));
    mAnimator.play("Idle");

    mCurrentState = mIdleState.get();
    mCurrentState->enter(*this);
}
//-------------------------------------------------------

//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
Enemy::~Enemy() = default;
//-------------------------------------------------------

//------------[Fixed Update - Step AI FSM and Kinematic Physics (60Hz)]-------------------
void Enemy::fixedUpdate(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    if (mHitFlashTimer > 0.f) {
        mHitFlashTimer = std::max(0.f, mHitFlashTimer - dt);
    }

    if (mAttackCooldownTimer > 0.f) {
        mAttackCooldownTimer = std::max(0.f, mAttackCooldownTimer - dt);
    }

    // Passive Posture Recovery (Souls-like: recovers after delay if not staggered or dead)
    if (!isStaggered() && !isDead()) {
        if (mPostureRegenDelayTimer > 0.f) {
            mPostureRegenDelayTimer = std::max(0.f, mPostureRegenDelayTimer - dt);
        } else if (mPosture < mMaxPosture) {
            mPosture = std::min(mMaxPosture, mPosture + mPostureRegenRate * dt);
        }
    }

    if (mCurrentState) {
        mCurrentState->fixedUpdate(*this, dt, player, physicsWorld);
    }

    mAnimator.update(dt);

    mHurtbox.localBounds = Physics::AABB::fromPositionSize({0.f, 0.f}, mShape.getSize());
}
//-------------------------------------------------------

//------------[Render - Draw Enemy Visuals, Telegraph Indicators, Overhead Gauges & Debug Boxes]-------------------
void Enemy::render(sf::RenderWindow& window, bool showHitbox) {
    // 1. Enemy Character Body
    sf::RectangleShape renderShape = mShape;

    if (isDead()) {
        renderShape.setFillColor(sf::Color(60, 60, 65, 180));
        renderShape.setOutlineColor(sf::Color(30, 30, 35, 150));
    } else if (mHitFlashTimer > 0.f) {
        renderShape.setFillColor(sf::Color(255, 255, 255, 240));
    } else if (isStaggered()) {
        // Staggered pose visual - vulnerability flashing
        renderShape.setFillColor(sf::Color(180, 110, 110));
        renderShape.setOutlineColor(sf::Color(255, 60, 60));
        renderShape.setOutlineThickness(2.5f);
    } else if (getCurrentStateType() == EnemyStateType::TelegraphAttack) {
        // Souls-like glowing telegraph aura (Yellow/Orange warning)
        float progress = (mTelegraphDuration > 0.f) ? (mTelegraphTimer / mTelegraphDuration) : 1.f;
        auto pulse = static_cast<std::uint8_t>(180 + 75 * std::sin(progress * 3.14159f * 4.f));
        renderShape.setOutlineColor(sf::Color(255, pulse, 20));
        renderShape.setOutlineThickness(3.f);
    }

    window.draw(renderShape);

    // 2. Telegraph Warning Icon / Glint when winding up attack
    if (getCurrentStateType() == EnemyStateType::TelegraphAttack) {
        sf::CircleShape glint(6.f);
        glint.setFillColor(sf::Color(255, 200, 30));
        glint.setOutlineColor(sf::Color::Red);
        glint.setOutlineThickness(1.5f);
        sf::Vector2f headPos = mShape.getPosition();
        glint.setPosition({mFacingRight ? (headPos.x + mShape.getSize().x - 4.f) : (headPos.x - 8.f),
                           headPos.y - 12.f});
        window.draw(glint);
    }

    // 3. Overhead Health & Posture Gauges (Souls-like UI)
    if (!isDead()) {
        sf::Vector2f pos = mShape.getPosition();
        float barWidth = std::max(36.f, mShape.getSize().x + 8.f);
        float barHeight = 4.f;
        float barX = pos.x + (mShape.getSize().x - barWidth) * 0.5f;
        float barY = pos.y - 16.f;

        // Health Bar Background
        sf::RectangleShape hpBg({barWidth, barHeight});
        hpBg.setPosition({barX, barY});
        hpBg.setFillColor(sf::Color(30, 30, 35, 220));
        hpBg.setOutlineThickness(1.f);
        hpBg.setOutlineColor(sf::Color::Black);
        window.draw(hpBg);

        // Health Bar Fill (Red)
        float hpPct = std::clamp(mHealth / mMaxHealth, 0.f, 1.f);
        if (hpPct > 0.f) {
            sf::RectangleShape hpFill({barWidth * hpPct, barHeight});
            hpFill.setPosition({barX, barY});
            hpFill.setFillColor(sf::Color(210, 40, 40));
            window.draw(hpFill);
        }

        // Posture Bar Background (Directly below HP bar)
        float postureY = barY + barHeight + 2.f;
        sf::RectangleShape postBg({barWidth, barHeight});
        postBg.setPosition({barX, postureY});
        postBg.setFillColor(sf::Color(30, 30, 35, 220));
        postBg.setOutlineThickness(1.f);
        postBg.setOutlineColor(sf::Color::Black);
        window.draw(postBg);

        // Posture Bar Fill (Yellow/Orange; Flashing when broken/staggered)
        float postPct = std::clamp(mPosture / mMaxPosture, 0.f, 1.f);
        if (isStaggered()) {
            // Flash empty broken posture bar
            sf::RectangleShape postFill({barWidth, barHeight});
            postFill.setPosition({barX, postureY});
            postFill.setFillColor(sf::Color(255, 80, 80, 200));
            window.draw(postFill);
        } else if (postPct > 0.f) {
            sf::RectangleShape postFill({barWidth * postPct, barHeight});
            postFill.setPosition({barX, postureY});
            postFill.setFillColor(sf::Color(245, 180, 30));
            window.draw(postFill);
        }
    }

    // 4. Debug Combat Boxes
    if (showHitbox) {
        // Defensive Hurtbox
        if (mHurtbox.active) {
            Physics::AABB worldHurtbox = mHurtbox.getWorldAABB(mShape.getPosition());
            sf::RectangleShape hurtboxVis;
            hurtboxVis.setPosition(worldHurtbox.min);
            hurtboxVis.setSize(worldHurtbox.getSize());
            hurtboxVis.setFillColor(sf::Color(50, 150, 255, 70));
            hurtboxVis.setOutlineColor(sf::Color(50, 180, 255));
            hurtboxVis.setOutlineThickness(1.f);
            window.draw(hurtboxVis);
        }

        // Offensive Attack Hitbox
        if (mAttackHitbox.active) {
            Physics::AABB worldHitbox = mAttackHitbox.getWorldAABB(mShape.getPosition());
            sf::RectangleShape attackVis;
            attackVis.setPosition(worldHitbox.min);
            attackVis.setSize(worldHitbox.getSize());
            attackVis.setFillColor(sf::Color(255, 30, 30, 120));
            attackVis.setOutlineColor(sf::Color::Yellow);
            attackVis.setOutlineThickness(2.f);
            window.draw(attackVis);
        }
    }
}
//-------------------------------------------------------

//------------[Change State - Transition to Specified AI State]-------------------
void Enemy::changeState(EnemyStateType newType) {
    if (mCurrentState && mCurrentState->getType() == newType) {
        return;
    }

    if (mCurrentState) {
        mCurrentState->exit(*this);
    }

    switch (newType) {
        case EnemyStateType::Idle:
            mCurrentState = mIdleState.get();
            break;
        case EnemyStateType::Chase:
            mCurrentState = mChaseState.get();
            break;
        case EnemyStateType::TelegraphAttack:
            mCurrentState = mTelegraphAttackState.get();
            break;
        case EnemyStateType::ActiveAttack:
            mCurrentState = mActiveAttackState.get();
            break;
        case EnemyStateType::Staggered:
            mCurrentState = mStaggeredState.get();
            break;
        case EnemyStateType::Dead:
            mCurrentState = mDeadState.get();
            break;
    }

    if (mCurrentState) {
        mCurrentState->enter(*this);
    }
}
//-------------------------------------------------------

//------------[Get State Name - Return Human-Readable State String]-------------------
std::string_view Enemy::getStateName() const {
    if (mCurrentState) {
        return mCurrentState->getName();
    }
    return "None";
}
//-------------------------------------------------------

//------------[Get Current State Type - Return State Type Enum]-------------------
EnemyStateType Enemy::getCurrentStateType() const {
    if (mCurrentState) {
        return mCurrentState->getType();
    }
    return EnemyStateType::Idle;
}
//-------------------------------------------------------

//------------[Take Damage - Apply Damage, Poise Damage, and Knockback]-------------------
bool Enemy::takeDamage(float damage, float poiseDamage, sf::Vector2f knockback) {
    if (isDead()) return false;

    float actualDamage = damage;
    if (isStaggered()) {
        actualDamage *= 1.5f;
    }

    mHealth -= actualDamage;
    mHitFlashTimer = 0.12f;
    mVelocity += knockback;

    if (mHealth <= 0.f) {
        mHealth = 0.f;
        changeState(EnemyStateType::Dead);
        return false;
    }

    bool wasStaggered = isStaggered();
    bool postureBroken = false;

    mPosture = std::max(0.f, mPosture - poiseDamage);
    mPostureRegenDelayTimer = 3.5f;

    if (mPosture <= 0.f && !wasStaggered) {
        changeState(EnemyStateType::Staggered);
        postureBroken = true;
    }

    return postureBroken;
}
//-------------------------------------------------------

//------------[Has Line Of Sight - Perform Swept Raycast to Player]-------------------
bool Enemy::hasLineOfSightToPlayer(const Player& player, const Physics::PhysicsWorld& physicsWorld) const {
    sf::Vector2f enemyCenter = mShape.getPosition() + mShape.getSize() * 0.5f;
    sf::Vector2f playerCenter = player.getPosition() + player.getBounds().size * 0.5f;

    sf::Vector2f displacement = playerCenter - enemyCenter;
    Physics::AABB probe = Physics::AABB::fromPositionSize(enemyCenter - sf::Vector2f(2.f, 2.f), {4.f, 4.f});

    Physics::SweptHit hit = physicsWorld.sweepTest(probe, displacement, player.getRigidBody(), false);
    if (hit.hit && hit.toi < 0.99f && hit.body && hit.body->getType() == Physics::BodyType::Static && !hit.body->isOneWay()) {
        return false;
    }
    return true;
}
//-------------------------------------------------------

//------------[Get Distance To Player - Calculate Center-to-Center Distance]-------------------
float Enemy::getDistanceToPlayer(const Player& player) const {
    sf::Vector2f enemyCenter = mShape.getPosition() + mShape.getSize() * 0.5f;
    sf::Vector2f playerCenter = player.getPosition() + player.getBounds().size * 0.5f;

    float dx = playerCenter.x - enemyCenter.x;
    float dy = playerCenter.y - enemyCenter.y;
    return std::sqrt(dx * dx + dy * dy);
}
//-------------------------------------------------------

//------------[Get Direction To Player - Return Horizontal Unit Direction (-1 or 1)]-------------------
float Enemy::getDirectionToPlayer(const Player& player) const {
    sf::Vector2f enemyCenter = mShape.getPosition() + mShape.getSize() * 0.5f;
    sf::Vector2f playerCenter = player.getPosition() + player.getBounds().size * 0.5f;
    return (playerCenter.x >= enemyCenter.x) ? 1.f : -1.f;
}
//-------------------------------------------------------

//------------[Move With Swept CCD - Step Kinematic Movement with Continuous Collision Detection]-------------------
void Enemy::moveWithSweptCCD(sf::Vector2f displacement, const Physics::PhysicsWorld& physicsWorld, const Physics::RigidBody* ignoreBody) {
    // 0. Resolve any pre-existing penetration with static colliders
    Physics::AABB currentAABB = getAABB();
    auto overlaps = physicsWorld.queryAABB(currentAABB, nullptr);
    for (const auto* body : overlaps) {
        if (!body || body->getType() != Physics::BodyType::Static || body->isOneWay()) continue;
        Physics::AABB targetBox = body->getWorldAABB();

        sf::Vector2f centerA = currentAABB.getCenter();
        sf::Vector2f centerB = targetBox.getCenter();
        sf::Vector2f halfA = currentAABB.getHalfExtents();
        sf::Vector2f halfB = targetBox.getHalfExtents();
        sf::Vector2f delta = centerA - centerB;

        float overlapX = (halfA.x + halfB.x) - std::abs(delta.x);
        float overlapY = (halfA.y + halfB.y) - std::abs(delta.y);

        if (overlapX > 0.001f && overlapY > 0.001f) {
            if (overlapX < overlapY) {
                float pushX = (delta.x < 0.f) ? -overlapX : overlapX;
                mShape.move({pushX, 0.f});
            } else {
                float pushY = (delta.y < 0.f) ? -overlapY : overlapY;
                mShape.move({0.f, pushY});
            }
            currentAABB = getAABB();
        }
    }

    // 1. Horizontal swept collision
    if (displacement.x != 0.f) {
        sf::Vector2f dispX{displacement.x, 0.f};
        Physics::AABB boxX = getAABB();
        Physics::SweptHit hitX = physicsWorld.sweepTest(boxX, dispX, ignoreBody, false);

        if (hitX.hit) {
            float safeToi = std::max(0.0f, hitX.toi - 0.001f);
            mShape.move({dispX.x * safeToi, 0.f});
            mVelocity.x = 0.f;
        } else {
            mShape.move(dispX);
        }
    }

    // 2. Vertical swept collision
    mIsGrounded = false;
    if (displacement.y != 0.f) {
        sf::Vector2f dispY{0.f, displacement.y};
        Physics::AABB boxY = getAABB();
        Physics::SweptHit hitY = physicsWorld.sweepTest(boxY, dispY, ignoreBody, false);

        if (hitY.hit) {
            float safeToi = std::max(0.0f, hitY.toi - 0.001f);
            mShape.move({0.f, dispY.y * safeToi});

            if (displacement.y > 0.f) {
                mIsGrounded = true;
                mVelocity.y = 0.f;
            } else if (displacement.y < 0.f) {
                mVelocity.y = 0.f;
            }
        } else {
            mShape.move(dispY);
        }
    }
}
//-------------------------------------------------------

//------------[Get Position - Query World Space Position]-------------------
sf::Vector2f Enemy::getPosition() const {
    return mShape.getPosition();
}
//-------------------------------------------------------

//------------[Set Position - Update World Position and Hitboxes]-------------------
void Enemy::setPosition(sf::Vector2f pos) {
    mShape.setPosition(pos);
}
//-------------------------------------------------------

//------------[Get Velocity - Query Current Velocity Vector]-------------------
sf::Vector2f Enemy::getVelocity() const {
    return mVelocity;
}
//-------------------------------------------------------

//------------[Set Velocity - Update Current Velocity Vector]-------------------
void Enemy::setVelocity(sf::Vector2f vel) {
    mVelocity = vel;
}
//-------------------------------------------------------

//------------[Get Size - Query Bounding Box Size]-------------------
sf::Vector2f Enemy::getSize() const {
    return mShape.getSize();
}
//-------------------------------------------------------

//------------[Get AABB - Query Physics Collision AABB]-------------------
Physics::AABB Enemy::getAABB() const {
    return Physics::AABB::fromPositionSize(mShape.getPosition(), mShape.getSize());
}
//-------------------------------------------------------

//------------[Is Facing Right - Query Facing Direction]-------------------
bool Enemy::isFacingRight() const {
    return mFacingRight;
}
//-------------------------------------------------------

//------------[Set Facing Right - Update Facing Direction]-------------------
void Enemy::setFacingRight(bool right) {
    mFacingRight = right;
}
//-------------------------------------------------------

//------------[Get Health - Query Current Health Points]-------------------
float Enemy::getHealth() const {
    return mHealth;
}
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Health Capacity]-------------------
float Enemy::getMaxHealth() const {
    return mMaxHealth;
}
//-------------------------------------------------------

//------------[Get Posture - Query Current Posture / Poise Gauge]-------------------
float Enemy::getPosture() const {
    return mPosture;
}
//-------------------------------------------------------

//------------[Get Max Posture - Query Maximum Posture Capacity]-------------------
float Enemy::getMaxPosture() const {
    return mMaxPosture;
}
//-------------------------------------------------------

//------------[Is Staggered - Query If Enemy Is In Stunned Posture Break]-------------------
bool Enemy::isStaggered() const {
    return getCurrentStateType() == EnemyStateType::Staggered;
}
//-------------------------------------------------------

//------------[Is Dead - Query If Enemy Is Dead]-------------------
bool Enemy::isDead() const {
    return getCurrentStateType() == EnemyStateType::Dead;
}
//-------------------------------------------------------

//------------[Get Hurtbox - Access Defensive Hurtbox Const]-------------------
const Combat::Hurtbox& Enemy::getHurtbox() const {
    return mHurtbox;
}
//-------------------------------------------------------

//------------[Get Hurtbox Mutable - Access Defensive Hurtbox]-------------------
Combat::Hurtbox& Enemy::getHurtbox() {
    return mHurtbox;
}
//-------------------------------------------------------

//------------[Get Attack Hitbox - Access Offensive Attack Hitbox Const]-------------------
const Combat::Hitbox& Enemy::getAttackHitbox() const {
    return mAttackHitbox;
}
//-------------------------------------------------------

//------------[Get Attack Hitbox Mutable - Access Offensive Attack Hitbox]-------------------
Combat::Hitbox& Enemy::getAttackHitbox() {
    return mAttackHitbox;
}
//-------------------------------------------------------

//------------[Set Attack Hitbox - Activate Offensive Hitbox]-------------------
void Enemy::setAttackHitbox(const Combat::Hitbox& hitbox) {
    mAttackHitbox = hitbox;
}
//-------------------------------------------------------

//------------[Deactivate Attack Hitbox - Disable Offensive Hitbox]-------------------
void Enemy::deactivateAttackHitbox() {
    mAttackHitbox.active = false;
}
//-------------------------------------------------------

//------------[Get Move Speed - Query Base Movement Speed]-------------------
float Enemy::getMoveSpeed() const {
    return mMoveSpeed;
}
//-------------------------------------------------------

//------------[Get Attack Range - Query Distance Threshold For Initiating Attacks]-------------------
float Enemy::getAttackRange() const {
    return mAttackRange;
}
//-------------------------------------------------------

//------------[Get Detection Range - Query Vision Radius For Target Acquisition]-------------------
float Enemy::getDetectionRange() const {
    return mDetectionRange;
}
//-------------------------------------------------------

//------------[Get Telegraph Duration - Query Windup Time in Seconds]-------------------
float Enemy::getTelegraphDuration() const {
    return mTelegraphDuration;
}
//-------------------------------------------------------

//------------[Get Telegraph Timer - Query Current Telegraph Progress Timer]-------------------
float Enemy::getTelegraphTimer() const {
    return mTelegraphTimer;
}
//-------------------------------------------------------

//------------[Set Telegraph Timer - Update Telegraph Progress Timer]-------------------
void Enemy::setTelegraphTimer(float t) {
    mTelegraphTimer = t;
}
//-------------------------------------------------------

//------------[Get Attack Cooldown Timer - Query Attack Cooldown Remaining]-------------------
float Enemy::getAttackCooldownTimer() const {
    return mAttackCooldownTimer;
}
//-------------------------------------------------------

//------------[Set Attack Cooldown Timer - Update Attack Cooldown Remaining]-------------------
void Enemy::setAttackCooldownTimer(float t) {
    mAttackCooldownTimer = t;
}
//-------------------------------------------------------

//------------[Get Stagger Duration - Query Stun Duration Upon Posture Break]-------------------
float Enemy::getStaggerDuration() const {
    return mStaggerDuration;
}
//-------------------------------------------------------

//------------[Get Stagger Timer - Query Stagger Elapsed Time]-------------------
float Enemy::getStaggerTimer() const {
    return mStaggerTimer;
}
//-------------------------------------------------------

//------------[Set Stagger Timer - Update Stagger Elapsed Time]-------------------
void Enemy::setStaggerTimer(float t) {
    mStaggerTimer = t;
}
//-------------------------------------------------------

//------------[Reset Posture - Restore Posture Meter To Max]-------------------
void Enemy::resetPosture() {
    mPosture = mMaxPosture;
}
//-------------------------------------------------------

//------------[Get Animator - Access Enemy Animator]-------------------
Engine::Graphics::Animator& Enemy::getAnimator() {
    return mAnimator;
}
//-------------------------------------------------------

//------------[Get Animator Const - Access Enemy Animator Const]-------------------
const Engine::Graphics::Animator& Enemy::getAnimator() const {
    return mAnimator;
}
//-------------------------------------------------------
