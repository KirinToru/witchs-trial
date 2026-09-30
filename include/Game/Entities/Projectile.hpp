#pragma once

#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Game/Combat/CombatBoxes.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

class Projectile {
public:
//------------[Constructor - Initialize Magic Projectile]-------------------
    Projectile(sf::Vector2f position, sf::Vector2f velocity, float damage = 30.f, float poiseDamage = 25.f)
        : mPosition(position),
          mVelocity(velocity),
          mDamage(damage),
          mPoiseDamage(poiseDamage),
          mSize(16.f, 12.f),
          mKnockback(velocity.x > 0.f ? 180.f : -180.f, -40.f) {}
//-------------------------------------------------------

//------------[Update - Step Trajectory and World CCD Collision]-------------------
    void update(float dt, const Physics::PhysicsWorld& physicsWorld) {
        if (mDead) return;

        mLifetime += dt;
        if (mLifetime >= mMaxLifetime) {
            mDead = true;
            return;
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

//------------[Render - Draw Magic Bolt and Hitbox Visualization]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) const {
        if (mDead) return;

        sf::CircleShape aura(10.f);
        aura.setOrigin({10.f, 10.f});
        aura.setPosition(mPosition);
        aura.setFillColor(sf::Color(100, 180, 255, 100));
        window.draw(aura);

        sf::CircleShape core(5.f);
        core.setOrigin({5.f, 5.f});
        core.setPosition(mPosition);
        core.setFillColor(sf::Color(220, 245, 255, 255));
        window.draw(core);

        sf::RectangleShape trail({14.f, 4.f});
        trail.setOrigin({mVelocity.x > 0.f ? 14.f : 0.f, 2.f});
        trail.setPosition(mPosition);
        trail.setFillColor(sf::Color(80, 140, 240, 160));
        window.draw(trail);

        if (showHitbox) {
            Physics::AABB box = getAABB();
            sf::RectangleShape debugBox;
            debugBox.setPosition(box.min);
            debugBox.setSize(box.getSize());
            debugBox.setFillColor(sf::Color(0, 200, 255, 80));
            debugBox.setOutlineColor(sf::Color::Cyan);
            debugBox.setOutlineThickness(1.f);
            window.draw(debugBox);
        }
    }
//-------------------------------------------------------

//------------[Get Position - Query World Position]-------------------
    sf::Vector2f getPosition() const {
        return mPosition;
    }
//-------------------------------------------------------

//------------[Get AABB - Calculate Physics Bounding Box]-------------------
    Physics::AABB getAABB() const {
        return Physics::AABB::fromCenterHalfExtents(mPosition, mSize * 0.5f);
    }
//-------------------------------------------------------

//------------[Get Hitbox - Build Combat Hitbox Volume]-------------------
    Combat::Hitbox getHitbox() const {
        Combat::Hitbox box;
        box.damage = mDamage;
        box.poiseDamage = mPoiseDamage;
        box.knockback = mKnockback;
        box.localBounds = Physics::AABB::fromCenterHalfExtents({0.f, 0.f}, mSize * 0.5f);
        box.active = !mDead;
        return box;
    }
//-------------------------------------------------------

//------------[Get Damage - Query Attack Damage]-------------------
    float getDamage() const {
        return mDamage;
    }
//-------------------------------------------------------

//------------[Get Poise Damage - Query Posture Damage]-------------------
    float getPoiseDamage() const {
        return mPoiseDamage;
    }
//-------------------------------------------------------

//------------[Get Knockback - Query Knockback Vector]-------------------
    sf::Vector2f getKnockback() const {
        return mKnockback;
    }
//-------------------------------------------------------

//------------[Is Dead - Query Alive Status]-------------------
    bool isDead() const {
        return mDead;
    }
//-------------------------------------------------------

//------------[Destroy - Mark Projectile As Dead]-------------------
    void destroy() {
        mDead = true;
    }
//-------------------------------------------------------

private:
    sf::Vector2f mPosition{0.f, 0.f};
    sf::Vector2f mVelocity{0.f, 0.f};
    float mDamage{30.f};
    float mPoiseDamage{25.f};
    sf::Vector2f mSize{16.f, 12.f};
    sf::Vector2f mKnockback{180.f, -40.f};
    float mLifetime{0.f};
    float mMaxLifetime{3.0f};
    bool mDead{false};
};
