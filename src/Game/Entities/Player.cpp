#include <Game/Entities/Player.hpp>
#include <Game/World/Map.hpp>
#include <Game/Systems/ObjectManager.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Engine/Physics/SweptAABB.hpp>
#include <Engine/Audio/AudioManager.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

//------------[Constructor - Initialize Player Components & State Machine]-------------------
Player::Player()
    : shape({30.f, 35.f}),
      velocity(0.f, 0.f),
      isGrounded(false),
      mForm(PlayerForm::Witch),
      sprite(texture),
      facingRight(true),
      wasJumpPressed(false),
      wasTransformPressed(false),
      mAutoJumpEnabled(false) {

    mAnimator.addAnimation(Engine::Graphics::Animation("Idle", 0, 0, 2, {32, 32}, 0.35f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Walk", 1, 0, 2, {32, 32}, 0.15f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Run", 2, 0, 4, {32, 32}, 0.10f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Dash", 3, 0, 2, {32, 32}, 0.08f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Jump", 4, 0, 2, {32, 32}, 0.12f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Fall", 5, 0, 2, {32, 32}, 0.12f, true));
    mAnimator.addAnimation(Engine::Graphics::Animation("Attack", 5, 0, 4, {32, 32}, 0.08f, false));
    mAnimator.addAnimation(Engine::Graphics::Animation("HeavyStrike", 2, 0, 4, {32, 32}, 0.11f, false));
    mAnimator.addAnimation(Engine::Graphics::Animation("CastSpell", 5, 0, 4, {32, 32}, 0.09f, false));
    mAnimator.play("Idle");

    // Instantiate concrete states
    mIdleState = std::make_unique<PlayerIdleState>();
    mRunState = std::make_unique<PlayerRunState>();
    mAirborneState = std::make_unique<PlayerAirborneState>();
    mDashState = std::make_unique<PlayerDashState>();
    mPounceState = std::make_unique<PlayerPounceState>();
    mMeleeAttackState = std::make_unique<PlayerMeleeAttackState>();
    mHeavyStrikeState = std::make_unique<PlayerHeavyStrikeState>();
    mCastSpellState = std::make_unique<PlayerCastSpellState>();
    mGunState = std::make_unique<PlayerGunState>();
    mParryState = std::make_unique<PlayerParryState>();
    mRoarState = std::make_unique<PlayerRoarState>();

    // Set initial active state
    mCurrentState = mIdleState.get();
    mCurrentState->enter(*this);

    // Load character spritesheet
    if (!texture.loadFromFile("assets/player/spritesheet.png")) {
        std::cerr << "Failed to load player texture from assets/player/spritesheet.png!" << std::endl;
    }
    sprite.setTexture(texture, true);
    sprite.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
    sprite.setOrigin({16.f, 32.f});

    shape.setFillColor(sf::Color(0, 0, 0, 0));
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(sf::Color::Green);
    shape.setPosition({100.f, 0.f});

    recalculatePhysicsProperties();
}
//-------------------------------------------------------

//------------[Destructor - Cleanup Resources]-------------------
Player::~Player() = default;
//-------------------------------------------------------

//------------[Init Physics - Register Player RigidBody in PhysicsWorld]-------------------
void Player::initPhysics(Physics::PhysicsWorld& physicsWorld) {
    Physics::RigidBodyDef def;
    def.type = Physics::BodyType::Kinematic;
    def.tag = Physics::ColliderTag::Generic;
    def.position = shape.getPosition();
    def.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, shape.getSize());
    def.mass = mMass;
    def.friction = 0.3f;
    def.restitution = 0.0f;

    mRigidBody = physicsWorld.createBody(def);
    mPhysicsWorld = &physicsWorld;
}
//-------------------------------------------------------

//------------[Handle Input - Delegate Input to Active State & Form Toggles]-------------------
void Player::handleInput(const sf::Event& event) {
    if (mCurrentState) {
        mCurrentState->handleInput(*this, event);
    }
}
//-------------------------------------------------------

//------------[Fixed Update - Delegate Fixed Timestep Physics to Active State]-------------------
void Player::fixedUpdate(float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    if (!mRigidBody) {
        initPhysics(const_cast<Physics::PhysicsWorld&>(physicsWorld));
    }

    if (mTransformCooldownTimer > 0.f) {
        mTransformCooldownTimer -= dt;
    }

    if (dashCooldownTimer > 0.f) {
        dashCooldownTimer -= dt;
    }

    if (jumpBufferTimer > 0.f) {
        jumpBufferTimer -= dt;
    }

    if (isGrounded) {
        coyoteTimer = coyoteTime;
    } else {
        coyoteTimer -= dt;
    }

    // Decay high momentum over time down to standard move speed
    if (currentMaxSpeed > moveSpeed && !isDashing) {
        float currentDecay = isGrounded ? speedDecay : (speedDecay * 0.6f);
        currentMaxSpeed -= currentDecay * dt;
        if (currentMaxSpeed < moveSpeed) {
            currentMaxSpeed = moveSpeed;
        }
    }

    // Buffer jump input (W)
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    bool jumpJustPressed = jumpPressed && !wasJumpPressed;
    if (jumpJustPressed || (mAutoJumpEnabled && jumpPressed && isGrounded)) {
        jumpBufferTimer = jumpBufferTime;
    }
    wasJumpPressed = jumpPressed;

    // Form toggle keyboard polling (single press per keydown with debounce)
    bool transformPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q);
    if (transformPressed && !wasTransformPressed && mTransformCooldownTimer <= 0.f) {
        toggleForm(&physicsWorld);
        mTransformCooldownTimer = 0.25f;
    }
    wasTransformPressed = transformPressed;

    // Delegate fixed step physics to the currently active state
    if (mCurrentState) {
        mCurrentState->fixedUpdate(*this, dt, map, physicsWorld);
    }

    std::vector<Physics::RigidBody*> hazardOverlaps = physicsWorld.queryAABB(getAABB(), mRigidBody);
    for (const auto* b : hazardOverlaps) {
        if (b && (b->getTag() == Physics::ColliderTag::Hazard || b->getTag() == Physics::ColliderTag::MothFloor)) {
            triggerHazardDeath();
            break;
        }
    }

    if (mStaminaRegenDelayTimer > 0.f) {
        mStaminaRegenDelayTimer -= dt;
    } else if (mStamina < mMaxStamina) {
        mStamina = std::min(mMaxStamina, mStamina + mStaminaRegenRate * dt);
    }

    if (mForm == PlayerForm::Beast) {
        if (mRageDecayDelayTimer > 0.f) {
            mRageDecayDelayTimer -= dt;
        } else if (mRage > 0.f) {
            mRage = std::max(0.f, mRage - mRageDecayRate * dt);
        }
        if (mRage <= 0.f) {
            setForm(PlayerForm::Witch, &physicsWorld);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/transform.wav", 90.f);
        }
    }

    if (mExternalImpulseTimer > 0.f) {
        mExternalImpulseTimer = std::max(0.f, mExternalImpulseTimer - dt);
    }

    // Invulnerability frames update
    if (mInvulnerableTimer > 0.f) {
        mInvulnerableTimer -= dt;
        if (mInvulnerableTimer <= 0.f) {
            mInvulnerableTimer = 0.f;
            mHurtbox.invulnerable = false;
        }
    }

    if (mGunCooldownTimer > 0.f) {
        mGunCooldownTimer = std::max(0.f, mGunCooldownTimer - dt);
    }

    if (mSpellCooldownTimer > 0.f) {
        mSpellCooldownTimer = std::max(0.f, mSpellCooldownTimer - dt);
    }

    if (mMeleeCooldownTimer > 0.f) {
        mMeleeCooldownTimer -= dt;
        if (mMeleeCooldownTimer <= 0.f) {
            mMeleeCooldownTimer = 0.f;
            mComboStep = 0;
        }
    }

    if (mComboWindowTimer > 0.f) {
        mComboWindowTimer -= dt;
        if (mComboWindowTimer <= 0.f) {
            mComboWindowTimer = 0.f;
            if (!mCurrentState || mCurrentState->getType() != PlayerStateType::MeleeAttack) {
                mComboStep = 0;
            }
        }
    }

    if (mDebugRaycast.timer > 0.f) {
        mDebugRaycast.timer = std::max(0.f, mDebugRaycast.timer - dt);
        if (mDebugRaycast.timer <= 0.f) {
            mDebugRaycast.active = false;
        }
    }

    // Sync defensive hurtbox with player dimensions
    mHurtbox.localBounds = Physics::AABB::fromPositionSize({0.f, 0.f}, shape.getSize());

    // Synchronize PhysicsWorld RigidBody with updated kinematic state
    if (mRigidBody) {
        mRigidBody->setPosition(shape.getPosition());
        mRigidBody->setVelocity(velocity);
    }

    // Step sprite animation
    updateAnimation(dt);
}
//-------------------------------------------------------

