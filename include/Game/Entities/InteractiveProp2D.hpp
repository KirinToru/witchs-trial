#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <Engine/Physics/AABB.hpp>

struct PropPhysicsSettings {
    float density = 1.0f;
    float friction = 0.5f;
    float restitution = 0.0f;
    float linearDamping = 2.0f;
    float angularDamping = 0.5f;
    bool useCCD = true;
    float damageMultiplier = 1.0f;
};

class InteractiveProp2D {
public:
    enum class ShapeType {
        Box,
        Circle,
        Triangle,
        Star
    };

//------------[Constructor - Initialize Default Prop]-------------------
    InteractiveProp2D();
//-------------------------------------------------------

//------------[Init - Configure Prop Properties]-------------------
    void init(sf::Vector2f position, sf::Vector2f size, const PropPhysicsSettings& settings, ShapeType shapeType = ShapeType::Box);
//-------------------------------------------------------

//------------[Calculate Impact Damage - Compute Damage Based on Velocity]-------------------
    float calculateImpactDamage(const sf::Vector2f& impactVelocity) const;
//-------------------------------------------------------

//------------[Get Position - Return Prop Position]-------------------
    sf::Vector2f getPosition() const;
//-------------------------------------------------------

//------------[Set Position - Update Prop Position]-------------------
    void setPosition(sf::Vector2f pos);
//-------------------------------------------------------

//------------[Get Velocity - Return Prop Velocity]-------------------
    sf::Vector2f getVelocity() const;
//-------------------------------------------------------

//------------[Set Velocity - Update Prop Velocity]-------------------
    void setVelocity(sf::Vector2f vel);
//-------------------------------------------------------

//------------[Get Size - Return Prop Dimensions]-------------------
    sf::Vector2f getSize() const;
//-------------------------------------------------------

//------------[Get Rotation - Return Prop Angle]-------------------
    float getRotation() const;
//-------------------------------------------------------

//------------[Set Rotation - Update Prop Angle]-------------------
    void setRotation(float angle);
//-------------------------------------------------------

//------------[Get Shape Type - Return Prop Geometry Type]-------------------
    ShapeType getShapeType() const;
//-------------------------------------------------------

//------------[Get Settings - Return Physics Parameters]-------------------
    const PropPhysicsSettings& getSettings() const;
//-------------------------------------------------------

//------------[Is Destructible - Check Destructibility Flag]-------------------
    bool isDestructible() const;
//-------------------------------------------------------

//------------[Set Destructible - Configure Destructibility Flag]-------------------
    void setDestructible(bool destructible);
//-------------------------------------------------------

//------------[Get Health - Query Remaining Prop Durability]-------------------
    float getHealth() const;
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Prop Durability]-------------------
    float getMaxHealth() const;
//-------------------------------------------------------

//------------[Set Health - Update Prop Durability]-------------------
    void setHealth(float hp);
//-------------------------------------------------------

//------------[Is Destroyed - Query Destruction State]-------------------
    bool isDestroyed() const;
//-------------------------------------------------------

//------------[Take Damage - Inflict Structural Damage]-------------------
    bool takeDamage(float damage);
//-------------------------------------------------------

//------------[Get AABB - Calculate World Space Bounding Box]-------------------
    Physics::AABB getAABB() const;
//-------------------------------------------------------

private:
    sf::Vector2f mPosition{0.f, 0.f};
    sf::Vector2f mVelocity{0.f, 0.f};
    sf::Vector2f mSize{0.f, 0.f};
    float mRotation{0.f};
    ShapeType mShapeType{ShapeType::Box};
    PropPhysicsSettings mSettings;

    bool mDestructible{true};
    float mHealth{60.f};
    float mMaxHealth{60.f};
};
