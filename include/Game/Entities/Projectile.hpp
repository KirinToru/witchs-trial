#pragma once

#include <Engine/Graphics/ParticleSystem.hpp>
#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Game/Combat/CombatBoxes.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>

enum class ProjectileType {
    Standard,
    Gun,
    PogoOrb,
    Thunder
};

class Projectile {
public:
//------------[Constructor - Initialize Magic Projectile with Type Classification]-------------------
    Projectile(sf::Vector2f position, sf::Vector2f velocity, float damage = 30.f, float poiseDamage = 25.f, ProjectileType type = ProjectileType::Standard)
        : mPosition(position),
          mVelocity(type == ProjectileType::PogoOrb ? sf::Vector2f{velocity.x, 0.f} : velocity),
          mDamage(damage),
          mPoiseDamage(poiseDamage),
          mType(type),
          mSize(type == ProjectileType::PogoOrb ? sf::Vector2f{36.f, 36.f} : (type == ProjectileType::Gun ? sf::Vector2f{20.f, 8.f} : (type == ProjectileType::Thunder ? sf::Vector2f{24.f, 10.f} : sf::Vector2f{16.f, 12.f}))),
          mKnockback(velocity.x > 0.f ? 180.f : -180.f, -40.f),
          mMaxLifetime(type == ProjectileType::PogoOrb ? 6.0f : (type == ProjectileType::Gun ? 1.8f : (type == ProjectileType::Thunder ? 1.2f : 3.0f))) {}
//-------------------------------------------------------

//------------[Update - Step Trajectory and World CCD Collision]-------------------
    void update(float dt, const Physics::PhysicsWorld& physicsWorld, Engine::Graphics::ParticleSystem* particleSystem = nullptr) {
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

//------------[Render - Draw Magic Bolt, Gun Shot or Pogo Orb and Hitbox Visualization]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) const {
        if (mDead) return;

        if (mType == ProjectileType::PogoOrb) {
            float pulse = 1.0f + 0.12f * std::sin(mLifetime * 8.f);
            sf::CircleShape outerAura(18.f * pulse);
            outerAura.setOrigin({18.f * pulse, 18.f * pulse});
            outerAura.setPosition(mPosition);
            if (mPogoStruck) {
                outerAura.setFillColor(sf::Color(255, 140, 30, 200));
            } else {
                outerAura.setFillColor(sf::Color(255, 75, 15, 160));
            }
            window.draw(outerAura);

            sf::CircleShape innerCore(11.f);
            innerCore.setOrigin({11.f, 11.f});
            innerCore.setPosition(mPosition);
            if (mPogoStruck) {
                innerCore.setFillColor(sf::Color(255, 255, 220, 255));
            } else {
                innerCore.setFillColor(sf::Color(255, 215, 60, 245));
            }
            window.draw(innerCore);

            sf::CircleShape ring(15.f);
            ring.setOrigin({15.f, 15.f});
            ring.setPosition(mPosition);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineColor(mPogoStruck ? sf::Color(255, 230, 90, 240) : sf::Color(255, 120, 20, 220));
            ring.setOutlineThickness(2.f);
            window.draw(ring);
        } else if (mType == ProjectileType::Gun) {
            sf::CircleShape aura(8.f);
            aura.setOrigin({8.f, 8.f});
            aura.setPosition(mPosition);
            aura.setFillColor(sf::Color(255, 200, 80, 180));
            window.draw(aura);

            sf::CircleShape core(4.f);
            core.setOrigin({4.f, 4.f});
            core.setPosition(mPosition);
            core.setFillColor(sf::Color(255, 255, 230, 255));
            window.draw(core);

            sf::RectangleShape streak({22.f, 3.f});
            streak.setOrigin({mVelocity.x > 0.f ? 22.f : 0.f, 1.5f});
            streak.setPosition(mPosition);
            streak.setFillColor(sf::Color(255, 160, 40, 200));
            window.draw(streak);
        } else if (mType == ProjectileType::Thunder) {
            sf::CircleShape aura(12.f);
            aura.setOrigin({12.f, 12.f});
            aura.setPosition(mPosition);
            aura.setFillColor(sf::Color(80, 220, 255, 160));
            window.draw(aura);

            sf::CircleShape core(6.f);
            core.setOrigin({6.f, 6.f});
            core.setPosition(mPosition);
            core.setFillColor(sf::Color(255, 255, 180, 255));
            window.draw(core);

            sf::RectangleShape lightningBolt({26.f, 4.f});
            lightningBolt.setOrigin({mVelocity.x > 0.f ? 26.f : 0.f, 2.f});
            lightningBolt.setPosition(mPosition);
            lightningBolt.setFillColor(sf::Color(140, 240, 255, 240));
            window.draw(lightningBolt);
        } else {
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
        }

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

//------------[Get Velocity - Query Movement Vector]-------------------
    sf::Vector2f getVelocity() const {
        return mVelocity;
    }
//-------------------------------------------------------

//------------[Get Type - Query Projectile Classification Type]-------------------
    ProjectileType getType() const {
        return mType;
    }
//-------------------------------------------------------

//------------[Is Pogo Orb - Query If Projectile Is A Pogo Orb]-------------------
    bool isPogoOrb() const {
        return mType == ProjectileType::PogoOrb;
    }
//-------------------------------------------------------

//------------[Is Pogo Struck - Query If Pogo Orb Has Been Hit]-------------------
    bool isPogoStruck() const {
        return mPogoStruck;
    }
//-------------------------------------------------------

//------------[Strike Pogo - Launch Pogo Orb Forward Upon Melee Strike]-------------------
    void strikePogo(float launchSpeedX) {
        mPogoStruck = true;
        mVelocity = {launchSpeedX, -40.f};
        mDamage = 75.f;
        mPoiseDamage = 60.f;
        mKnockback = {launchSpeedX > 0.f ? 350.f : -350.f, -80.f};
        mLifetime = 0.f;
        mMaxLifetime = 2.5f;
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
    ProjectileType mType{ProjectileType::Standard};
    sf::Vector2f mSize{16.f, 12.f};
    sf::Vector2f mKnockback{180.f, -40.f};
    float mLifetime{0.f};
    float mMaxLifetime{3.0f};
    bool mDead{false};
    bool mPogoStruck{false};
};
