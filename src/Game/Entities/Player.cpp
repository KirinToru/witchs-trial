#include <Game/Entities/Player.hpp>
#include <Game/World/Map.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

//------------[Constructor - Initialize Player Components & Physics Variables]-------------------
Player::Player()
    : velocity(0.f, 0.f),
      isGrounded(false),
      moveSpeed(400.f),
      acceleration(1500.f),
      friction(1200.f),
      gravity(1000.f),
      jumpStrength(500.f),
      wallSlideSpeed(150.f),
      fastWallSlideSpeed(400.f),
      wallJumpForce(350.f, 500.f),
      isWallSliding(false),
      wallDir(0),
      dashSpeed(750.f),
      dashDuration(0.15f),
      dashTimer(0.f),
      dashCooldown(0.5f),
      dashCooldownTimer(0.f),
      isDashing(false),
      dashFreezeDuration(0.07f),
      dashFreezeTimer(0.f),
      dashDirection(0.f, 0.f),
      hasAirDash(true),
      hasAirJump(false),
      isJumping(false),
      jumpBufferTime(0.1f),
      jumpBufferTimer(0.f),
      bufferedJump(false),
      coyoteTime(0.1f),
      coyoteTimer(0.f),
      currentMaxSpeed(400.f),
      speedDecay(700.f),
      sprite(texture),
      facingRight(true),
      animState(AnimState::Idle),
      currentFrame(0),
      animationTimer(0.f),
      animationSpeed(0.1f),
      wasMoving(false),
      wasJumpPressed(false),
      mAutoJumpEnabled(false) {

  if (!texture.loadFromFile("assets/player/spritesheet.png")) {
    std::cerr << "Failed to load player texture from assets/player/spritesheet.png!" << std::endl;
  }
  sprite.setTexture(texture, true);
  sprite.setTextureRect(sf::IntRect({0, 0}, {32, 32}));
  sprite.setOrigin({16.f, 32.f});

  shape.setFillColor(sf::Color(0, 0, 0, 0));
  shape.setOutlineThickness(1.f);
  shape.setOutlineColor(sf::Color::Green);
  shape.setSize({30.f, 35.f});
  shape.setPosition({100.f, 0.f});
}
//-------------------------------------------------------

