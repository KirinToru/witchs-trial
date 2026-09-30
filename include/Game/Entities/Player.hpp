#pragma once

#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/RigidBody.hpp>
#include <Game/Combat/CombatBoxes.hpp>
#include <Game/Entities/PlayerState.hpp>
#include <Game/Entities/PlayerStates.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <string_view>

class Map;

namespace Physics {
class PhysicsWorld;
}

class Player {
public:
//------------[Constructor - Initialize Player Components & State Machine]-------------------
    Player();
//-------------------------------------------------------

//------------[Destructor - Cleanup Resources]-------------------
    ~Player();
//-------------------------------------------------------

//------------[Init Physics - Register Player RigidBody in PhysicsWorld]-------------------
    void initPhysics(Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Handle Input - Delegate Input to Active State & Form Toggles]-------------------
    void handleInput(const sf::Event& event);
//-------------------------------------------------------

//------------[Fixed Update - Delegate Fixed Timestep Physics to Active State]-------------------
    void fixedUpdate(float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Update - Backwards-Compatible Forwarder to Fixed Update]-------------------
    void update(float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
        fixedUpdate(dt, map, physicsWorld);
    }
//-------------------------------------------------------

//------------[Render - Draw Animated Character Sprite & Debug Hitbox]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false);
//-------------------------------------------------------

//------------[Reset - Reposition Player at Respawn Point]-------------------
    void reset(sf::Vector2f position);
//-------------------------------------------------------

//------------[Apply Force - Add Linear Velocity Impulse]-------------------
    void applyForce(sf::Vector2f force);
//-------------------------------------------------------

//------------[Set Auto Jump - Enable Automatic Bunnyhopping]-------------------
    void setAutoJump(bool enabled);
//-------------------------------------------------------

//------------[Is Auto Jump Enabled - Query Bunnyhop State]-------------------
    bool isAutoJumpEnabled() const;
//-------------------------------------------------------

//------------[Get Form - Query Active Dual Form]-------------------
    PlayerForm getForm() const;
//-------------------------------------------------------

//------------[Set Form - Transition to Witch or Beast Form with Physics Recalculation]-------------------
    void setForm(PlayerForm form, const Physics::PhysicsWorld* physicsWorld = nullptr);
//-------------------------------------------------------

//------------[Toggle Form - Seamlessly Switch Between Witch and Beast Forms]-------------------
    void toggleForm(const Physics::PhysicsWorld* physicsWorld = nullptr);
//-------------------------------------------------------

//------------[Change State - Switch to Specified State Type]-------------------
    void changeState(PlayerStateType newType);
//-------------------------------------------------------

//------------[Get Current State - Query Active State Pointer]-------------------
    PlayerState* getCurrentState() const;
//-------------------------------------------------------

//------------[Get State Name - Query Human-Readable Active State Name]-------------------
    std::string_view getStateName() const;
//-------------------------------------------------------

//------------[Get Position - Query World Space Position]-------------------
    sf::Vector2f getPosition() const;
//-------------------------------------------------------

//------------[Set Position - Update World Coordinates and Hitbox Position]-------------------
    void setPosition(sf::Vector2f pos);
//-------------------------------------------------------

//------------[Get Velocity - Query Current Velocity Vector]-------------------
    sf::Vector2f getVelocity() const;
//-------------------------------------------------------

//------------[Set Velocity - Update Current Velocity Vector]-------------------
    void setVelocity(sf::Vector2f vel);
//-------------------------------------------------------

//------------[Get Bounds - Query World Space FloatRect Bounds]-------------------
    sf::FloatRect getBounds() const;
//-------------------------------------------------------

//------------[Get AABB - Query Physics AABB Extents]-------------------
    Physics::AABB getAABB() const;
//-------------------------------------------------------

//------------[Get Rigid Body - Query Registered Physics Rigid Body]-------------------
    Physics::RigidBody* getRigidBody() const;
//-------------------------------------------------------

//------------[Get Is Grounded - Query Grounded Contact State]-------------------
    bool getIsGrounded() const;
//-------------------------------------------------------

//------------[Set Is Grounded - Update Ground Contact Flag]-------------------
    void setIsGrounded(bool grounded);
//-------------------------------------------------------

//------------[Get Is Dashing - Query Dash Execution State]-------------------
    bool getIsDashing() const;
//-------------------------------------------------------

//------------[Set Is Dashing - Update Dash Execution Flag]-------------------
    void setIsDashing(bool dashing);
//-------------------------------------------------------

//------------[Get Is Wall Sliding - Query Wall Slide Flag]-------------------
    bool getIsWallSliding() const;
//-------------------------------------------------------

//------------[Set Is Wall Sliding - Update Wall Slide State]-------------------
    void setIsWallSliding(bool wallSliding);
//-------------------------------------------------------

//------------[Get Has Air Dash - Query Aerial Dash Availability]-------------------
    bool getHasAirDash() const;
//-------------------------------------------------------

//------------[Set Has Air Dash - Reset or Consume Aerial Dash]-------------------
    void setHasAirDash(bool airDash);
//-------------------------------------------------------

//------------[Get Dash Cooldown Timer - Query Remaining Dash Cooldown]-------------------
    float getDashCooldownTimer() const;
//-------------------------------------------------------

//------------[Set Dash Cooldown Timer - Update Dash Cooldown Value]-------------------
    void setDashCooldownTimer(float t);
//-------------------------------------------------------

//------------[Get Has Air Jump - Query Double Jump Availability]-------------------
    bool getHasAirJump() const;
//-------------------------------------------------------

//------------[Set Has Air Jump - Reset or Consume Double Jump]-------------------
    void setHasAirJump(bool airJump);
//-------------------------------------------------------

//------------[Get Is Jumping - Query Active Jump Upward Phase]-------------------
    bool getIsJumping() const;
//-------------------------------------------------------

//------------[Set Is Jumping - Update Active Jump Flag]-------------------
    void setIsJumping(bool jumping);
//-------------------------------------------------------

//------------[Is Facing Right - Query Horizontal Facing Direction]-------------------
    bool isFacingRight() const;
//-------------------------------------------------------

//------------[Set Facing Right - Update Horizontal Facing Direction]-------------------
    void setFacingRight(bool facing);
//-------------------------------------------------------

//------------[Get Wall Dir - Query Wall Direction Relative to Player]-------------------
    int getWallDir() const;
//-------------------------------------------------------

//------------[Set Wall Dir - Update Wall Direction Value]-------------------
    void setWallDir(int dir);
//-------------------------------------------------------

//------------[Get Move Speed - Query Form-Specific Base Run Speed]-------------------
    float getMoveSpeed() const;
//-------------------------------------------------------

//------------[Get Acceleration - Query Form-Specific Acceleration]-------------------
    float getAcceleration() const;
//-------------------------------------------------------

//------------[Get Friction - Query Form-Specific Ground Friction]-------------------
    float getFriction() const;
//-------------------------------------------------------

//------------[Get Gravity - Query Form-Specific Gravity]-------------------
    float getGravity() const;
//-------------------------------------------------------

//------------[Get Jump Strength - Query Form-Specific Jump Velocity]-------------------
    float getJumpStrength() const;
//-------------------------------------------------------

//------------[Get Wall Slide Speed - Query Standard Wall Slide Terminal Velocity]-------------------
    float getWallSlideSpeed() const;
//-------------------------------------------------------

//------------[Get Fast Wall Slide Speed - Query Downward Fast Wall Slide Speed]-------------------
    float getFastWallSlideSpeed() const;
//-------------------------------------------------------

//------------[Get Wall Jump Force - Query Form-Specific Wall Jump Velocity Vector]-------------------
    sf::Vector2f getWallJumpForce() const;
//-------------------------------------------------------

//------------[Get Dash Speed - Query Form-Specific Dash / Pounce Speed]-------------------
    float getDashSpeed() const;
//-------------------------------------------------------

//------------[Get Current Max Speed - Query Decaying High-Velocity Cap]-------------------
    float getCurrentMaxSpeed() const;
//-------------------------------------------------------

//------------[Set Current Max Speed - Update Decaying Velocity Cap]-------------------
    void setCurrentMaxSpeed(float maxSpeed);
//-------------------------------------------------------

//------------[Get Speed Decay - Query Form-Specific High-Momentum Decay Rate]-------------------
    float getSpeedDecay() const;
//-------------------------------------------------------

//------------[Get Coyote Timer - Query Grace Period Jump Timer]-------------------
    float getCoyoteTimer() const;
//-------------------------------------------------------

//------------[Set Coyote Timer - Update Grace Period Jump Timer]-------------------
    void setCoyoteTimer(float t);
//-------------------------------------------------------

//------------[Get Jump Buffer Timer - Query Jump Input Buffering Timer]-------------------
    float getJumpBufferTimer() const;
//-------------------------------------------------------

//------------[Set Jump Buffer Timer - Update Jump Input Buffering Timer]-------------------
    void setJumpBufferTimer(float t);
//-------------------------------------------------------

//------------[Get Dash Direction - Query Directional Dash Vector]-------------------
    sf::Vector2f getDashDirection() const;
//-------------------------------------------------------

//------------[Set Dash Direction - Update Directional Dash Vector]-------------------
    void setDashDirection(sf::Vector2f dir);
//-------------------------------------------------------

//------------[Get Dash Timer - Query Dash Active Duration Timer]-------------------
    float getDashTimer() const;
//-------------------------------------------------------

//------------[Set Dash Timer - Update Dash Active Duration Timer]-------------------
    void setDashTimer(float t);
//-------------------------------------------------------

//------------[Get Dash Freeze Timer - Query Dash Initial Freeze Pause]-------------------
    float getDashFreezeTimer() const;
//-------------------------------------------------------

//------------[Set Dash Freeze Timer - Update Dash Initial Freeze Pause]-------------------
    void setDashFreezeTimer(float t);
//-------------------------------------------------------

//------------[Move With Swept CCD - Step Displacement with Continuous Collision Detection]-------------------
    void moveWithSweptCCD(sf::Vector2f displacement, const Physics::PhysicsWorld& physicsWorld, bool checkOneWay);
//-------------------------------------------------------

//------------[Check Ceiling Corner Correction - Prevent Upward Head Bumping on Ledges]-------------------
    bool checkCeilingCornerCorrection(const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Check One Way Drop Through - Check Downward Drop Below Platform]-------------------
    void checkOneWayDropThrough(const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Update Animation - Step Character Sprite Animation Frame]-------------------
    void updateAnimation(float dt);
//-------------------------------------------------------

//------------[Get Health - Query Current Health Points]-------------------
    float getHealth() const;
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Health Capacity]-------------------
    float getMaxHealth() const;