//------------[Render - Draw Animated Character Sprite & Debug Hitbox]-------------------
void Player::render(sf::RenderWindow& window, bool showHitbox) {
    window.draw(sprite);

    if (showHitbox) {
        // 1. Draw Player Defensive Hurtbox
        sf::RectangleShape hurtboxVis = shape;
        if (mHurtbox.invulnerable) {
            hurtboxVis.setFillColor(sf::Color(255, 255, 255, 120)); // I-frames white flash
            hurtboxVis.setOutlineColor(sf::Color::White);
        } else if (mForm == PlayerForm::Witch) {
            hurtboxVis.setFillColor(sf::Color(0, 255, 0, 80));
            hurtboxVis.setOutlineColor(sf::Color::Green);
        } else {
            hurtboxVis.setFillColor(sf::Color(255, 60, 60, 90));
            hurtboxVis.setOutlineColor(sf::Color::Red);
        }
        hurtboxVis.setOutlineThickness(1.f);
        window.draw(hurtboxVis);

        // 2. Draw Offensive Attack Hitbox if active
        if (mAttackHitbox.active) {
            Physics::AABB worldHitbox = mAttackHitbox.getWorldAABB(shape.getPosition());
            sf::RectangleShape attackVis;
            attackVis.setPosition(worldHitbox.min);
            attackVis.setSize(worldHitbox.getSize());
            attackVis.setFillColor(sf::Color(255, 40, 40, 130));
            attackVis.setOutlineColor(sf::Color(255, 200, 50));
            attackVis.setOutlineThickness(2.f);
            window.draw(attackVis);
        }
    }
}
//-------------------------------------------------------

//------------[Reset - Reposition Player at Respawn Point]-------------------
void Player::reset(sf::Vector2f position) {
    shape.setPosition({position.x - shape.getSize().x / 2.f,
                       position.y - shape.getSize().y / 2.f});
    velocity = {0.f, 0.f};
    isGrounded = false;
    isDashing = false;
    isWallSliding = false;
    currentMaxSpeed = moveSpeed;
    mHealth = mMaxHealth;
    mMana = mMaxMana;
    mStamina = mMaxStamina;
    mInvulnerableTimer = 0.f;
    mHurtbox.invulnerable = false;
    mGroundSmashImpact = false;
    mGunCooldownTimer = 0.f;
    mSpellCooldownTimer = 0.f;
    mExternalImpulseTimer = 0.f;
    mDebugRaycast.active = false;
    mDebugRaycast.timer = 0.f;

    if (mRigidBody) {
        mRigidBody->setPosition(shape.getPosition());
        mRigidBody->setVelocity(velocity);
    }

    changeState(PlayerStateType::Idle);
}
//-------------------------------------------------------

//------------[Apply Force - Add Linear Velocity Impulse]-------------------
void Player::applyForce(sf::Vector2f force) {
    velocity += force;
}
//-------------------------------------------------------

//------------[Set Auto Jump - Enable Automatic Bunnyhopping]-------------------
void Player::setAutoJump(bool enabled) {
    mAutoJumpEnabled = enabled;
}
//-------------------------------------------------------

//------------[Is Auto Jump Enabled - Query Bunnyhop State]-------------------
    bool Player::isAutoJumpEnabled() const {
    return mAutoJumpEnabled;
}
//-------------------------------------------------------

//------------[Get Form - Query Active Dual Form]-------------------
PlayerForm Player::getForm() const {
    return mForm;
}
//-------------------------------------------------------

