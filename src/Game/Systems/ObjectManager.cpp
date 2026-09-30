#include <Game/Systems/ObjectManager.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Engine/Physics/SweptAABB.hpp>
#include <cmath>

//------------[Constructor - Initialize Empty Manager]-------------------
ObjectManager::ObjectManager() = default;
//-------------------------------------------------------

//------------[Spawn Box - Create Rectangle Prop]-------------------
void ObjectManager::spawnBox(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 2.0f;
  settings.friction = 0.4f;
  settings.restitution = 0.1f;
  
  sf::Vector2f size = {40.f, 40.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Box);
  prop->setDestructible(true);
  prop->setHealth(60.f);
  
  auto shape = std::make_unique<sf::RectangleShape>(size);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(180, 100, 50));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Crate - Create Destructible Wooden Crate]-------------------
void ObjectManager::spawnCrate(sf::Vector2f position, sf::Vector2f size) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 1.8f;
  settings.friction = 0.5f;
  settings.restitution = 0.05f;
  
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Box);
  prop->setDestructible(true);
  prop->setHealth(50.f);
  
  auto shape = std::make_unique<sf::RectangleShape>(size);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(150, 95, 45));
  shape->setOutlineThickness(1.5f);
  shape->setOutlineColor(sf::Color(210, 160, 90));
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Barrel - Create Destructible Barrel Prop]-------------------
void ObjectManager::spawnBarrel(sf::Vector2f position, sf::Vector2f size) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 2.2f;
  settings.friction = 0.45f;
  settings.restitution = 0.15f;
  
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Box);
  prop->setDestructible(true);
  prop->setHealth(70.f);
  
  auto shape = std::make_unique<sf::RectangleShape>(size);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(115, 75, 40));
  shape->setOutlineThickness(1.5f);
  shape->setOutlineColor(sf::Color(180, 180, 190));
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Ball - Create Circle Prop]-------------------
void ObjectManager::spawnBall(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 1.0f;
  settings.friction = 0.2f;
  settings.restitution = 0.7f;
  
  sf::Vector2f size = {40.f, 40.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Circle);
  prop->setDestructible(false);
  
  auto shape = std::make_unique<sf::CircleShape>(size.x / 2.f);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(50, 150, 220));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Triangle - Create Triangular Convex Prop]-------------------
void ObjectManager::spawnTriangle(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 1.5f;
  settings.friction = 0.5f;
  settings.restitution = 0.1f;
  
  sf::Vector2f size = {50.f, 45.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Triangle);
  prop->setDestructible(false);
  
  auto shape = std::make_unique<sf::ConvexShape>(3);
  shape->setPoint(0, {-size.x / 2.f, size.y / 2.f});
  shape->setPoint(1, {size.x / 2.f, size.y / 2.f});
  shape->setPoint(2, {0.f, -size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(220, 180, 50));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Star - Create Star Shaped Convex Prop]-------------------
void ObjectManager::spawnStar(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 0.8f;
  settings.friction = 0.3f;
  settings.restitution = 0.4f;
  
  sf::Vector2f size = {45.f, 45.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Star);
  prop->setDestructible(false);
  
  auto shape = std::make_unique<sf::ConvexShape>(10);
  float outerR = size.x / 2.f;
  float innerR = outerR * 0.4f;
  for (int i = 0; i < 10; ++i) {
    float angle = i * (3.14159265f / 5.f) - (3.14159265f / 2.f);
    float r = (i % 2 == 0) ? outerR : innerR;
    shape->setPoint(i, {r * std::cos(angle), r * std::sin(angle)});
  }
  shape->setPosition(position);
  shape->setFillColor(sf::Color(255, 105, 180));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Inquisitor - Instantiate Concrete Inquisitor Footman Enemy]-------------------
InquisitorFootman* ObjectManager::spawnInquisitor(sf::Vector2f position) {
  auto footman = std::make_unique<InquisitorFootman>(position);
  InquisitorFootman* ptr = footman.get();
  mEnemies.push_back(std::move(footman));
  return ptr;
}
//-------------------------------------------------------

//------------[Spawn Boss - Instantiate Grand Inquisitor Boss Encounter]-------------------
GrandInquisitor* ObjectManager::spawnBoss(sf::Vector2f position) {
  auto boss = std::make_unique<GrandInquisitor>(position);
  GrandInquisitor* ptr = boss.get();
  mBoss = ptr;
  mEnemies.push_back(std::move(boss));
  return ptr;
}
//-------------------------------------------------------

//------------[Add Enemy - Register Dynamically Allocated Enemy]-------------------
void ObjectManager::addEnemy(std::unique_ptr<Enemy> enemy) {
  if (enemy) {
    mEnemies.push_back(std::move(enemy));
  }
}
//-------------------------------------------------------

//------------[Update Enemies - Step Enemy AI & Physics Loop (60Hz)]-------------------
void ObjectManager::updateEnemies(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
  for (auto& enemy : mEnemies) {
    if (enemy) {
      enemy->fixedUpdate(dt, player, physicsWorld);
    }
  }
}
//-------------------------------------------------------

//------------[Render Enemies - Draw All Active Enemies & Overhead Combat Gauges]-------------------
void ObjectManager::renderEnemies(sf::RenderWindow& window, bool showHitbox) {
  for (auto& enemy : mEnemies) {
    if (enemy) {
      enemy->render(window, showHitbox);
    }
  }
}
//-------------------------------------------------------

//------------[Clear Enemies - Remove All Spawned Enemies]-------------------
void ObjectManager::clearEnemies() {
  mBoss = nullptr;
  mEnemies.clear();
}
//-------------------------------------------------------

//------------[Spawn Projectile - Instantiate Magic Projectile]-------------------
void ObjectManager::spawnProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage, float poiseDamage) {
  mProjectiles.push_back(std::make_unique<Projectile>(position, velocity, damage, poiseDamage));
}
//-------------------------------------------------------

