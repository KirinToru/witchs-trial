#pragma once

#include <Engine/Physics/AABB.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Engine/Physics/RigidBody.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>

class IceWall {
public:
//------------[Constructor - Initialize Ice Wall RigidBody and Visuals]-------------------
    IceWall(Physics::PhysicsWorld& world, sf::Vector2f center, sf::Vector2f size = {28.f, 96.f})
        : mWorld(&world), mPosition(center), mSize(size) {
        Physics::RigidBodyDef def;
        def.type = Physics::BodyType::Static;
        def.tag = Physics::ColliderTag::SolidWall;
        def.isOneWay = false;
        def.position = center;
        def.localAABB = Physics::AABB::fromCenterHalfExtents({0.f, 0.f}, size * 0.5f);
        mBody = mWorld->createBody(def);
    }
//-------------------------------------------------------

//------------[Destructor - Cleanup RigidBody From Physics World]-------------------
    ~IceWall() {
        if (mWorld && mBody) {
            mWorld->removeBody(mBody);
            mBody = nullptr;
        }
    }
//-------------------------------------------------------

    IceWall(const IceWall&) = delete;
    IceWall& operator=(const IceWall&) = delete;

//------------[Move Constructor - Transfer Ownership of Physics Body]-------------------
    IceWall(IceWall&& other) noexcept
        : mWorld(other.mWorld),
          mBody(other.mBody),
          mPosition(other.mPosition),
          mSize(other.mSize),
          mLifetime(other.mLifetime),
          mMaxLifetime(other.mMaxLifetime),
          mDead(other.mDead) {
        other.mBody = nullptr;
        other.mWorld = nullptr;
    }
//-------------------------------------------------------

//------------[Move Assignment - Transfer Ownership of Physics Body]-------------------
    IceWall& operator=(IceWall&& other) noexcept {
        if (this != &other) {
            if (mWorld && mBody) {
                mWorld->removeBody(mBody);
            }
            mWorld = other.mWorld;
            mBody = other.mBody;
            mPosition = other.mPosition;
            mSize = other.mSize;
            mLifetime = other.mLifetime;
            mMaxLifetime = other.mMaxLifetime;
            mDead = other.mDead;
            other.mBody = nullptr;
            other.mWorld = nullptr;
        }
        return *this;
    }
//-------------------------------------------------------

//------------[Update - Step Lifetime and Destruction Timer]-------------------
    void update(float dt) {
        if (mDead) return;
        mLifetime += dt;
        if (mLifetime >= mMaxLifetime) {
            destroy();
        }
    }
//-------------------------------------------------------

//------------[Render - Draw Crystalline Translucent Ice Pillar]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) const {
        if (mDead) return;

        sf::RectangleShape outerPillar(mSize);
        outerPillar.setOrigin(mSize * 0.5f);
        outerPillar.setPosition(mPosition);
        outerPillar.setFillColor(sf::Color(120, 210, 255, 190));
        outerPillar.setOutlineColor(sf::Color(220, 245, 255, 240));
        outerPillar.setOutlineThickness(2.f);
        window.draw(outerPillar);

        sf::Vector2f innerSize = {mSize.x - 8.f, mSize.y - 12.f};
        sf::RectangleShape innerCore(innerSize);
        innerCore.setOrigin(innerSize * 0.5f);
        innerCore.setPosition(mPosition);
        innerCore.setFillColor(sf::Color(180, 235, 255, 140));
        window.draw(innerCore);

        sf::RectangleShape facet({innerSize.x * 0.4f, innerSize.y * 0.7f});
        facet.setOrigin({innerSize.x * 0.2f, innerSize.y * 0.35f});
        facet.setPosition({mPosition.x - 2.f, mPosition.y});
        facet.setFillColor(sf::Color(255, 255, 255, 100));
        window.draw(facet);

        if (showHitbox) {
            Physics::AABB box = getAABB();
            sf::RectangleShape debugBox;
            debugBox.setPosition(box.min);
            debugBox.setSize(box.getSize());
            debugBox.setFillColor(sf::Color(0, 220, 255, 60));
            debugBox.setOutlineColor(sf::Color::Cyan);
            debugBox.setOutlineThickness(1.f);
            window.draw(debugBox);
        }
    }
//-------------------------------------------------------

//------------[Get AABB - Calculate World Space Bounding Box]-------------------
    Physics::AABB getAABB() const {
        return Physics::AABB::fromCenterHalfExtents(mPosition, mSize * 0.5f);
    }
//-------------------------------------------------------

//------------[Get Position - Query Center Position]-------------------
    sf::Vector2f getPosition() const {
        return mPosition;
    }
//-------------------------------------------------------

//------------[Get Size - Query Ice Pillar Dimensions]-------------------
    sf::Vector2f getSize() const {
        return mSize;
    }
//-------------------------------------------------------

//------------[Is Dead - Query Destruction Status]-------------------
    bool isDead() const {
        return mDead;
    }
//-------------------------------------------------------

//------------[Destroy - Mark As Dead and Remove RigidBody]-------------------
    void destroy() {
        if (!mDead) {
            mDead = true;
            if (mWorld && mBody) {
                mWorld->removeBody(mBody);
                mBody = nullptr;
            }
        }
    }
//-------------------------------------------------------

private:
    Physics::PhysicsWorld* mWorld{nullptr};
    Physics::RigidBody* mBody{nullptr};
    sf::Vector2f mPosition{0.f, 0.f};
    sf::Vector2f mSize{28.f, 96.f};
    float mLifetime{0.f};
    float mMaxLifetime{4.0f};
    bool mDead{false};
};