//------------[Set Form - Transition to Witch or Beast Form with Physics Recalculation]-------------------
void Player::setForm(PlayerForm form, const Physics::PhysicsWorld* physicsWorld) {
    if (mForm == form) return;

    // Preserve bottom-center contact point so player doesn't clip into terrain
    sf::Vector2f currentPos = shape.getPosition();
    sf::Vector2f currentSize = shape.getSize();
    float bottomCenterX = currentPos.x + currentSize.x * 0.5f;
    float bottomCenterY = currentPos.y + currentSize.y;

    sf::Vector2f newSize = (form == PlayerForm::Witch) ? sf::Vector2f(30.f, 35.f) : sf::Vector2f(42.f, 40.f);
    sf::Vector2f newPos{bottomCenterX - newSize.x * 0.5f, bottomCenterY - newSize.y};

    // Clearance check when expanding into Beast
    if (physicsWorld && form == PlayerForm::Beast) {
        Physics::AABB targetBox = Physics::AABB::fromPositionSize(newPos, newSize);
        if (physicsWorld->checkOverlap(targetBox, mRigidBody)) {
            // Nudge slightly up to avoid ground interpenetration
            newPos.y -= 2.f;
        }
    }

    shape.setSize(newSize);
    shape.setPosition(newPos);
    mForm = form;

    if (mForm == PlayerForm::Beast) {
        mRage = mMaxRage;
        mRageDecayDelayTimer = 1.5f;
    }

    recalculatePhysicsProperties(physicsWorld);
}
//-------------------------------------------------------

//------------[Toggle Form - Seamlessly Switch Between Witch and Beast Forms]-------------------
void Player::toggleForm(const Physics::PhysicsWorld* physicsWorld) {
    if (mForm == PlayerForm::Witch) {
        setForm(PlayerForm::Beast, physicsWorld);
    } else {
        setForm(PlayerForm::Witch, physicsWorld);
    }
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/transform.wav", 90.f);
}
//-------------------------------------------------------

//------------[Change State - Switch to Specified State Type]-------------------
void Player::changeState(PlayerStateType newType, bool force) {
    if (!force && mCurrentState && mCurrentState->getType() == newType) {
        return;
    }

    if (mCurrentState) {
        mCurrentState->exit(*this);
    }

    switch (newType) {
        case PlayerStateType::Idle:
            mCurrentState = mIdleState.get();
            break;
        case PlayerStateType::Run:
            mCurrentState = mRunState.get();
            break;
        case PlayerStateType::Airborne:
            mCurrentState = mAirborneState.get();
            break;
        case PlayerStateType::Dash:
            mCurrentState = mDashState.get();
            break;
        case PlayerStateType::Pounce:
            mCurrentState = mPounceState.get();
            break;
        case PlayerStateType::MeleeAttack:
            mCurrentState = mMeleeAttackState.get();
            break;
        case PlayerStateType::HeavyStrike:
            mCurrentState = mHeavyStrikeState.get();
            break;
        case PlayerStateType::CastSpell:
            mCurrentState = mCastSpellState.get();
            break;
        case PlayerStateType::Gun:
            mCurrentState = mGunState.get();
            break;
        case PlayerStateType::Parry:
            mCurrentState = mParryState.get();
            break;
        case PlayerStateType::Roar:
            mCurrentState = mRoarState.get();
            break;
    }

    if (mCurrentState) {
        mCurrentState->enter(*this);
    }
}
//-------------------------------------------------------

//------------[Get Current State - Query Active State Pointer]-------------------
PlayerState* Player::getCurrentState() const {
    return mCurrentState;
}
//-------------------------------------------------------

//------------[Get State Type - Query Active Player State Type]-------------------
PlayerStateType Player::getStateType() const {
    return mCurrentState ? mCurrentState->getType() : PlayerStateType::Idle;
}
//-------------------------------------------------------

//------------[Get State Name - Query Human-Readable Active State Name]-------------------
std::string_view Player::getStateName() const {
    return mCurrentState ? mCurrentState->getName() : "None";
}
//-------------------------------------------------------

//------------[Get Position - Query World Space Position]-------------------
sf::Vector2f Player::getPosition() const {
    return shape.getPosition();
}
//-------------------------------------------------------

//------------[Set Position - Update World Coordinates and Hitbox Position]-------------------
void Player::setPosition(sf::Vector2f pos) {
    shape.setPosition(pos);
    if (mRigidBody) {
        mRigidBody->setPosition(pos);
    }
}
//-------------------------------------------------------

//------------[Get Velocity - Query Current Velocity Vector]-------------------
sf::Vector2f Player::getVelocity() const {
    return velocity;
}
//-------------------------------------------------------

//------------[Set Velocity - Update Current Velocity Vector]-------------------
void Player::setVelocity(sf::Vector2f vel) {
    velocity = vel;
    if (mRigidBody) {
        mRigidBody->setVelocity(vel);
    }
}
//-------------------------------------------------------

//------------[Get Bounds - Query World Space FloatRect Bounds]-------------------
sf::FloatRect Player::getBounds() const {
    return shape.getGlobalBounds();
}
//-------------------------------------------------------

//------------[Get AABB - Query Physics AABB Extents]-------------------
Physics::AABB Player::getAABB() const {
    return Physics::AABB::fromPositionSize(shape.getPosition(), shape.getSize());
}
//-------------------------------------------------------

//------------[Get Rigid Body - Query Registered Physics Rigid Body]-------------------
Physics::RigidBody* Player::getRigidBody() const {
    return mRigidBody;
}
//-------------------------------------------------------

//------------[Get Is Grounded - Query Grounded Contact State]-------------------
bool Player::getIsGrounded() const {
    return isGrounded;
}
//-------------------------------------------------------

//------------[Set Is Grounded - Update Ground Contact Flag]-------------------
void Player::setIsGrounded(bool grounded) {
    isGrounded = grounded;
}
//-------------------------------------------------------

//------------[Get Is Dashing - Query Dash Execution State]-------------------
bool Player::getIsDashing() const {
    return isDashing;
}
//-------------------------------------------------------

//------------[Set Is Dashing - Update Dash Execution Flag]-------------------
void Player::setIsDashing(bool dashing) {
    isDashing = dashing;
}
//-------------------------------------------------------

//------------[Get Is Wall Sliding - Query Wall Slide Flag]-------------------
bool Player::getIsWallSliding() const {
    return isWallSliding;
}
//-------------------------------------------------------

//------------[Set Is Wall Sliding - Update Wall Slide State]-------------------
void Player::setIsWallSliding(bool wallSliding) {
    isWallSliding = wallSliding;
}
//-------------------------------------------------------

//------------[Get Has Air Dash - Query Aerial Dash Availability]-------------------
bool Player::getHasAirDash() const {
    return hasAirDash;
}
//-------------------------------------------------------