//------------[Update - Kinematic Movement & Collision Resolution]-------------------
void Player::update(float dt, const Map &map) {
  if (dashCooldownTimer > 0.f)
    dashCooldownTimer -= dt;

  if (jumpBufferTimer > 0.f)
    jumpBufferTimer -= dt;

  if (isGrounded) {
    coyoteTimer = coyoteTime;
    hasAirDash = true;
    hasAirJump = true;
  } else {
    coyoteTimer -= dt;
  }

  if (currentMaxSpeed > moveSpeed && !isDashing) {
    float currentDecay = isGrounded ? speedDecay : (speedDecay * 0.6f);
    currentMaxSpeed -= currentDecay * dt;
    if (currentMaxSpeed < moveSpeed)
      currentMaxSpeed = moveSpeed;
  }

  bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
  bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
  bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
  bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) ||
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);

  bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
  bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);

  bool jumpJustPressed = jumpPressed && !wasJumpPressed;
  if (jumpJustPressed || (mAutoJumpEnabled && jumpPressed && isGrounded)) {
    jumpBufferTimer = jumpBufferTime;
  }
  wasJumpPressed = jumpPressed;

  if (dashPressed && !isDashing && dashCooldownTimer <= 0.f && (isGrounded || hasAirDash)) {
    isDashing = true;
    dashTimer = dashDuration;
    dashFreezeTimer = dashFreezeDuration;
    dashCooldownTimer = dashCooldown;
    velocity = {0.f, 0.f};
    hasAirDash = false;

    dashDirection = {0.f, 0.f};
    if (up)
      dashDirection.y = -1.f;
    else if (down)
      dashDirection.y = 1.f;
    else if (left)
      dashDirection.x = -1.f;
    else if (right)
      dashDirection.x = 1.f;

    if (dashDirection.x == 0.f && dashDirection.y == 0.f) {
      dashDirection.x = facingRight ? 1.f : -1.f;
    }
  }

  if (isDashing) {
    if (dashFreezeTimer > 0.f) {
      dashFreezeTimer -= dt;
      velocity = {0.f, 0.f};

      sf::Vector2f newDir = {0.f, 0.f};
      if (up)
        newDir.y = -1.f;
      else if (down)
        newDir.y = 1.f;
      else if (left)
        newDir.x = -1.f;
      else if (right)
        newDir.x = 1.f;

      if (newDir.x != 0.f || newDir.y != 0.f) {
        dashDirection = newDir;
      }
    } else {
      dashTimer -= dt;
      velocity = dashDirection * dashSpeed;
      currentMaxSpeed = dashSpeed * 0.8f;

      if (dashTimer <= 0.f) {
        isDashing = false;
        isJumping = false;
        if (dashDirection.y < 0.f)
          velocity.y *= 0.85f;
      }
    }
  } else {
    if (left && !right) {
      velocity.x -= acceleration * dt;
    } else if (right && !left) {
      velocity.x += acceleration * dt;
    } else {
      if (velocity.x > 0.f) {
        velocity.x -= friction * dt;
        if (velocity.x < 0.f)
          velocity.x = 0.f;
      } else if (velocity.x < 0.f) {
        velocity.x += friction * dt;
        if (velocity.x > 0.f)
          velocity.x = 0.f;
      }
    }

    if (velocity.x > currentMaxSpeed)
      velocity.x = currentMaxSpeed;
    if (velocity.x < -currentMaxSpeed)
      velocity.x = -currentMaxSpeed;

    sf::FloatRect bounds = shape.getGlobalBounds();
    sf::FloatRect leftCheck = bounds;
    leftCheck.position.x -= 2.f;
    sf::FloatRect rightCheck = bounds;
    rightCheck.position.x += 2.f;

    bool touchingLeft = !map.checkCollision(leftCheck).empty();
    bool touchingRight = !map.checkCollision(rightCheck).empty();

    isWallSliding = false;
    wallDir = 0;

    if (touchingLeft)
      wallDir = -1;
    if (touchingRight)
      wallDir = 1;

    if (wallDir != 0 && velocity.y > 0.f && !isGrounded) {
      if ((wallDir == -1 && left) || (wallDir == 1 && right)) {
        isWallSliding = true;
        if (down) {
          velocity.y = fastWallSlideSpeed;
        } else {
          velocity.y = wallSlideSpeed;
        }
      }
    }

    if (jumpBufferTimer > 0.f) {
      if (coyoteTimer > 0.f) {
        velocity.y = -jumpStrength;
        hasAirDash = true;
        isJumping = true;
        coyoteTimer = 0.f;
        jumpBufferTimer = 0.f;
        hasAirJump = false;
      } else if (isWallSliding || (wallDir != 0 && !isGrounded)) {
        velocity.y = -wallJumpForce.y;
        velocity.x = -static_cast<float>(wallDir) * wallJumpForce.x;
        hasAirDash = true;
        isJumping = true;
        jumpBufferTimer = 0.f;
        hasAirJump = false;
      } else if (hasAirJump) {
        velocity.y = -jumpStrength;
        isJumping = true;
        jumpBufferTimer = 0.f;
        hasAirJump = false;
      }
    }

    float currentGravity = gravity;
    const float peakThreshold = 50.f;
    if (std::abs(velocity.y) < peakThreshold && !isGrounded && !isWallSliding) {
      currentGravity *= 0.7f;
    } else if (velocity.y < 0.f && (!jumpPressed || !isJumping)) {
      currentGravity *= 2.0f;
    } else if (velocity.y > 0.f) {
      if (!isWallSliding) {
        currentGravity *= 1.8f;
      } else {
        currentGravity = 0.f;
      }
    }

    velocity.y += currentGravity * dt;
  }

  // --- X-Axis Movement & Collision ---
  shape.move({velocity.x * dt, 0.f});

  std::vector<sf::FloatRect> walls = map.checkCollision(shape.getGlobalBounds());
  for (const auto &wall : walls) {
    sf::FloatRect playerBounds = shape.getGlobalBounds();
    float overlapY = std::min(playerBounds.position.y + playerBounds.size.y,
                              wall.position.y + wall.size.y) -
                     std::max(playerBounds.position.y, wall.position.y);

    if (overlapY < 5.f)
      continue;

    float playerCenter = shape.getPosition().x + shape.getSize().x / 2.f;
    float wallCenter = wall.position.x + wall.size.x / 2.f;

    if (velocity.x > 0.f) {
      if (wallCenter > playerCenter) {
        shape.setPosition({wall.position.x - shape.getSize().x, shape.getPosition().y});
        velocity.x = 0.f;
      }
    } else if (velocity.x < 0.f) {
      if (wallCenter < playerCenter) {
        shape.setPosition({wall.position.x + wall.size.x, shape.getPosition().y});
        velocity.x = 0.f;
      }
    }
  }

  // --- Y-Axis Movement & Collision ---
  isGrounded = false;
  float prevBottom = shape.getPosition().y + shape.getSize().y;
  shape.move({0.f, velocity.y * dt});

  walls = map.checkCollision(shape.getGlobalBounds());
  for (const auto &wall : walls) {
    sf::FloatRect playerBounds = shape.getGlobalBounds();
    float overlapX = std::min(playerBounds.position.x + playerBounds.size.x,
                              wall.position.x + wall.size.x) -
                     std::max(playerBounds.position.x, wall.position.x);

    if (overlapX < 2.f)
      continue;

    if (velocity.y > 0.f) {
      if (prevBottom > wall.position.y + 15.f)
        continue;

      shape.setPosition({shape.getPosition().x, wall.position.y - shape.getSize().y});
      velocity.y = 0.f;
      isGrounded = true;
    } else if (velocity.y < 0.f) {
      const float cornerMargin = 6.f;
      sf::FloatRect nudgeLeft = playerBounds;
      nudgeLeft.position.x -= cornerMargin;
      if (map.checkCollision(nudgeLeft).empty()) {
        shape.move({-cornerMargin, 0.f});
      } else {
        sf::FloatRect nudgeRight = playerBounds;
        nudgeRight.position.x += cornerMargin;
        if (map.checkCollision(nudgeRight).empty()) {
          shape.move({cornerMargin, 0.f});
        } else {
          shape.setPosition({shape.getPosition().x, wall.position.y + wall.size.y});
          velocity.y = 0.f;
        }
      }
    }
  }

  // --- One-Way Platforms ---
  bool dropPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) ||
                     sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);

  if (velocity.y >= 0.f && !dropPressed) {
    std::vector<sf::FloatRect> platforms = map.checkPlatformCollision(shape.getGlobalBounds());
    for (const auto &platform : platforms) {
      sf::FloatRect playerBounds = shape.getGlobalBounds();
      float overlapX = std::min(playerBounds.position.x + playerBounds.size.x,
                                platform.position.x + platform.size.x) -
                       std::max(playerBounds.position.x, platform.position.x);

      if (overlapX < 4.f)
        continue;

      if (prevBottom <= platform.position.y + 4.f) {
        shape.setPosition({shape.getPosition().x, platform.position.y - shape.getSize().y});
        velocity.y = 0.f;
        isGrounded = true;
        break;
      }
    }
  }

  // --- Animation State Machine ---
  bool isMoving = std::abs(velocity.x) > 10.f;
  bool inputActive = left || right;

  if (isGrounded && !isWallSliding) {
    if (!isMoving && !inputActive) {
      if (animState != AnimState::Idle && animState != AnimState::Stopping) {
        animState = AnimState::Stopping;
        currentFrame = 0;
        animationTimer = 0.f;
      } else if (animState == AnimState::Stopping) {
        animationTimer += dt;
        float stopSpeed = 0.06f;
        if (animationTimer >= stopSpeed) {
          animationTimer = 0.f;
          currentFrame = (currentFrame == 0) ? 1 : 0;
        }
        if (std::abs(velocity.x) < 5.f) {
          animState = AnimState::Idle;
          currentFrame = 0;
          animationTimer = 0.f;
        }
        sprite.setTextureRect(sf::IntRect({currentFrame * 32, 96}, {32, 32}));
      } else {
        animationTimer += dt;
        float frameDelay = (currentFrame == 0) ? 2.0f : 0.5f;
        if (animationTimer >= frameDelay) {
          animationTimer = 0.f;
          currentFrame = (currentFrame + 1) % 2;
        }
        sprite.setTextureRect(sf::IntRect({currentFrame * 32, 0}, {32, 32}));
      }
    } else if (inputActive && isMoving) {
      if (animState == AnimState::Idle || animState == AnimState::Stopping || !wasMoving) {
        animState = AnimState::WalkStart;
        currentFrame = 0;
        animationTimer = 0.f;
      }
      animationTimer += dt;

      if (animState == AnimState::WalkStart) {
        float walkSpeed = 0.15f;
        if (animationTimer >= walkSpeed) {
          animationTimer = 0.f;
          currentFrame++;
          if (currentFrame >= 2) {
            animState = AnimState::RunLoop;
            currentFrame = 0;
          }
        }
        sprite.setTextureRect(sf::IntRect({currentFrame * 32, 32}, {32, 32}));
      } else {
        float runSpeed = 0.1f;
        if (animationTimer >= runSpeed) {
          animationTimer = 0.f;
          currentFrame = (currentFrame + 1) % 4;
        }
        sprite.setTextureRect(sf::IntRect({currentFrame * 32, 64}, {32, 32}));
      }
    } else {
      if (animState != AnimState::Stopping) {
        animState = AnimState::Stopping;
        currentFrame = 0;
        animationTimer = 0.f;
      }
      animationTimer += dt;
      float stopSpeed = 0.1f;
      if (animationTimer >= stopSpeed) {
        animationTimer = 0.f;
        currentFrame = (currentFrame == 0) ? 1 : 0;
      }
      sprite.setTextureRect(sf::IntRect({currentFrame * 32, 96}, {32, 32}));
    }
  } else {
    float airAnimSpeed = 0.1f;
    if (velocity.y < 0.f) {
      if (animState != AnimState::Jumping) {
        animState = AnimState::Jumping;
        currentFrame = 0;
        animationTimer = 0.f;
      }
      animationTimer += dt;
      if (animationTimer >= airAnimSpeed) {
        animationTimer = 0.f;
        currentFrame = (currentFrame + 1) % 2;
      }
      sprite.setTextureRect(sf::IntRect({currentFrame * 32, 128}, {32, 32}));
    } else {
      if (animState != AnimState::Falling) {
        animState = AnimState::Falling;
        currentFrame = 0;
        animationTimer = 0.f;
      }
      sprite.setTextureRect(sf::IntRect({0, 160}, {32, 32}));
    }
  }

  wasMoving = isMoving;
  sf::Vector2f bottomCenter = {shape.getPosition().x + shape.getSize().x / 2.f,
                               shape.getPosition().y + shape.getSize().y};
  sprite.setPosition(bottomCenter);

  if (velocity.x > 1.f) {
    facingRight = true;
  } else if (velocity.x < -1.f) {
    facingRight = false;
  }

  if (facingRight) {
    sprite.setScale({1.5f, 1.5f});
  } else {
    sprite.setScale({-1.5f, 1.5f});
  }
}
//-------------------------------------------------------

