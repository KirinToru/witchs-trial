#pragma once

#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

namespace Physics {

struct AABB {
    sf::Vector2f min{0.f, 0.f};
    sf::Vector2f max{0.f, 0.f};

//------------[Default Constructor]-------------------
    AABB() = default;
//-------------------------------------------------------

//------------[Constructor from Min/Max Vectors]-------------------
    AABB(sf::Vector2f minVal, sf::Vector2f maxVal)
        : min(std::min(minVal.x, maxVal.x), std::min(minVal.y, maxVal.y)),
          max(std::max(minVal.x, maxVal.x), std::max(minVal.y, maxVal.y)) {}
//-------------------------------------------------------

//------------[Constructor from Coordinates]-------------------
    AABB(float minX, float minY, float maxX, float maxY)
        : min(std::min(minX, maxX), std::min(minY, maxY)),
          max(std::max(minX, maxX), std::max(minY, maxY)) {}
//-------------------------------------------------------

//------------[Factory: From Center and Half Extents]-------------------
    static AABB fromCenterHalfExtents(sf::Vector2f center, sf::Vector2f halfExtents) {
        sf::Vector2f absHalf(std::abs(halfExtents.x), std::abs(halfExtents.y));
        return AABB(center - absHalf, center + absHalf);
    }
//-------------------------------------------------------

//------------[Factory: From Top-Left Position and Size]-------------------
    static AABB fromPositionSize(sf::Vector2f position, sf::Vector2f size) {
        return AABB(position, position + size);
    }
//-------------------------------------------------------

//------------[Get Center - Calculate Bounding Box Midpoint]-------------------
    sf::Vector2f getCenter() const {
        return (min + max) * 0.5f;
    }
//-------------------------------------------------------

//------------[Get Size - Calculate Dimensions Vector]-------------------
    sf::Vector2f getSize() const {
        return max - min;
    }
//-------------------------------------------------------

//------------[Get Half Extents - Calculate Radial Distances]-------------------
    sf::Vector2f getHalfExtents() const {
        return (max - min) * 0.5f;
    }
//-------------------------------------------------------

//------------[Intersects - Test Overlap with Other AABB]-------------------
    bool intersects(const AABB& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }
//-------------------------------------------------------

//------------[Contains Point - Test if Vector Lies Within Bounds]-------------------
    bool contains(sf::Vector2f point) const {
        return (point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y);
    }
//-------------------------------------------------------

//------------[Contains AABB - Test if Entire Sub-AABB is Inside]-------------------
    bool contains(const AABB& other) const {
        return (other.min.x >= min.x && other.max.x <= max.x &&
                other.min.y >= min.y && other.max.y <= max.y);
    }
//-------------------------------------------------------

//------------[Translated - Return Shifted AABB]-------------------
    AABB translated(sf::Vector2f offset) const {
        return AABB(min + offset, max + offset);
    }
//-------------------------------------------------------

//------------[Expanded - Return Padded AABB]-------------------
    AABB expanded(sf::Vector2f padding) const {
        return AABB(min - padding, max + padding);
    }
//-------------------------------------------------------

//------------[Merged - Compute Enclosing Union AABB]-------------------
    AABB merged(const AABB& other) const {
        return AABB(
            std::min(min.x, other.min.x),
            std::min(min.y, other.min.y),
            std::max(max.x, other.max.x),
            std::max(max.y, other.max.y)
        );
    }
//-------------------------------------------------------

//------------[Swept Broadphase AABB - Compute Motion Bounding Box]-------------------
    AABB getSweptBroadphase(sf::Vector2f velocity, float dt) const {
        sf::Vector2f nextMin = min + velocity * dt;
        sf::Vector2f nextMax = max + velocity * dt;
        return AABB(
            std::min(min.x, nextMin.x),
            std::min(min.y, nextMin.y),
            std::max(max.x, nextMax.x),
            std::max(max.y, nextMax.y)
        );
    }
//-------------------------------------------------------
};

} // namespace Physics