//------------[Set Has Air Dash - Reset or Consume Aerial Dash]-------------------
void Player::setHasAirDash(bool airDash) {
    hasAirDash = airDash;
}
//-------------------------------------------------------

//------------[Get Dash Cooldown Timer - Query Remaining Dash Cooldown]-------------------
float Player::getDashCooldownTimer() const {
    return dashCooldownTimer;
}
//-------------------------------------------------------

//------------[Set Dash Cooldown Timer - Update Dash Cooldown Value]-------------------
void Player::setDashCooldownTimer(float t) {
    dashCooldownTimer = t;
}
//-------------------------------------------------------

//------------[Get Has Air Jump - Query Double Jump Availability]-------------------
bool Player::getHasAirJump() const {
    return hasAirJump;
}
//-------------------------------------------------------

//------------[Set Has Air Jump - Reset or Consume Double Jump]-------------------
void Player::setHasAirJump(bool airJump) {
    hasAirJump = airJump;
}
//-------------------------------------------------------

//------------[Get Is Jumping - Query Active Jump Upward Phase]-------------------
bool Player::getIsJumping() const {
    return isJumping;
}
//-------------------------------------------------------

//------------[Set Is Jumping - Update Active Jump Flag]-------------------
void Player::setIsJumping(bool jumping) {
    isJumping = jumping;
}
//-------------------------------------------------------

//------------[Is Facing Right - Query Horizontal Facing Direction]-------------------
bool Player::isFacingRight() const {
    return facingRight;
}
//-------------------------------------------------------

//------------[Set Facing Right - Update Horizontal Facing Direction]-------------------
void Player::setFacingRight(bool facing) {
    facingRight = facing;
}
//-------------------------------------------------------

//------------[Get Wall Dir - Query Wall Direction Relative to Player]-------------------
int Player::getWallDir() const {
    return wallDir;
}
//-------------------------------------------------------

//------------[Set Wall Dir - Update Wall Direction Value]-------------------
void Player::setWallDir(int dir) {
    wallDir = dir;
}
//-------------------------------------------------------

//------------[Get Move Speed - Query Form-Specific Base Run Speed]-------------------
float Player::getMoveSpeed() const {
    return moveSpeed;
}
//-------------------------------------------------------

//------------[Get Acceleration - Query Form-Specific Acceleration]-------------------
float Player::getAcceleration() const {
    return acceleration;
}
//-------------------------------------------------------

//------------[Get Friction - Query Form-Specific Ground Friction]-------------------
float Player::getFriction() const {
    if (mIsOnIce) {
        return friction * 0.03f;
    }
    return friction;
}
//-------------------------------------------------------

//------------[Get Gravity - Query Form-Specific Gravity]-------------------
float Player::getGravity() const {
    return gravity;
}
//-------------------------------------------------------

//------------[Get Jump Strength - Query Form-Specific Jump Velocity]-------------------
float Player::getJumpStrength() const {
    return jumpStrength;
}
//-------------------------------------------------------

//------------[Get Wall Slide Speed - Query Standard Wall Slide Terminal Velocity]-------------------
float Player::getWallSlideSpeed() const {
    return wallSlideSpeed;
}
//-------------------------------------------------------

//------------[Get Fast Wall Slide Speed - Query Downward Fast Wall Slide Speed]-------------------
float Player::getFastWallSlideSpeed() const {
    return fastWallSlideSpeed;
}
//-------------------------------------------------------

//------------[Get Wall Jump Force - Query Form-Specific Wall Jump Velocity Vector]-------------------
sf::Vector2f Player::getWallJumpForce() const {
    return wallJumpForce;
}
//-------------------------------------------------------

//------------[Get Dash Speed - Query Form-Specific Dash / Pounce Speed]-------------------
float Player::getDashSpeed() const {
    return dashSpeed;
}
//-------------------------------------------------------

//------------[Get Current Max Speed - Query Decaying High-Velocity Cap]-------------------
float Player::getCurrentMaxSpeed() const {
    return currentMaxSpeed;
}
//-------------------------------------------------------

//------------[Set Current Max Speed - Update Decaying Velocity Cap]-------------------
void Player::setCurrentMaxSpeed(float maxSpeed) {
    currentMaxSpeed = maxSpeed;
}
//-------------------------------------------------------

//------------[Get Speed Decay - Query Form-Specific High-Momentum Decay Rate]-------------------
float Player::getSpeedDecay() const {
    return speedDecay;
}
//-------------------------------------------------------

//------------[Get Coyote Timer - Query Grace Period Jump Timer]-------------------
float Player::getCoyoteTimer() const {
    return coyoteTimer;
}
//-------------------------------------------------------

//------------[Set Coyote Timer - Update Grace Period Jump Timer]-------------------
void Player::setCoyoteTimer(float t) {
    coyoteTimer = t;
}
//-------------------------------------------------------

//------------[Get Jump Buffer Timer - Query Jump Input Buffering Timer]-------------------
float Player::getJumpBufferTimer() const {
    return jumpBufferTimer;
}
//-------------------------------------------------------

//------------[Set Jump Buffer Timer - Update Jump Input Buffering Timer]-------------------
void Player::setJumpBufferTimer(float t) {
    jumpBufferTimer = t;
}
//-------------------------------------------------------

//------------[Get Dash Direction - Query Directional Dash Vector]-------------------
sf::Vector2f Player::getDashDirection() const {
    return dashDirection;
}
//-------------------------------------------------------

//------------[Set Dash Direction - Update Directional Dash Vector]-------------------
void Player::setDashDirection(sf::Vector2f dir) {
    dashDirection = dir;
}
//-------------------------------------------------------

//------------[Get Dash Timer - Query Dash Active Duration Timer]-------------------
float Player::getDashTimer() const {
    return dashTimer;
}
//-------------------------------------------------------

//------------[Set Dash Timer - Update Dash Active Duration Timer]-------------------
void Player::setDashTimer(float t) {
    dashTimer = t;
}
//-------------------------------------------------------

//------------[Get Dash Freeze Timer - Query Dash Initial Freeze Pause]-------------------
float Player::getDashFreezeTimer() const {
    return dashFreezeTimer;
}
//-------------------------------------------------------

//------------[Set Dash Freeze Timer - Update Dash Initial Freeze Pause]-------------------
void Player::setDashFreezeTimer(float t) {
    dashFreezeTimer = t;
}
//-------------------------------------------------------