//------------[Render - Draw Player Sprite & Debug Hitbox]-------------------
void Player::render(sf::RenderWindow &window, bool showHitbox) {
  window.draw(sprite);
  if (showHitbox) {
    sf::RectangleShape hitboxVis = shape;
    hitboxVis.setFillColor(sf::Color(0, 255, 0, 100));
    hitboxVis.setOutlineColor(sf::Color::Green);
    hitboxVis.setOutlineThickness(1.f);
    window.draw(hitboxVis);
  }
}
//-------------------------------------------------------

//------------[Reset - Position & State Reset]-------------------
void Player::reset(sf::Vector2f position) {
  shape.setPosition({position.x - shape.getSize().x / 2.f,
                     position.y - shape.getSize().y / 2.f});
  velocity = {0.f, 0.f};
  isGrounded = false;
  isDashing = false;
  isWallSliding = false;
  currentMaxSpeed = moveSpeed;
}
//-------------------------------------------------------

//------------[Apply Force - Add Velocity Impulse]-------------------
void Player::applyForce(sf::Vector2f force) {
  velocity += force;
}
//-------------------------------------------------------

//------------[Set Auto Jump - Enable or Disable Automatic Jump]-------------------
void Player::setAutoJump(bool enabled) {
  mAutoJumpEnabled = enabled;
}
//-------------------------------------------------------

