#pragma once

#include <Engine/Physics/AABB.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace Physics {

class RigidBody;

struct SweptHit {
    bool hit = false;
    float toi = 1.0f; // Time of Impact normalized to [0.0, 1.0]
    sf::Vector2f normal{0.f, 0.f};
    sf::Vector2f hitPoint{0.f, 0.f};
    RigidBody* body = nullptr;
};

//------------[Sweep AABB - Continuous Collision Detection for Moving Box against Target Box]-------------------
inline SweptHit sweepAABB(const AABB& movingBox, sf::Vector2f displacement, const AABB& targetBox) {
    SweptHit result;
    result.hit = false;
    result.toi = 1.0f;
    result.normal = {0.f, 0.f};

    // If no displacement, check for instantaneous static overlap
    if (displacement.x == 0.f && displacement.y == 0.f) {
        if (movingBox.intersects(targetBox)) {
            result.hit = true;
            result.toi = 0.0f;

            sf::Vector2f centerA = movingBox.getCenter();
            sf::Vector2f centerB = targetBox.getCenter();
            sf::Vector2f halfA = movingBox.getHalfExtents();
            sf::Vector2f halfB = targetBox.getHalfExtents();
            sf::Vector2f delta = centerA - centerB;

            float overlapX = (halfA.x + halfB.x) - std::abs(delta.x);
            float overlapY = (halfA.y + halfB.y) - std::abs(delta.y);

            if (overlapX < overlapY) {
                result.normal = (delta.x < 0.f) ? sf::Vector2f(-1.f, 0.f) : sf::Vector2f(1.f, 0.f);
            } else {
                result.normal = (delta.y < 0.f) ? sf::Vector2f(0.f, -1.f) : sf::Vector2f(0.f, 1.f);
            }
            result.hitPoint = centerA;
        }
        return result;
    }

    // Minkowski expanded bounding box: expand target by movingBox extents
    sf::Vector2f movingSize = movingBox.getSize();
    sf::Vector2f expandedMin = targetBox.min - movingSize;
    sf::Vector2f expandedMax = targetBox.max;

    sf::Vector2f rayOrigin = movingBox.min;
    sf::Vector2f rayDir = displacement;

    float tNearX, tFarX;
    float tNearY, tFarY;

    // X Axis Slab Check
    if (rayDir.x != 0.f) {
        tNearX = (expandedMin.x - rayOrigin.x) / rayDir.x;
        tFarX = (expandedMax.x - rayOrigin.x) / rayDir.x;
        if (tNearX > tFarX) std::swap(tNearX, tFarX);
    } else {
        if (rayOrigin.x < expandedMin.x || rayOrigin.x > expandedMax.x) {
            return result; // Parallel and outside
        }
        tNearX = -std::numeric_limits<float>::infinity();
        tFarX = std::numeric_limits<float>::infinity();
    }

    // Y Axis Slab Check
    if (rayDir.y != 0.f) {
        tNearY = (expandedMin.y - rayOrigin.y) / rayDir.y;
        tFarY = (expandedMax.y - rayOrigin.y) / rayDir.y;
        if (tNearY > tFarY) std::swap(tNearY, tFarY);
    } else {
        if (rayOrigin.y < expandedMin.y || rayOrigin.y > expandedMax.y) {
            return result; // Parallel and outside
        }
        tNearY = -std::numeric_limits<float>::infinity();
        tFarY = std::numeric_limits<float>::infinity();
    }

    float tEntry = std::max(tNearX, tNearY);
    float tExit = std::min(tFarX, tFarY);

    // No intersection or collision happens behind us / after this step
    if (tEntry > tExit || tExit < 0.0f || tEntry > 1.0f) {
        return result;
    }

    result.hit = true;
    result.toi = std::clamp(tEntry, 0.0f, 1.0f);

    // Compute surface normal from entering axis
    if (tNearX > tNearY) {
        result.normal = (rayDir.x > 0.f) ? sf::Vector2f(-1.f, 0.f) : sf::Vector2f(1.f, 0.f);
    } else {
        result.normal = (rayDir.y > 0.f) ? sf::Vector2f(0.f, -1.f) : sf::Vector2f(0.f, 1.f);
    }

    // Compute contact position
    result.hitPoint = rayOrigin + rayDir * result.toi + movingSize * 0.5f;

    return result;
}
//-------------------------------------------------------

} // namespace Physics