//------------[Move With Swept CCD - Step Displacement with Continuous Collision Detection]-------------------
void Player::moveWithSweptCCD(sf::Vector2f displacement, const Physics::PhysicsWorld& physicsWorld, bool checkOneWay) {
    (void)checkOneWay;

    if (isDead()) {
        shape.move(displacement);
        if (mRigidBody) mRigidBody->setPosition(shape.getPosition());
        return;
    }

    // 1. Horizontal swept collision
    if (displacement.x != 0.f) {
        sf::Vector2f dispX{displacement.x, 0.f};
        Physics::AABB boxX = getAABB();
        Physics::SweptHit hitX = physicsWorld.sweepTest(boxX, dispX, mRigidBody, false);

        if (hitX.hit) {
            if (hitX.body && (hitX.body->getTag() == Physics::ColliderTag::Hazard || hitX.body->getTag() == Physics::ColliderTag::MothFloor)) {
                triggerHazardDeath();
                return;
            }
            float safeToi = std::max(0.0f, hitX.toi - 0.001f);
            shape.move({dispX.x * safeToi, 0.f});
            velocity.x = 0.f;
        } else {
            shape.move(dispX);
        }
    }

    // 2. Vertical swept collision
    isGrounded = false;
    float prevBottom = shape.getPosition().y + shape.getSize().y;

    if (displacement.y != 0.f) {
        sf::Vector2f dispY{0.f, displacement.y};
        Physics::AABB boxY = getAABB();
        Physics::SweptHit hitY = physicsWorld.sweepTest(boxY, dispY, mRigidBody, false);

        if (hitY.hit) {
            if (hitY.body && (hitY.body->getTag() == Physics::ColliderTag::Hazard || hitY.body->getTag() == Physics::ColliderTag::MothFloor)) {
                triggerHazardDeath();
                return;
            }
            float safeToi = std::max(0.0f, hitY.toi - 0.001f);
            shape.move({0.f, dispY.y * safeToi});

            if (displacement.y > 0.f) {
                if (hitY.body && hitY.body->getTag() == Physics::ColliderTag::TrampolineModifier) {
                    float bounceSpeed = std::max(850.f, std::abs(velocity.y) * 1.5f);
                    velocity.y = -bounceSpeed;
                    isGrounded = false;
                    changeState(PlayerStateType::Airborne);
                    triggerExternalImpulse(0.35f);
                    setHasAirJump(true);
                    setHasAirDash(true);
                    setDashCooldownTimer(0.f);
                    if (mParticleSystem) {
                        mParticleSystem->emitPogoBurst(shape.getPosition() + sf::Vector2f(shape.getSize().x * 0.5f, shape.getSize().y));
                    }
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/pogo_hit.wav", 90.f);
                } else {
                    isGrounded = true;
                    velocity.y = 0.f;
                }
            } else if (displacement.y < 0.f) {
                if (!checkCeilingCornerCorrection(physicsWorld)) {
                    velocity.y = 0.f;
                }
            }
        } else {
            shape.move(dispY);
        }
    }

    std::vector<Physics::RigidBody*> hazardOverlaps = physicsWorld.queryAABB(getAABB(), mRigidBody);
    for (const auto* b : hazardOverlaps) {
        if (b && (b->getTag() == Physics::ColliderTag::Hazard || b->getTag() == Physics::ColliderTag::MothFloor)) {
            triggerHazardDeath();
            return;
        }
    }

    // 3. One-Way platform landing check when descending
    bool dropPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

    if (velocity.y >= 0.f && !dropPressed) {
        float currentBottom = shape.getPosition().y + shape.getSize().y;
        Physics::AABB platformProbe = Physics::AABB::fromPositionSize(
            {shape.getPosition().x, prevBottom - 2.f},
            {shape.getSize().x, (currentBottom - prevBottom) + 6.f});

        std::vector<Physics::RigidBody*> platforms = physicsWorld.queryAABB(platformProbe, mRigidBody);
        for (const auto* platform : platforms) {
            if (platform->isOneWay()) {
                Physics::AABB pBox = platform->getWorldAABB();
                float overlapX = std::min(shape.getPosition().x + shape.getSize().x, pBox.max.x) -
                                 std::max(shape.getPosition().x, pBox.min.x);

                if (overlapX < 4.f) continue;

                if (prevBottom <= pBox.min.y + 4.f && currentBottom >= pBox.min.y - 1.f) {
                    shape.setPosition({shape.getPosition().x, pBox.min.y - shape.getSize().y});
                    velocity.y = 0.f;
                    isGrounded = true;
                    break;
                }
            }
        }
    }

    mIsOnIce = false;
    if (isGrounded) {
        Physics::AABB groundProbe = Physics::AABB::fromPositionSize(
            {shape.getPosition().x + 2.f, shape.getPosition().y + shape.getSize().y - 1.f},
            {shape.getSize().x - 4.f, 5.f}
        );
        std::vector<Physics::RigidBody*> groundBodies = physicsWorld.queryAABB(groundProbe, mRigidBody);
        for (const auto* b : groundBodies) {
            if (b && b->getTag() == Physics::ColliderTag::IceModifier) {
                mIsOnIce = true;
                break;
            }
            if (b && b->getTag() == Physics::ColliderTag::TrampolineModifier) {
                float bounceSpeed = std::max(850.f, std::abs(velocity.y) * 1.5f);
                velocity.y = -bounceSpeed;
                isGrounded = false;
                changeState(PlayerStateType::Airborne);
                triggerExternalImpulse(0.35f);
                setHasAirJump(true);
                setHasAirDash(true);
                setDashCooldownTimer(0.f);
                if (mParticleSystem) {
                    mParticleSystem->emitPogoBurst(shape.getPosition() + sf::Vector2f(shape.getSize().x * 0.5f, shape.getSize().y));
                }
                Engine::Audio::AudioManager::getInstance().playSound("assets/audio/pogo_hit.wav", 90.f);
                break;
            }
        }
    }

    if (mRigidBody) {
        mRigidBody->setPosition(shape.getPosition());
    }
}
//-------------------------------------------------------

