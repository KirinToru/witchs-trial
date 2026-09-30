#include <Game/Entities/InteractiveProp2D.hpp>
#include <algorithm>
#include <cmath>

//------------[Constructor - Initialize Default Prop]-------------------
InteractiveProp2D::InteractiveProp2D() = default;
//-------------------------------------------------------

//------------[Init - Configure Prop Properties]-------------------
void InteractiveProp2D::init(sf::Vector2f position, sf::Vector2f size, const PropPhysicsSettings& settings, ShapeType shapeType) {
    mPosition = position;
    mSize = size;
    mSettings = settings;
    mShapeType = shapeType;
    mVelocity = {0.f, 0.f};
    mRotation = 0.f;
    mDestructible = true;
    mHealth = 60.f;
    mMaxHealth = 60.f;
}
//-------------------------------------------------------

//------------[Calculate Impact Damage - Compute Damage Based on Velocity]-------------------
float InteractiveProp2D::calculateImpactDamage(const sf::Vector2f& impactVelocity) const {
    float mass = mSettings.density * (mSize.x * mSize.y * 0.001f);
    if (mass < 0.1f) mass = 0.1f;
    float velocityMag = std::hypot(impactVelocity.x, impactVelocity.y);
    float impactForce = mass * velocityMag;
    return impactForce * mSettings.damageMultiplier;
}
//-------------------------------------------------------

//------------[Get Position - Return Prop Position]-------------------
sf::Vector2f InteractiveProp2D::getPosition() const {
    return mPosition;
}
//-------------------------------------------------------

//------------[Set Position - Update Prop Position]-------------------
void InteractiveProp2D::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}
//-------------------------------------------------------

//------------[Get Velocity - Return Prop Velocity]-------------------
sf::Vector2f InteractiveProp2D::getVelocity() const {
    return mVelocity;
}
//-------------------------------------------------------

//------------[Set Velocity - Update Prop Velocity]-------------------
void InteractiveProp2D::setVelocity(sf::Vector2f vel) {
    mVelocity = vel;
}
//-------------------------------------------------------

//------------[Get Size - Return Prop Dimensions]-------------------
sf::Vector2f InteractiveProp2D::getSize() const {
    return mSize;
}
//-------------------------------------------------------

//------------[Get Rotation - Return Prop Angle]-------------------
float InteractiveProp2D::getRotation() const {
    return mRotation;
}
//-------------------------------------------------------

//------------[Set Rotation - Update Prop Angle]-------------------
void InteractiveProp2D::setRotation(float angle) {
    mRotation = angle;
}
//-------------------------------------------------------

//------------[Get Shape Type - Return Prop Geometry Type]-------------------
InteractiveProp2D::ShapeType InteractiveProp2D::getShapeType() const {
    return mShapeType;
}
//-------------------------------------------------------

//------------[Get Settings - Return Physics Parameters]-------------------
const PropPhysicsSettings& InteractiveProp2D::getSettings() const {
    return mSettings;
}
//-------------------------------------------------------

//------------[Is Destructible - Check Destructibility Flag]-------------------
bool InteractiveProp2D::isDestructible() const {
    return mDestructible;
}
//-------------------------------------------------------

//------------[Set Destructible - Configure Destructibility Flag]-------------------
void InteractiveProp2D::setDestructible(bool destructible) {
    mDestructible = destructible;
}
//-------------------------------------------------------

//------------[Get Health - Query Remaining Prop Durability]-------------------
float InteractiveProp2D::getHealth() const {
    return mHealth;
}
//-------------------------------------------------------

//------------[Get Max Health - Query Maximum Prop Durability]-------------------
float InteractiveProp2D::getMaxHealth() const {
    return mMaxHealth;
}
//-------------------------------------------------------

//------------[Set Health - Update Prop Durability]-------------------
void InteractiveProp2D::setHealth(float hp) {
    mHealth = std::clamp(hp, 0.f, mMaxHealth);
}
//-------------------------------------------------------

//------------[Is Destroyed - Query Destruction State]-------------------
bool InteractiveProp2D::isDestroyed() const {
    return mDestructible && (mHealth <= 0.f);
}
//-------------------------------------------------------

//------------[Take Damage - Inflict Structural Damage]-------------------
bool InteractiveProp2D::takeDamage(float damage) {
    if (!mDestructible) return false;
    mHealth = std::max(0.f, mHealth - damage);
    return mHealth <= 0.f;
}
//-------------------------------------------------------

//------------[Get AABB - Calculate World Space Bounding Box]-------------------
Physics::AABB InteractiveProp2D::getAABB() const {
    return Physics::AABB::fromCenterHalfExtents(mPosition, mSize * 0.5f);
}
//-------------------------------------------------------