//------------[Is Auto Jump Enabled - Query Automatic Jump State]-------------------
bool Player::isAutoJumpEnabled() const {
  return mAutoJumpEnabled;
}
//-------------------------------------------------------

//------------[Get Position - Query World Position]-------------------
sf::Vector2f Player::getPosition() const {
  return shape.getPosition();
}
//-------------------------------------------------------

//------------[Get Velocity - Query Current Velocity]-------------------
sf::Vector2f Player::getVelocity() const {
  return velocity;
}
//-------------------------------------------------------

//------------[Get Bounds - Query Global Bounding Rectangle]-------------------
sf::FloatRect Player::getBounds() const {
  return shape.getGlobalBounds();
}
//-------------------------------------------------------

//------------[Get Is Grounded - Query Ground State]-------------------
bool Player::getIsGrounded() const {
  return isGrounded;
}
//-------------------------------------------------------

//------------[Get Is Dashing - Query Dash State]-------------------
bool Player::getIsDashing() const {
  return isDashing;
}
//-------------------------------------------------------

//------------[Get Is Wall Sliding - Query Wall Slide State]-------------------
bool Player::getIsWallSliding() const {
  return isWallSliding;
}
//-------------------------------------------------------

//------------[Get Has Air Dash - Query Air Dash Availability]-------------------
bool Player::getHasAirDash() const {
  return hasAirDash;
}
//-------------------------------------------------------

//------------[Get Dash Cooldown Timer - Query Dash Cooldown Value]-------------------
float Player::getDashCooldownTimer() const {
  return dashCooldownTimer;
}
//-------------------------------------------------------