//------------[Check Ceiling Corner Correction - Prevent Upward Head Bumping on Ledges]-------------------
bool Player::checkCeilingCornerCorrection(const Physics::PhysicsWorld& physicsWorld) {
    Physics::AABB currentBounds = getAABB();
    Physics::AABB ceilingProbe = Physics::AABB::fromPositionSize(
        {currentBounds.min.x, currentBounds.min.y - 3.f},
        {currentBounds.getSize().x, 3.f}
    );
    if (!physicsWorld.checkOverlap(ceilingProbe, mRigidBody)) {
        return false;
    }

    const float cornerMargin = 8.f;
    Physics::AABB nudgeLeftCeiling = Physics::AABB::fromPositionSize(
        {currentBounds.min.x - cornerMargin, currentBounds.min.y - 3.f},
        {currentBounds.getSize().x, 3.f}
    );
    Physics::AABB nudgeLeftBody = currentBounds.translated({-cornerMargin, 0.f});
    if (!physicsWorld.checkOverlap(nudgeLeftCeiling, mRigidBody) && !physicsWorld.checkOverlap(nudgeLeftBody, mRigidBody)) {
        shape.move({-cornerMargin, 0.f});
        if (mRigidBody) mRigidBody->setPosition(shape.getPosition());
        return true;
    }

    Physics::AABB nudgeRightCeiling = Physics::AABB::fromPositionSize(
        {currentBounds.min.x + cornerMargin, currentBounds.min.y - 3.f},
        {currentBounds.getSize().x, 3.f}
    );
    Physics::AABB nudgeRightBody = currentBounds.translated({cornerMargin, 0.f});
    if (!physicsWorld.checkOverlap(nudgeRightCeiling, mRigidBody) && !physicsWorld.checkOverlap(nudgeRightBody, mRigidBody)) {
        shape.move({cornerMargin, 0.f});
        if (mRigidBody) mRigidBody->setPosition(shape.getPosition());
        return true;
    }

    return false;
}
//-------------------------------------------------------

//------------[Check One Way Drop Through - Check Downward Drop Below Platform]-------------------
void Player::checkOneWayDropThrough(const Physics::PhysicsWorld& physicsWorld) {
    bool dropPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

    if (dropPressed && isGrounded) {
        float bottom = shape.getPosition().y + shape.getSize().y;
        Physics::AABB feetProbe = Physics::AABB::fromPositionSize(
            {shape.getPosition().x + 2.f, bottom - 1.f},
            {shape.getSize().x - 4.f, 4.f});

        std::vector<Physics::RigidBody*> bodies = physicsWorld.queryAABB(feetProbe, mRigidBody);
        for (const auto* b : bodies) {
            if (b->isOneWay()) {
                shape.move({0.f, 3.f});
                isGrounded = false;
                velocity.y = 50.f;
                if (mRigidBody) mRigidBody->setPosition(shape.getPosition());
                changeState(PlayerStateType::Airborne);
                break;
            }
        }
    }
}
//-------------------------------------------------------

//------------[Update Animation - Step Character Sprite Animation Frame]-------------------
void Player::updateAnimation(float dt) {
    if (velocity.x > 1.f) {
        facingRight = true;
    } else if (velocity.x < -1.f) {
        facingRight = false;
    }

    sf::Vector2f bottomCenter = {shape.getPosition().x + shape.getSize().x / 2.f,
                                 shape.getPosition().y + shape.getSize().y};
    sprite.setPosition(bottomCenter);

    float scaleX = (mForm == PlayerForm::Witch) ? 1.5f : 1.8f;
    float scaleY = (mForm == PlayerForm::Witch) ? 1.5f : 1.8f;
    sprite.setScale({facingRight ? scaleX : -scaleX, scaleY});

    if (mForm == PlayerForm::Witch) {
        sprite.setColor(sf::Color::White);
    } else {
        sprite.setColor(sf::Color(255, 140, 120));
    }

    mAnimator.update(dt, &sprite);
}
//-------------------------------------------------------

//------------[Recalculate Physics Properties - Update AABB, Mass, and Constants for Active Form]-------------------
void Player::recalculatePhysicsProperties(const Physics::PhysicsWorld* physicsWorld) {
    (void)physicsWorld;

    if (mForm == PlayerForm::Witch) {
        // High agility, standard mass (1.0 kg)
        mMass = 1.0f;
        moveSpeed = 400.f;
        acceleration = 1600.f;
        friction = 1200.f;
        gravity = 1000.f;
        jumpStrength = 500.f;
        wallSlideSpeed = 150.f;
        fastWallSlideSpeed = 400.f;
        wallJumpForce = {350.f, 500.f};
        dashSpeed = 750.f;
        speedDecay = 700.f;
    } else {
        // Heavy werewolf mass (3.5 kg) with high momentum inheritance
        mMass = 3.5f;
        moveSpeed = 460.f;
        acceleration = 1300.f;
        friction = 800.f;
        gravity = 1300.f;
        jumpStrength = 550.f;
        wallSlideSpeed = 180.f;
        fastWallSlideSpeed = 500.f;
        wallJumpForce = {420.f, 540.f};
        dashSpeed = 750.f;
        speedDecay = 300.f; // Slower momentum decay retains speed longer
    }

    if (currentMaxSpeed < moveSpeed) {
        currentMaxSpeed = moveSpeed;
    }

    if (mRigidBody) {
        mRigidBody->setMass(mMass);
        mRigidBody->setLocalAABB(Physics::AABB::fromPositionSize({0.f, 0.f}, shape.getSize()));
    }
}
//-------------------------------------------------------

//------------[Get Health - Query Current Health Points]-------------------
float Player::getHealth() const {
    return mHealth;
}
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Health Capacity]-------------------
float Player::getMaxHealth() const {
    return mMaxHealth;
}
//-------------------------------------------------------

//------------[Set Health - Update Health Value]-------------------
void Player::setHealth(float hp) {
    mHealth = std::clamp(hp, 0.f, mMaxHealth);
}
//-------------------------------------------------------

//------------[Take Damage - Apply Damage with Invulnerability Frames and Knockback]-------------------
bool Player::takeDamage(float damage, sf::Vector2f knockback) {
    if (mHurtbox.invulnerable || mInvulnerableTimer > 0.f || isDead() || mIsParrying) {
        return false;
    }

    mHealth = std::max(0.f, mHealth - damage);

    if (knockback.x != 0.f || knockback.y != 0.f) {
        velocity = knockback;
        if (mRigidBody) {
            mRigidBody->setVelocity(velocity);
        }
        isGrounded = false;
        isJumping = true;
        changeState(PlayerStateType::Airborne);
        triggerExternalImpulse(0.4f);
    }

    hasAirJump = true;
    hasAirDash = true;
    dashCooldownTimer = 0.f;

    mInvulnerableTimer = 0.5f;
    mHurtbox.invulnerable = true;
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_hurt.wav", 85.f);
    return true;
}
//-------------------------------------------------------

