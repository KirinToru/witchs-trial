#pragma once

#include <Game/Entities/Enemy.hpp>

class InquisitorFootman : public Enemy {
public:
//------------[Constructor - Initialize Inquisitor Footman Stats & Position]-------------------
    explicit InquisitorFootman(sf::Vector2f position);
//-------------------------------------------------------

//------------[Virtual Destructor - Cleanup Inquisitor Footman Resources]-------------------
    ~InquisitorFootman() override = default;
//-------------------------------------------------------

//------------[Spawn Attack Hitbox - Build and Activate Telegraphed Greatsword Slash]-------------------
    void spawnAttackHitbox() override;
//-------------------------------------------------------

//------------[Render - Draw Distinct Holy Order Armor, Greatsword & Telegraph Glow]-------------------
    void render(sf::RenderWindow& window, bool showHitbox = false) override;
//-------------------------------------------------------
};
