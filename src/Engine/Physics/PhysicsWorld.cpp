#include <Engine/Physics/PhysicsWorld.hpp>
#include <algorithm>
#include <cmath>

namespace Physics {

//------------[Default Constructor - Standard World Gravity]-------------------
PhysicsWorld::PhysicsWorld()
    : mGravity(0.f, 980.f),
      mVelocityIterations(6),
      mPositionIterations(2) {}
//-------------------------------------------------------

//------------[Parameterized Constructor - Custom World Gravity]-------------------
PhysicsWorld::PhysicsWorld(sf::Vector2f gravity)
    : mGravity(gravity),
      mVelocityIterations(6),
      mPositionIterations(2) {}
//-------------------------------------------------------

//------------[Destructor - Clean Up Resources]-------------------
PhysicsWorld::~PhysicsWorld() = default;
//-------------------------------------------------------

//------------[Create Body - Factory Method for Instantiating Bodies]-------------------
RigidBody* PhysicsWorld::createBody(const RigidBodyDef& def) {
    auto body = std::make_unique<RigidBody>(def);
    RigidBody* ptr = body.get();
    mBodies.push_back(std::move(body));
    return ptr;
}
//-------------------------------------------------------

//------------[Add Body - Register External Unique Body Pointer]-------------------
void PhysicsWorld::addBody(std::unique_ptr<RigidBody> body) {
    if (body) {
        mBodies.push_back(std::move(body));
    }
}
//-------------------------------------------------------

//------------[Remove Body - Deregister Body by Pointer]-------------------
void PhysicsWorld::removeBody(RigidBody* body) {
    std::erase_if(mBodies, [body](const std::unique_ptr<RigidBody>& item) {
        return item.get() == body;
    });
}
//-------------------------------------------------------

//------------[Clear - Remove All Simulated Bodies]-------------------
void PhysicsWorld::clear() {
    mBodies.clear();
}
//-------------------------------------------------------

//------------[Update - Advance Physical Simulation by Step dt]-------------------
void PhysicsWorld::update(float dt) {
    if (dt <= 0.f) return;

    for (const auto& body : mBodies) {
        body->integrateForces(dt, mGravity);
    }

    std::vector<std::pair<RigidBody*, RigidBody*>> pairs;
    broadphase(pairs);

    std::vector<CollisionManifold> manifolds;
    manifolds.reserve(pairs.size());

    for (const auto& [bodyA, bodyB] : pairs) {
        CollisionManifold m = narrowphase(bodyA, bodyB);
        if (m.hasCollision) {
            manifolds.push_back(m);
        }
    }

    for (int iter = 0; iter < mVelocityIterations; ++iter) {
        for (auto& manifold : manifolds) {
            resolveVelocity(manifold);
        }
    }

    for (const auto& body : mBodies) {
        body->integrateVelocity(dt);
    }

    for (int iter = 0; iter < mPositionIterations; ++iter) {
        for (auto& manifold : manifolds) {
            resolvePosition(manifold);
        }
    }
}
//-------------------------------------------------------

//------------[Set Gravity - Update Global Gravity Vector]-------------------
void PhysicsWorld::setGravity(sf::Vector2f gravity) {
    mGravity = gravity;
}
//-------------------------------------------------------

//------------[Get Gravity - Query Current Gravity Vector]-------------------
sf::Vector2f PhysicsWorld::getGravity() const {
    return mGravity;
}
//-------------------------------------------------------

//------------[Set Velocity Iterations - Adjust Solver Precision]-------------------
void PhysicsWorld::setVelocityIterations(int iterations) {
    mVelocityIterations = std::max(1, iterations);
}
//-------------------------------------------------------

//------------[Get Velocity Iterations - Query Solver Iterations]-------------------
int PhysicsWorld::getVelocityIterations() const {
    return mVelocityIterations;
}
//-------------------------------------------------------

//------------[Set Position Iterations - Adjust Projection Iterations]-------------------
void PhysicsWorld::setPositionIterations(int iterations) {
    mPositionIterations = std::max(1, iterations);
}
//-------------------------------------------------------

//------------[Get Position Iterations - Query Projection Iterations]-------------------
int PhysicsWorld::getPositionIterations() const {
    return mPositionIterations;
}
//-------------------------------------------------------

//------------[Get Bodies - Query Collection of Bodies]-------------------
const std::vector<std::unique_ptr<RigidBody>>& PhysicsWorld::getBodies() const {
    return mBodies;
}
//-------------------------------------------------------

//------------[Render Debug - Draw All Collider Outlines]-------------------
void PhysicsWorld::renderDebug(sf::RenderWindow& window) const {
    for (const auto& body : mBodies) {
        AABB aabb = body->getWorldAABB();
        sf::Vector2f size = aabb.getSize();

        sf::RectangleShape rect(size);
        rect.setPosition(aabb.min);
        rect.setFillColor(sf::Color(0, 0, 0, 0));

        if (body->getType() == BodyType::Static) {
            rect.setOutlineColor(sf::Color::Blue);
        } else if (body->getType() == BodyType::Kinematic) {
            rect.setOutlineColor(sf::Color::Yellow);
        } else {
            rect.setOutlineColor(sf::Color::Cyan);
        }

        rect.setOutlineThickness(1.f);
        window.draw(rect);
    }
}
//-------------------------------------------------------

//------------[Broadphase - Pairwise Overlap Filtering]-------------------
void PhysicsWorld::broadphase(std::vector<std::pair<RigidBody*, RigidBody*>>& potentialPairs) {
    const size_t count = mBodies.size();
    for (size_t i = 0; i < count; ++i) {
        RigidBody* a = mBodies[i].get();
        for (size_t j = i + 1; j < count; ++j) {
            RigidBody* b = mBodies[j].get();

            if (a->getType() != BodyType::Dynamic && b->getType() != BodyType::Dynamic) {
                continue;
            }

            if (a->getWorldAABB().intersects(b->getWorldAABB())) {
                potentialPairs.emplace_back(a, b);
            }
        }
    }
}
//-------------------------------------------------------

//------------[Narrowphase - Compute Collision Manifold with Minimum Penetration]-------------------
CollisionManifold PhysicsWorld::narrowphase(RigidBody* a, RigidBody* b) {
    CollisionManifold manifold;
    AABB boxA = a->getWorldAABB();
    AABB boxB = b->getWorldAABB();

    sf::Vector2f centerA = boxA.getCenter();
    sf::Vector2f centerB = boxB.getCenter();
    sf::Vector2f halfA = boxA.getHalfExtents();
    sf::Vector2f halfB = boxB.getHalfExtents();

    sf::Vector2f delta = centerB - centerA;
    float overlapX = (halfA.x + halfB.x) - std::abs(delta.x);
    if (overlapX <= 0.f) return manifold;

    float overlapY = (halfA.y + halfB.y) - std::abs(delta.y);
    if (overlapY <= 0.f) return manifold;

    manifold.hasCollision = true;
    manifold.bodyA = a;
    manifold.bodyB = b;

    if (overlapX < overlapY) {
        manifold.penetration = overlapX;
        manifold.normal = (delta.x < 0.f) ? sf::Vector2f(-1.f, 0.f) : sf::Vector2f(1.f, 0.f);
    } else {
        manifold.penetration = overlapY;
        manifold.normal = (delta.y < 0.f) ? sf::Vector2f(0.f, -1.f) : sf::Vector2f(0.f, 1.f);
    }

    return manifold;
}
//-------------------------------------------------------

//------------[Resolve Velocity - Apply Normal and Friction Impulses]-------------------
void PhysicsWorld::resolveVelocity(CollisionManifold& manifold) {
    RigidBody* a = manifold.bodyA;
    RigidBody* b = manifold.bodyB;

    float totalInvMass = a->getInverseMass() + b->getInverseMass();
    if (totalInvMass <= 0.f) return;

    sf::Vector2f rv = b->getVelocity() - a->getVelocity();
    float velAlongNormal = rv.x * manifold.normal.x + rv.y * manifold.normal.y;
    if (velAlongNormal > 0.f) return;

    float e = std::min(a->getRestitution(), b->getRestitution());
    float j = -(1.0f + e) * velAlongNormal / totalInvMass;
    sf::Vector2f impulse = j * manifold.normal;

    a->applyImpulse(-impulse);
    b->applyImpulse(impulse);

    sf::Vector2f rvTangential = b->getVelocity() - a->getVelocity();
    float vn = rvTangential.x * manifold.normal.x + rvTangential.y * manifold.normal.y;
    sf::Vector2f tangent = rvTangential - vn * manifold.normal;
    float tangentLen = std::hypot(tangent.x, tangent.y);

    if (tangentLen > 0.0001f) {
        tangent /= tangentLen;
        float jt = -(rvTangential.x * tangent.x + rvTangential.y * tangent.y) / totalInvMass;
        float mu = std::sqrt(a->getFriction() * b->getFriction());
        float maxFriction = j * mu;
        jt = std::clamp(jt, -maxFriction, maxFriction);

        sf::Vector2f frictionImpulse = jt * tangent;
        a->applyImpulse(-frictionImpulse);
        b->applyImpulse(frictionImpulse);
    }
}
//-------------------------------------------------------

//------------[Resolve Position - Baumgarte Positional Projection]-------------------
void PhysicsWorld::resolvePosition(CollisionManifold& manifold) {
    RigidBody* a = manifold.bodyA;
    RigidBody* b = manifold.bodyB;

    float totalInvMass = a->getInverseMass() + b->getInverseMass();
    if (totalInvMass <= 0.f) return;

    const float percent = 0.4f;
    const float slop = 0.05f;
    float correctionMag = (std::max(manifold.penetration - slop, 0.0f) / totalInvMass) * percent;
    sf::Vector2f correction = correctionMag * manifold.normal;

    a->setPosition(a->getPosition() - a->getInverseMass() * correction);
    b->setPosition(b->getPosition() + b->getInverseMass() * correction);
}
//-------------------------------------------------------

} // namespace Physics