//------------[Heal - Restore Health Points]-------------------
void Player::heal(float amount) {
    mHealth = std::min(mMaxHealth, mHealth + amount);
}
//-------------------------------------------------------

//------------[Is Dead - Check If Player Health Depleted]-------------------
bool Player::isDead() const {
    return mHealth <= 0.f;
}
//-------------------------------------------------------

//------------[Trigger Hazard Death - Instant Lethal Damage on Hazard Contact]-------------------
void Player::triggerHazardDeath() {
    if (isDead()) return;
    mHealth = 0.f;
    velocity = {0.f, -520.f};
    isGrounded = false;
    isJumping = true;
    mInvulnerableTimer = 0.f;
    mHurtbox.invulnerable = false;
    if (mRigidBody) {
        mRigidBody->setVelocity(velocity);
    }
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_hurt.wav", 100.f);
}
//-------------------------------------------------------

//------------[Get Stamina - Query Current Stamina Points]-------------------
float Player::getStamina() const {
    return mStamina;
}
//-------------------------------------------------------

//------------[Get Max Stamina - Query Maximum Stamina Capacity]-------------------
float Player::getMaxStamina() const {
    return mMaxStamina;
}
//-------------------------------------------------------

//------------[Has Stamina - Check If Stamina Gauge Suffices]-------------------
bool Player::hasStamina(float amount) const {
    return mStamina >= amount;
}
//-------------------------------------------------------

//------------[Consume Stamina - Spend Stamina and Trigger Regen Delay]-------------------
bool Player::consumeStamina(float amount) {
    if (mStamina < amount) return false;
    mStamina -= amount;
    mStaminaRegenDelayTimer = 0.8f;
    return true;
}
//-------------------------------------------------------

//------------[Restore Stamina - Replenish Stamina Points]-------------------
void Player::restoreStamina(float amount) {
    mStamina = std::min(mMaxStamina, mStamina + amount);
}
//-------------------------------------------------------

//------------[Deduct Stamina - Apply Resource Penalty and Reset Delay]-------------------
void Player::deductStamina(float amount) {
    mStamina = std::max(0.f, mStamina - amount);
    mStaminaRegenDelayTimer = 1.0f;
}
//-------------------------------------------------------

//------------[Get Mana - Query Current Mana Points]-------------------
float Player::getMana() const {
    return mMana;
}
//-------------------------------------------------------

//------------[Get Max Mana - Query Maximum Mana Capacity]-------------------
float Player::getMaxMana() const {
    return mMaxMana;
}
//-------------------------------------------------------

//------------[Has Mana - Check If Mana Suffices]-------------------
bool Player::hasMana(float amount) const {
    return mMana >= amount;
}
//-------------------------------------------------------

//------------[Consume Mana - Spend Mana Points]-------------------
bool Player::consumeMana(float amount) {
    if (mMana < amount) return false;
    mMana -= amount;
    return true;
}
//-------------------------------------------------------

//------------[Restore Mana - Replenish Mana Points]-------------------
void Player::restoreMana(float amount) {
    mMana = std::min(mMaxMana, mMana + amount);
}
//-------------------------------------------------------

//------------[Get Rage - Query Current Beast Rage]-------------------
float Player::getRage() const {
    return mRage;
}
//-------------------------------------------------------

//------------[Get Max Rage - Query Maximum Beast Rage Capacity]-------------------
float Player::getMaxRage() const {
    return mMaxRage;
}
//-------------------------------------------------------

//------------[Add Rage - Increase Beast Rage Upon Attacks]-------------------
void Player::addRage(float amount) {
    mRage = std::min(mMaxRage, mRage + amount);
    mRageDecayDelayTimer = 1.5f;
}
//-------------------------------------------------------

//------------[Reset Rage Decay Delay - Delay Rage Loss Upon Dealing Damage]-------------------
void Player::resetRageDecayDelay(float delay) {
    mRageDecayDelayTimer = delay;
}
//-------------------------------------------------------

//------------[Trigger External Impulse - Lock Vertical Jump Modifier During Knockback]-------------------
void Player::triggerExternalImpulse(float duration) {
    mExternalImpulseTimer = duration;
}
//-------------------------------------------------------

//------------[Is External Impulse Active - Query Active Knockback Suspension]-------------------
bool Player::isExternalImpulseActive() const {
    return mExternalImpulseTimer > 0.f;
}
//-------------------------------------------------------

//------------[Consume Rage - Spend Beast Rage For Heavy Strikes]-------------------
bool Player::consumeRage(float amount) {
    if (mRage < amount) return false;
    mRage -= amount;
    return true;
}
//-------------------------------------------------------

//------------[Get Hurtbox Const - Access Defensive Box Const]-------------------
const Combat::Hurtbox& Player::getHurtbox() const {
    return mHurtbox;
}
//-------------------------------------------------------

//------------[Get Hurtbox - Access Defensive Box]-------------------
Combat::Hurtbox& Player::getHurtbox() {
    return mHurtbox;
}
//-------------------------------------------------------

//------------[Get Attack Hitbox Const - Access Offensive Box Const]-------------------
const Combat::Hitbox& Player::getAttackHitbox() const {
    return mAttackHitbox;
}
//-------------------------------------------------------

//------------[Get Attack Hitbox - Access Offensive Box]-------------------
Combat::Hitbox& Player::getAttackHitbox() {
    return mAttackHitbox;
}
//-------------------------------------------------------

//------------[Set Attack Hitbox - Activate Offensive Attack Box]-------------------
void Player::setAttackHitbox(const Combat::Hitbox& hitbox) {
    mAttackHitbox = hitbox;
}
//-------------------------------------------------------

//------------[Deactivate Attack Hitbox - Disable Offensive Attack Box]-------------------
void Player::deactivateAttackHitbox() {
    mAttackHitbox.active = false;
}
//-------------------------------------------------------

//------------[Trigger Ground Smash Impact - Register Ground Smash Landing Impact]-------------------
void Player::triggerGroundSmashImpact() {
    mGroundSmashImpact = true;
}
//-------------------------------------------------------