//-------------------------------------------------------

//------------[Set Health - Update Health Value]-------------------
    void setHealth(float hp);
//-------------------------------------------------------

//------------[Take Damage - Apply Damage with Invulnerability Frames and Knockback]-------------------
    void takeDamage(float damage, sf::Vector2f knockback = {0.f, 0.f});
//-------------------------------------------------------

//------------[Heal - Restore Health Points]-------------------
    void heal(float amount);
//-------------------------------------------------------

//------------[Is Dead - Check If Player Health Depleted]-------------------
    bool isDead() const;
//-------------------------------------------------------

//------------[Get Stamina - Query Current Stamina]-------------------
    float getStamina() const;
//-------------------------------------------------------

//------------[Get Max Stamina - Query Maximum Stamina Capacity]-------------------
    float getMaxStamina() const;

//-------------------------------------------------------

//------------[Has Stamina - Check If Stamina Gauge Suffices]-------------------
    bool hasStamina(float amount) const;
//-------------------------------------------------------

//------------[Consume Stamina - Spend Stamina and Trigger Regen Delay]-------------------
    bool consumeStamina(float amount);
//-------------------------------------------------------

//------------[Restore Stamina - Replenish Stamina Points]-------------------
    void restoreStamina(float amount);
//-------------------------------------------------------

