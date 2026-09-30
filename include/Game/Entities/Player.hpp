#pragma once

#include <SFML/Graphics.hpp>

class Map;

namespace Physics {
  class PhysicsWorld;
}

class Player {
public:
  Player();

  void update(float dt, const Map &map, const Physics::PhysicsWorld &physicsWorld);
  void render(sf::RenderWindow &window, bool showHitbox = false);
  void reset(sf::Vector2f position);
  void applyForce(sf::Vector2f force);

  void setAutoJump(bool enabled);
  bool isAutoJumpEnabled() const;

  sf::Vector2f getPosition() const;
  sf::Vector2f getVelocity() const;
  sf::FloatRect getBounds() const;

  bool getIsGrounded() const;
  bool getIsDashing() const;
  bool getIsWallSliding() const;
  bool getHasAirDash() const;
  float getDashCooldownTimer() const;

private:
  sf::RectangleShape shape;

  sf::Vector2f velocity;
  bool isGrounded;

  float moveSpeed;
  float acceleration;
  float friction;
  float gravity;
  float jumpStrength;

  float wallSlideSpeed;
  float fastWallSlideSpeed;
  sf::Vector2f wallJumpForce;
  bool isWallSliding;
  int wallDir;

  float dashSpeed;
  float dashDuration;
  float dashTimer;
  float dashCooldown;
  float dashCooldownTimer;
  bool isDashing;
  float dashFreezeDuration;
  float dashFreezeTimer;
  sf::Vector2f dashDirection;
  bool hasAirDash;

  bool hasAirJump;
  bool isJumping;

  float jumpBufferTime;
  float jumpBufferTimer;
  bool bufferedJump;

  float coyoteTime;
  float coyoteTimer;

  float currentMaxSpeed;
  float speedDecay;

  sf::Texture texture;
  sf::Sprite sprite;
  bool facingRight;

  enum class AnimState { Idle, WalkStart, RunLoop, Stopping, Jumping, Falling };
  AnimState animState;
  int currentFrame;
  float animationTimer;
  float animationSpeed;
  bool wasMoving;

  bool wasJumpPressed;
  bool mAutoJumpEnabled;
};