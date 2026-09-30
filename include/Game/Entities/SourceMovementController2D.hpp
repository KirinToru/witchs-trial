#pragma once

#include <SFML/System/Vector2.hpp>

struct MovementSettings {
    float maxSpeed = 7.0f;
    float maxAirSpeed = 7.0f;
    float bhopSpeedLimit = 15.0f;
    float groundAcceleration = 6.5f;
    float airAcceleration = 100.0f;
    float friction = 6.0f;
    float stopSpeed = 4.0f;
    float jumpImpulse = 11.0f;
};

class SourceMovementController2D {
public:
    SourceMovementController2D();
    SourceMovementController2D(MovementSettings settings);

    void applyFriction(sf::Vector2f& velocity, float dt, bool isGrounded, bool jumpedThisFrame);
    void accelerate(sf::Vector2f& velocity, sf::Vector2f wishDir, float wishSpeed, float accel, float dt);
    void airMove(sf::Vector2f& velocity, sf::Vector2f wishDir, float dt);
    void groundMove(sf::Vector2f& velocity, sf::Vector2f wishDir, float dt, bool jumpedThisFrame);

    MovementSettings settings;
};