//------------[Get Mana - Query Current Mana Points]-------------------
    float getMana() const;
//-------------------------------------------------------

//------------[Get Max Mana - Query Maximum Mana Capacity]-------------------
    float getMaxMana() const;
//-------------------------------------------------------

//------------[Has Mana - Check If Mana Suffices]-------------------
    bool hasMana(float amount) const;
//-------------------------------------------------------

//------------[Consume Mana - Spend Mana Points]-------------------
    bool consumeMana(float amount);
//-------------------------------------------------------

//------------[Restore Mana - Replenish Mana Points]-------------------
    void restoreMana(float amount);
//-------------------------------------------------------

//------------[Get Rage - Query Current Beast Rage]-------------------
    float getRage() const;
//-------------------------------------------------------

//------------[Get Max Rage - Query Maximum Beast Rage Capacity]-------------------
    float getMaxRage() const;
//-------------------------------------------------------

//------------[Add Rage - Increase Beast Rage Upon Attacks]-------------------
    void addRage(float amount);
//-------------------------------------------------------

//------------[Consume Rage - Spend Beast Rage For Heavy Strikes]-------------------
    bool consumeRage(float amount);
//-------------------------------------------------------

//------------[Get Hurtbox - Access Player Defensive Vulnerability Box]-------------------
    const Combat::Hurtbox& getHurtbox() const;
    Combat::Hurtbox& getHurtbox();