//------------[Consume Ground Smash Impact - Query and Reset Ground Smash Landing Impact]-------------------
bool Player::consumeGroundSmashImpact() {
    bool impact = mGroundSmashImpact;
    mGroundSmashImpact = false;
    return impact;
}
//-------------------------------------------------------

//------------[Get Animator - Access Character Animator]-------------------
Engine::Graphics::Animator& Player::getAnimator() {
    return mAnimator;
}
//-------------------------------------------------------

//------------[Get Animator Const - Access Character Animator Const]-------------------
const Engine::Graphics::Animator& Player::getAnimator() const {
    return mAnimator;
}
//-------------------------------------------------------

//------------[Set Object Manager - Assign Object Manager For Entity Spawning]-------------------
void Player::setObjectManager(ObjectManager* objectManager) {
    mObjectManager = objectManager;
}
//-------------------------------------------------------

//------------[Get Object Manager - Access Object Manager]-------------------
ObjectManager* Player::getObjectManager() const {
    return mObjectManager;
}
//-------------------------------------------------------

//------------[Spawn Projectile - Dispatch Projectile Creation to Object Manager]-------------------
void Player::spawnProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage, float poiseDamage) {
    if (mObjectManager) {
        mObjectManager->spawnProjectile(position, velocity, damage, poiseDamage);
    }
}
//-------------------------------------------------------

//------------[Spawn Pogo Orb - Dispatch Pogo Orb Creation to Object Manager]-------------------
void Player::spawnPogoOrb(sf::Vector2f position, sf::Vector2f velocity, float damage) {
    if (mObjectManager) {
        mObjectManager->spawnPogoOrb(position, velocity, damage);
    }
}
//-------------------------------------------------------

//------------[Spawn Gun Projectile - Dispatch Fast Gun Shot to Object Manager]-------------------
void Player::spawnGunProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage, float poiseDamage) {
    if (mObjectManager) {
        mObjectManager->spawnGunProjectile(position, velocity, damage, poiseDamage);
    }
}
//-------------------------------------------------------

//------------[Spawn Thunder Projectile - Dispatch Fast Lightning Bolt to Object Manager]-------------------
void Player::spawnThunderProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage, float poiseDamage) {
    if (mObjectManager) {
        mObjectManager->spawnThunderProjectile(position, velocity, damage, poiseDamage);
    }
}
//-------------------------------------------------------

//------------[Spawn Ice Wall - Dispatch Ice Wall Creation to Object Manager]-------------------
void Player::spawnIceWall() {
    if (mObjectManager && mPhysicsWorld) {
        sf::Vector2f pPos = getPosition();
        sf::FloatRect bounds = getBounds();
        float spawnX = facingRight ? (pPos.x + bounds.size.x + 32.f) : (pPos.x - 32.f);
        float spawnY = pPos.y + bounds.size.y * 0.45f;
        mObjectManager->spawnIceWall({spawnX, spawnY}, *mPhysicsWorld);
    }
}
//-------------------------------------------------------

//------------[Cast Active Spell - Execute Currently Equipped L Magic Spell]-------------------
void Player::castActiveSpell() {
    if (mForm != PlayerForm::Witch || !canCastSpell()) return;

    if (mActiveSpell == ActiveSpell::PogoOrb) {
        if (!hasMana(30.f)) return;
        consumeMana(30.f);
        setSpellCooldownTimer(0.4f);
        sf::Vector2f pPos = getPosition();
        sf::FloatRect bounds = getBounds();
        float spawnX = facingRight ? (pPos.x + bounds.size.x * 0.7f) : (pPos.x + bounds.size.x * 0.3f);
        float spawnY = pPos.y + bounds.size.y * 0.45f;
        float orbSpeed = facingRight ? 420.f : -420.f;
        spawnPogoOrb({spawnX, spawnY}, {orbSpeed, 0.f}, 55.f);
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/pogo_spawn.wav", 85.f);
    } else if (mActiveSpell == ActiveSpell::IceWall) {
        if (!hasMana(25.f)) return;
        consumeMana(25.f);
        setSpellCooldownTimer(0.5f);
        spawnIceWall();
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/ice_spawn.wav", 85.f);
    } else if (mActiveSpell == ActiveSpell::Thunder) {
        if (!hasMana(20.f)) return;
        consumeMana(20.f);
        setSpellCooldownTimer(0.4f);
        sf::Vector2f pPos = getPosition();
        sf::FloatRect bounds = getBounds();
        float spawnX = facingRight ? (pPos.x + bounds.size.x + 24.f) : (pPos.x - 24.f);
        float spawnY = pPos.y + bounds.size.y * 0.45f;
        spawnThunderProjectile({spawnX, spawnY}, {facingRight ? 1300.f : -1300.f, 0.f}, 25.f, 50.f);
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/thunder_cast.wav", 90.f);
    }
}
//-------------------------------------------------------

//------------[Is Parrying - Query Active Parry Counter Window]-------------------
bool Player::isParrying() const {
    return mIsParrying;
}
//-------------------------------------------------------

//------------[Set Parrying - Update Active Parry Counter Flag]-------------------
void Player::setParrying(bool parrying) {
    mIsParrying = parrying;
}
//-------------------------------------------------------

//------------[Spend Skill Point - Deduct One Upgrade Point]-------------------
bool Player::spendSkillPoint() {
    if (mSkillPoints > 0) {
        --mSkillPoints;
        return true;
    }
    return false;
}
//-------------------------------------------------------

//------------[Upgrade Max Health - Increase Maximum Health and Current Health]-------------------
void Player::upgradeMaxHealth(float amount) {
    mMaxHealth += amount;
    mHealth += amount;
}
//-------------------------------------------------------

//------------[Upgrade Max Mana - Increase Maximum Mana and Current Mana]-------------------
void Player::upgradeMaxMana(float amount) {
    mMaxMana += amount;
    mMana += amount;
}
//-------------------------------------------------------

//------------[Upgrade Max Stamina - Increase Maximum Stamina and Current Stamina]-------------------
void Player::upgradeMaxStamina(float amount) {
    mMaxStamina += amount;
    mStamina += amount;
}
//-------------------------------------------------------

//------------[Apply Melee Hit Recoil - Apply Backward Repulsion on Successful Hit]-------------------
void Player::applyMeleeHitRecoil() {
}
//-------------------------------------------------------

