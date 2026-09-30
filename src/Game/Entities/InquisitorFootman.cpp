#include <Game/Entities/InquisitorFootman.hpp>
#include <cmath>

//------------[Constructor - Initialize Inquisitor Footman Stats & Position]-------------------
InquisitorFootman::InquisitorFootman(sf::Vector2f position)
    : Enemy(
          /*size=*/{30.f, 52.f},
          /*maxHealth=*/120.f,
          /*maxPosture=*/60.f,
          /*moveSpeed=*/115.f,
          /*attackRange=*/54.f,
          /*detectionRange=*/380.f,
          /*telegraphDuration=*/0.65f,
          /*staggerDuration=*/2.5f) {

    setPosition(position);

    // Holy Order Iron/Steel Armor with Gold/Crimson trim
    mShape.setFillColor(sf::Color(90, 95, 110));
    mShape.setOutlineColor(sf::Color(45, 45, 55));
    mShape.setOutlineThickness(1.5f);
}
//-------------------------------------------------------

//------------[Spawn Attack Hitbox - Build and Activate Telegraphed Greatsword Slash]-------------------
void InquisitorFootman::spawnAttackHitbox() {
    Combat::Hitbox slash;
    slash.damage = 20.f;
    slash.poiseDamage = 15.f;
    slash.knockback = mFacingRight ? sf::Vector2f(220.f, -90.f) : sf::Vector2f(-220.f, -90.f);
    slash.active = true;

    // Greatsword slash volume in front of the Footman
    if (mFacingRight) {
        slash.localBounds = Physics::AABB::fromPositionSize({24.f, 6.f}, {42.f, 44.f});
    } else {
        slash.localBounds = Physics::AABB::fromPositionSize({-36.f, 6.f}, {42.f, 44.f});
    }

    setAttackHitbox(slash);
}
//-------------------------------------------------------

//------------[Render - Draw Distinct Holy Order Armor, Greatsword & Telegraph Glow]-------------------
void InquisitorFootman::render(sf::RenderWindow& window, bool showHitbox) {
    // 1. Draw base body, overhead health/posture gauges, and debug hitboxes
    Enemy::render(window, showHitbox);

    if (isDead()) return;

    sf::Vector2f pos = mShape.getPosition();
    sf::Vector2f size = mShape.getSize();

    // 2. Draw Holy Order Crimson Tabard
    sf::RectangleShape tabard({size.x * 0.55f, size.y * 0.45f});
    tabard.setFillColor(sf::Color(165, 30, 30));
    tabard.setPosition({pos.x + size.x * 0.225f, pos.y + size.y * 0.25f});
    window.draw(tabard);

    // 3. Draw Inquisitor Helmet Visor Slit (Menacing Red Glow)
    sf::RectangleShape visor({8.f, 3.f});
    visor.setFillColor(sf::Color(255, 40, 30));
    float visorX = mFacingRight ? (pos.x + size.x * 0.55f) : (pos.x + size.x * 0.20f);
    visor.setPosition({visorX, pos.y + 10.f});
    window.draw(visor);

    // 4. Draw Inquisitor Greatsword & Telegraph / Slash Effects
    EnemyStateType state = getCurrentStateType();

    sf::RectangleShape sword;
    if (state == EnemyStateType::TelegraphAttack) {
        // Raised Greatsword charged with bright telegraph energy (flashing golden glow)
        sword.setSize({6.f, 36.f});
        sword.setOrigin({3.f, 34.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.7f) : (pos.x + size.x * 0.3f), pos.y + 12.f});
        sword.setRotation(sf::degrees(mFacingRight ? -35.f : 35.f));
        sword.setFillColor(sf::Color(255, 210, 40));
        sword.setOutlineColor(sf::Color(255, 80, 20));
        sword.setOutlineThickness(2.f);
        window.draw(sword);
    } else if (state == EnemyStateType::ActiveAttack) {
        // Greatsword swept down in heavy forward slash
        sword.setSize({38.f, 6.f});
        sword.setOrigin({2.f, 3.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.5f) : (pos.x + size.x * 0.5f), pos.y + 26.f});
        sword.setRotation(sf::degrees(mFacingRight ? 20.f : 160.f));
        sword.setFillColor(sf::Color(220, 225, 235));
        sword.setOutlineColor(sf::Color(240, 180, 50));
        sword.setOutlineThickness(1.5f);
        window.draw(sword);
    } else if (state == EnemyStateType::Staggered) {
        // Dropped / tilted sword when staggered and vulnerable
        sword.setSize({6.f, 30.f});
        sword.setOrigin({3.f, 2.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x + 4.f) : (pos.x - 4.f), pos.y + 28.f});
        sword.setRotation(sf::degrees(mFacingRight ? 55.f : -55.f));
        sword.setFillColor(sf::Color(120, 120, 130));
        window.draw(sword);
    } else {
        // Resting Greatsword held upright
        sword.setSize({5.f, 32.f});
        sword.setOrigin({2.5f, 28.f});
        sword.setPosition({mFacingRight ? (pos.x + size.x * 0.2f) : (pos.x + size.x * 0.8f), pos.y + 24.f});
        sword.setRotation(sf::degrees(mFacingRight ? -15.f : 15.f));
        sword.setFillColor(sf::Color(180, 185, 195));
        sword.setOutlineColor(sf::Color(40, 40, 45));
        sword.setOutlineThickness(1.f);
        window.draw(sword);
    }
}
//-------------------------------------------------------