//------------[Update Projectiles - Step Projectile CCD Physics and Lifetimes]-------------------
void ObjectManager::updateProjectiles(float dt, const Physics::PhysicsWorld& physicsWorld) {
  for (auto& proj : mProjectiles) {
    if (proj && !proj->isDead()) {
      proj->update(dt, physicsWorld);
    }
  }
}
//-------------------------------------------------------

//------------[Render Projectiles - Draw Magic Bolts and Debug Boxes]-------------------
void ObjectManager::renderProjectiles(sf::RenderWindow& window, bool showHitbox) {
  for (auto& proj : mProjectiles) {
    if (proj && !proj->isDead()) {
      proj->render(window, showHitbox);
    }
  }
}
//-------------------------------------------------------

//------------[Clear Projectiles - Remove All Active Projectiles]-------------------
void ObjectManager::clearProjectiles() {
  mProjectiles.clear();
}
//-------------------------------------------------------

//------------[Cleanup Destroyed - Remove Destroyed Props and Dead Projectiles]-------------------
void ObjectManager::cleanupDestroyed() {
  for (size_t i = 0; i < mSpawnedProps.size(); ) {
    if (mSpawnedProps[i] && mSpawnedProps[i]->isDestroyed()) {
      mSpawnedProps.erase(mSpawnedProps.begin() + i);
      if (i < mSpawnedShapes.size()) {
        mSpawnedShapes.erase(mSpawnedShapes.begin() + i);
      }
    } else {
      ++i;
    }
  }

  std::erase_if(mProjectiles, [](const std::unique_ptr<Projectile>& proj) {
    return !proj || proj->isDead();
  });
}
//-------------------------------------------------------

//------------[Clear - Remove All Spawned Props, Enemies and Projectiles]-------------------
void ObjectManager::clear() {
  mSpawnedProps.clear();
  mSpawnedShapes.clear();
  mEnemies.clear();
  mProjectiles.clear();
  mBoss = nullptr;
}
//-------------------------------------------------------

//------------[Render - Draw All Active Props]-------------------
void ObjectManager::render(sf::RenderWindow& window) {
  for (size_t i = 0; i < mSpawnedProps.size() && i < mSpawnedShapes.size(); ++i) {
    mSpawnedShapes[i]->setPosition(mSpawnedProps[i]->getPosition());
    mSpawnedShapes[i]->setRotation(sf::degrees(mSpawnedProps[i]->getRotation()));
    window.draw(*mSpawnedShapes[i]);
  }
}
//-------------------------------------------------------

//------------[Settle Props - Drop Environmental Props to Floor Colliders]-------------------
void ObjectManager::settleProps(const Physics::PhysicsWorld& physicsWorld) {
  for (size_t i = 0; i < mSpawnedProps.size(); ++i) {
    auto& prop = mSpawnedProps[i];
    if (!prop) continue;

    Physics::AABB box = prop->getAABB();
    Physics::SweptHit groundHit = physicsWorld.sweepTest(box, {0.f, 2000.f}, nullptr, false);
    float dropDist = groundHit.hit ? (2000.f * groundHit.toi) : 0.f;

    for (size_t j = 0; j < i; ++j) {
      const auto& other = mSpawnedProps[j];
      if (!other) continue;
      Physics::AABB otherBox = other->getAABB();
      Physics::SweptHit propHit = Physics::sweepAABB(box, {0.f, 2000.f}, otherBox);
      if (propHit.hit && (propHit.toi * 2000.f) < dropDist) {
        dropDist = propHit.toi * 2000.f;
      }
    }

    if (dropDist > 0.f) {
      sf::Vector2f newPos = prop->getPosition() + sf::Vector2f(0.f, dropDist);
      prop->setPosition(newPos);
      if (i < mSpawnedShapes.size() && mSpawnedShapes[i]) {
        mSpawnedShapes[i]->setPosition(newPos);
      }
    }
  }
}
//-------------------------------------------------------
