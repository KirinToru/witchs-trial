#include <Game/Entities/Projectile.hpp>
#include <Game/Entities/Player.hpp>

//------------[Update - Step Trajectory and World CCD Collision]-------------------
void Projectile::update(float dt, const Physics::PhysicsWorld& physicsWorld, Engine::Graphics::ParticleSystem* particleSystem, Player* player) {
    if (mDead) return;

    mLifetime += dt;
    if (mLifetime >= mMaxLifetime) {
        mDead = true;
        return;
    }

    if (mType == ProjectileType::PogoOrb && !mPogoStruck) {
        mVelocity.y = 0.f;
    }

    if (particleSystem && mType == ProjectileType::PogoOrb) {
        particleSystem->emitPogoTrail(mPosition);
    }

    if (mType == ProjectileType::PogoOrb && player && !mPogoStruck) {
        Physics::AABB playerBox = player->getAABB();
        Physics::AABB orbBox = getAABB();
        bool overlaps = playerBox.intersects(orbBox);

        if (!mHasPlayerExited) {
            if (!overlaps) {
                mHasPlayerExited = true;
            }
        } else {
            if (overlaps) {
                bool meleeActive = player->getStateType() == PlayerStateType::MeleeAttack && player->getAttackHitbox().active;
                if (!meleeActive) {
                    sf::Vector2f pPos = player->getPosition();
                    sf::Vector2f knockback{pPos.x < mPosition.x ? -160.f : 160.f, -280.f};
                    if (player->takeDamage(10.f, knockback)) {
                        player->setHasAirJump(true);
                        player->setHasAirDash(true);
                        player->setDashCooldownTimer(0.f);
                        player->triggerExternalImpulse(0.4f);
                        if (particleSystem) {
                            particleSystem->emitBloodSplatter(pPos + player->getBounds().size * 0.5f, {0.f, -1.f});
                        }
                    }
                }
            }
        }
    }

    sf::Vector2f moveDelta = mVelocity * dt;
    Physics::AABB currentAABB = getAABB();

    for (const auto& body : physicsWorld.getBodies()) {
        if (!body || body->getType() != Physics::BodyType::Static || body->isOneWay()) continue;
        if (currentAABB.intersects(body->getWorldAABB())) {
            mDead = true;
            return;
        }
    }

    Physics::SweptHit hit = physicsWorld.sweepTest(currentAABB, moveDelta, nullptr, false);
    if (hit.hit && hit.toi <= 1.0f && hit.body && hit.body->getType() == Physics::BodyType::Static && !hit.body->isOneWay()) {
        mPosition += moveDelta * hit.toi;
        mDead = true;
        return;
    }

    mPosition += moveDelta;
}
//-------------------------------------------------------
