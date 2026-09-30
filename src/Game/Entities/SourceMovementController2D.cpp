#include <Game/Entities/SourceMovementController2D.hpp>
#include <cmath>
#include <algorithm>

//------------[Default Constructor]-------------------
SourceMovementController2D::SourceMovementController2D() = default;
//-------------------------------------------------------

//------------[Parameterized Constructor]-------------------
SourceMovementController2D::SourceMovementController2D(MovementSettings settings) : settings(settings) {}
//-------------------------------------------------------

//------------[Apply Friction - Dampen Ground Velocity]-------------------
void SourceMovementController2D::applyFriction(sf::Vector2f& velocity, float dt, bool isGrounded, bool jumpedThisFrame) {
    if (!isGrounded || jumpedThisFrame) return;

    float speed = std::abs(velocity.x);
    if (speed < 0.1f) {
        velocity.x = 0.0f;
        return;
    }

    float control = (speed < settings.stopSpeed) ? settings.stopSpeed : speed;
    float drop = control * settings.friction * dt;

    float newSpeed = speed - drop;
    if (newSpeed < 0.f) newSpeed = 0.f;
    newSpeed /= speed;

    velocity.x *= newSpeed;
}
//-------------------------------------------------------

//------------[Accelerate - Apply Directional Wish Acceleration]-------------------
void SourceMovementController2D::accelerate(sf::Vector2f& velocity, sf::Vector2f wishDir, float wishSpeed, float accel, float dt) {
    float currentSpeed = velocity.x * wishDir.x + velocity.y * wishDir.y;
    float addSpeed = wishSpeed - currentSpeed;
    if (addSpeed <= 0.f) return;

    float accelSpeed = accel * dt * wishSpeed;
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }

    velocity.x += accelSpeed * wishDir.x;
    velocity.y += accelSpeed * wishDir.y;
}
//-------------------------------------------------------

//------------[Air Move - Process In-Air Strafe Movement]-------------------
void SourceMovementController2D::airMove(sf::Vector2f& velocity, sf::Vector2f wishDir, float dt) {
    if (wishDir.x != 0.f && velocity.x * wishDir.x < 0.f) {
        velocity.x = -velocity.x;
    }

    float wishSpeed = settings.maxAirSpeed;
    float airCap = settings.maxSpeed * 0.1f; 
    wishSpeed = std::min(wishSpeed, airCap);

    float currentSpeed = velocity.x * wishDir.x;
    float addSpeed = wishSpeed - currentSpeed;

    if (addSpeed <= 0.f) {
        if (std::abs(velocity.x) < settings.bhopSpeedLimit) {
            velocity.x += wishDir.x * 5.0f * dt;
        }
        return;
    }

    float accelSpeed = settings.airAcceleration * dt * wishSpeed;
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }

    velocity.x += accelSpeed * wishDir.x;
}
//-------------------------------------------------------

//------------[Ground Move - Process Ground Friction & Acceleration]-------------------
void SourceMovementController2D::groundMove(sf::Vector2f& velocity, sf::Vector2f wishDir, float dt, bool jumpedThisFrame) {
    applyFriction(velocity, dt, true, jumpedThisFrame);
    accelerate(velocity, wishDir, settings.maxSpeed, settings.groundAcceleration, dt);
}
//-------------------------------------------------------
