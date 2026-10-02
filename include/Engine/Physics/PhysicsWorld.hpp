#pragma once

#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/RigidBody.hpp>
#include <Engine/Physics/SweptAABB.hpp>
#include <SFML/Graphics.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace Physics {

struct CollisionManifold {
    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;
    sf::Vector2f normal{0.f, 0.f}; // Points from bodyA to bodyB
    float penetration = 0.f;
    bool hasCollision = false;
};

class PhysicsWorld {
public:
    PhysicsWorld();
    explicit PhysicsWorld(sf::Vector2f gravity);
    ~PhysicsWorld();

    RigidBody* createBody(const RigidBodyDef& def = RigidBodyDef{});
    void addBody(std::unique_ptr<RigidBody> body);
    void removeBody(RigidBody* body);
    void clear();

    void update(float dt);

    void setGravity(sf::Vector2f gravity);
    sf::Vector2f getGravity() const;

    void setVelocityIterations(int iterations);
    int getVelocityIterations() const;

    void setPositionIterations(int iterations);
    int getPositionIterations() const;

    const std::vector<std::unique_ptr<RigidBody>>& getBodies() const;

    SweptHit sweepTest(const AABB& box, sf::Vector2f displacement, const RigidBody* ignoreBody = nullptr, bool checkOneWay = false) const;
//------------[Raycast - Cast Linear Ray Against Physics Bodies]-------------------
    SweptHit raycast(sf::Vector2f start, sf::Vector2f end, const RigidBody* ignoreBody = nullptr, bool checkOneWay = false) const;
//-------------------------------------------------------
    std::vector<RigidBody*> queryAABB(const AABB& aabb, const RigidBody* ignoreBody = nullptr) const;
    bool checkOverlap(const AABB& aabb, const RigidBody* ignoreBody = nullptr) const;

    void renderDebug(sf::RenderWindow& window) const;

private:
    void broadphase(std::vector<std::pair<RigidBody*, RigidBody*>>& potentialPairs);
    CollisionManifold narrowphase(RigidBody* a, RigidBody* b);
    void resolveVelocity(CollisionManifold& manifold);
    void resolvePosition(CollisionManifold& manifold);

    std::vector<std::unique_ptr<RigidBody>> mBodies;
    sf::Vector2f mGravity;
    int mVelocityIterations;
    int mPositionIterations;
};

} // namespace Physics
