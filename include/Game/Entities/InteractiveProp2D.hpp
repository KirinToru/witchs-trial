#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

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

    InteractiveProp2D();

    void init(sf::Vector2f position, sf::Vector2f size, const PropPhysicsSettings& settings, ShapeType shapeType = ShapeType::Box);
    float calculateImpactDamage(const sf::Vector2f& impactVelocity) const;

    sf::Vector2f getPosition() const;
    void setPosition(sf::Vector2f pos);

    sf::Vector2f getVelocity() const;
    void setVelocity(sf::Vector2f vel);

    sf::Vector2f getSize() const;
    float getRotation() const;
    void setRotation(float angle);

    ShapeType getShapeType() const;
    const PropPhysicsSettings& getSettings() const;

private:
    sf::Vector2f mPosition{0.f, 0.f};
    sf::Vector2f mVelocity{0.f, 0.f};
    sf::Vector2f mSize{0.f, 0.f};
    float mRotation{0.f};
    ShapeType mShapeType{ShapeType::Box};
    PropPhysicsSettings mSettings;
};