//-------------------------------------------------------

//------------[Get Attack Hitbox - Access Offensive Attack Box]-------------------
    const Combat::Hitbox& getAttackHitbox() const;
    Combat::Hitbox& getAttackHitbox();
//-------------------------------------------------------

//------------[Set Attack Hitbox - Activate Offensive Attack Box]-------------------
    void setAttackHitbox(const Combat::Hitbox& hitbox);
//-------------------------------------------------------

//------------[Deactivate Attack Hitbox - Disable Offensive Attack Box]-------------------
    void deactivateAttackHitbox();
//-------------------------------------------------------

private:
//------------[Recalculate Physics Properties - Update AABB, Mass, and Constants for Active Form]-------------------
    void recalculatePhysicsProperties(const Physics::PhysicsWorld* physicsWorld = nullptr);
//-------------------------------------------------------

    sf::RectangleShape shape;
    sf::Vector2f velocity{0.f, 0.f};
    bool isGrounded{false};

    PlayerForm mForm{PlayerForm::Witch};

    // Pre-allocated state machine instances
    std::unique_ptr<PlayerIdleState> mIdleState;
    std::unique_ptr<PlayerRunState> mRunState;
    std::unique_ptr<PlayerAirborneState> mAirborneState;
    std::unique_ptr<PlayerDashState> mDashState;
    std::unique_ptr<PlayerPounceState> mPounceState;
    std::unique_ptr<PlayerMeleeAttackState> mMeleeAttackState;
    std::unique_ptr<PlayerHeavyStrikeState> mHeavyStrikeState;
    PlayerState* mCurrentState{nullptr};

    // Combat Health & Resources
    float mHealth{100.f};
    float mMaxHealth{100.f};
    float mInvulnerableTimer{0.f};

    // Combat Resources (Witch: Stamina & Mana, Beast: Rage)
    float mStamina{100.f};
    float mMaxStamina{100.f};
    float mStaminaRegenRate{35.f};
    float mStaminaRegenDelayTimer{0.f};

    float mMana{100.f};
    float mMaxMana{100.f};

    float mRage{0.f};
    float mMaxRage{100.f};
    float mRageDecayDelayTimer{0.f};
    float mRageDecayRate{10.f};

    // Combat Collision Boxes
    Combat::Hurtbox mHurtbox;
    Combat::Hitbox mAttackHitbox;

    // Physics world registration
    Physics::RigidBody* mRigidBody{nullptr};

    // Form physics settings
    float moveSpeed{400.f};
    float acceleration{1500.f};
    float friction{1200.f};
    float gravity{1000.f};
    float jumpStrength{500.f};
    float wallSlideSpeed{150.f};
    float fastWallSlideSpeed{400.f};
    sf::Vector2f wallJumpForce{350.f, 500.f};
    float dashSpeed{750.f};
    float currentMaxSpeed{400.f};
    float speedDecay{700.f};
    float mMass{1.0f};

    // Mechanics timers & flags
    bool isWallSliding{false};
    int wallDir{0};
    float dashDuration{0.15f};
    float dashTimer{0.f};
    float dashCooldown{0.5f};
    float dashCooldownTimer{0.f};
    bool isDashing{false};
    float dashFreezeDuration{0.07f};
    float dashFreezeTimer{0.f};
    sf::Vector2f dashDirection{0.f, 0.f};
    bool hasAirDash{true};
    bool hasAirJump{false};
    bool isJumping{false};
    float jumpBufferTime{0.1f};
    float jumpBufferTimer{0.f};
    bool bufferedJump{false};
    float coyoteTime{0.1f};
    float coyoteTimer{0.f};

    // Graphics & animation
    sf::Texture texture;
    sf::Sprite sprite;
    bool facingRight{true};

    enum class AnimState { Idle, WalkStart, RunLoop, Stopping, Jumping, Falling };
    AnimState animState{AnimState::Idle};
    int currentFrame{0};
    float animationTimer{0.f};
    float animationSpeed{0.1f};
    bool wasMoving{false};

    bool wasJumpPressed{false};
    bool wasTransformPressed{false};
    float mTransformCooldownTimer{0.f};
    bool mAutoJumpEnabled{false};
};