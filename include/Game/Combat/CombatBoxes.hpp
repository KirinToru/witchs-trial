#pragma once

#include <Engine/Physics/AABB.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>

namespace Combat {

//------------[Hitbox - Offensive Collision Volume With Damage & Poise]-------------------
struct Hitbox {
    Physics::AABB localBounds{0.f, 0.f, 0.f, 0.f};
    float damage{15.f};
    float poiseDamage{10.f};
    sf::Vector2f knockback{120.f, -80.f};
    bool active{false};

    Physics::AABB getWorldAABB(sf::Vector2f origin) const {
        return Physics::AABB::fromPositionSize(
            origin + localBounds.min,
            localBounds.getSize()
        );
    }
};
//-------------------------------------------------------

//------------[Hurtbox - Defensive Vulnerability Volume With I-Frame Support]-------------------
struct Hurtbox {
    Physics::AABB localBounds{0.f, 0.f, 0.f, 0.f};
    bool active{true};
    bool invulnerable{false};

    Physics::AABB getWorldAABB(sf::Vector2f origin) const {
        return Physics::AABB::fromPositionSize(
            origin + localBounds.min,
            localBounds.getSize()
        );
    }
};
//-------------------------------------------------------

//------------[Check Overlap - Test Intersection Between Active Hitbox and Hurtbox]-------------------
inline bool checkOverlap(const Hitbox& hit, sf::Vector2f hitOrigin, const Hurtbox& hurt, sf::Vector2f hurtOrigin) {
    if (!hit.active || !hurt.active || hurt.invulnerable) {
        return false;
    }
    return hit.getWorldAABB(hitOrigin).intersects(hurt.getWorldAABB(hurtOrigin));
}
//-------------------------------------------------------

} // namespace Combat
