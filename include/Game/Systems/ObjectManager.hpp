#pragma once

#include <SFML/Graphics.hpp>
#include <Game/Entities/InteractiveProp2D.hpp>
#include <Game/Entities/Enemy.hpp>
#include <Game/Entities/InquisitorFootman.hpp>
#include <Game/Entities/GrandInquisitor.hpp>
#include <Game/Entities/Projectile.hpp>
#include <Game/Entities/IceWall.hpp>
#include <vector>
#include <memory>

class Player;

namespace Physics {
class PhysicsWorld;
}

namespace Engine::Graphics {
class ParticleSystem;
}

class ObjectManager {
public:
//------------[Constructor - Initialize Empty Manager]-------------------
  ObjectManager();
//-------------------------------------------------------

//------------[Spawn Box - Create Rectangle Prop]-------------------
  void spawnBox(sf::Vector2f position);
//-------------------------------------------------------

//------------[Spawn Crate - Create Destructible Wooden Crate]-------------------
  void spawnCrate(sf::Vector2f position, sf::Vector2f size = {40.f, 40.f});
//-------------------------------------------------------

//------------[Spawn Barrel - Create Destructible Barrel Prop]-------------------
  void spawnBarrel(sf::Vector2f position, sf::Vector2f size = {36.f, 44.f});
//-------------------------------------------------------

//------------[Spawn Ball - Create Circle Prop]-------------------
  void spawnBall(sf::Vector2f position);
//-------------------------------------------------------

//------------[Spawn Triangle - Create Triangular Convex Prop]-------------------
  void spawnTriangle(sf::Vector2f position);
//-------------------------------------------------------

//------------[Spawn Star - Create Star Shaped Convex Prop]-------------------
  void spawnStar(sf::Vector2f position);
//-------------------------------------------------------

//------------[Spawn Inquisitor - Instantiate Concrete Inquisitor Footman Enemy]-------------------
  InquisitorFootman* spawnInquisitor(sf::Vector2f position);
//-------------------------------------------------------

//------------[Spawn Boss - Instantiate Grand Inquisitor Boss Encounter]-------------------
  GrandInquisitor* spawnBoss(sf::Vector2f position);
//-------------------------------------------------------

//------------[Add Enemy - Register Dynamically Allocated Enemy]-------------------
  void addEnemy(std::unique_ptr<Enemy> enemy);
//-------------------------------------------------------

//------------[Update Enemies - Step Enemy AI & Physics Loop (60Hz)]-------------------
  void updateEnemies(float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Render Enemies - Draw All Active Enemies & Overhead Combat Gauges]-------------------
  void renderEnemies(sf::RenderWindow& window, bool showHitbox = false);
//-------------------------------------------------------

//------------[Clear Enemies - Remove All Spawned Enemies]-------------------
  void clearEnemies();
//-------------------------------------------------------

//------------[Spawn Projectile - Instantiate Magic Projectile]-------------------
  void spawnProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage = 30.f, float poiseDamage = 25.f);
//-------------------------------------------------------

//------------[Spawn Pogo Orb - Instantiate Large Strikeable Magic Orb]-------------------
  void spawnPogoOrb(sf::Vector2f position, sf::Vector2f velocity, float damage = 25.f);
//-------------------------------------------------------

//------------[Spawn Gun Projectile - Instantiate Fast High-Velocity Gun Shot]-------------------
  void spawnGunProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage = 40.f, float poiseDamage = 30.f);
//-------------------------------------------------------

//------------[Spawn Thunder Projectile - Instantiate Electric Shock Lightning Bolt]-------------------
  void spawnThunderProjectile(sf::Vector2f position, sf::Vector2f velocity, float damage = 25.f, float poiseDamage = 50.f);
//-------------------------------------------------------

//------------[Spawn Ice Wall - Instantiate Static Solid Ice Pillar]-------------------
  void spawnIceWall(sf::Vector2f position, Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Update Ice Walls - Step Ice Wall Lifetimes]-------------------
  void updateIceWalls(float dt, Engine::Graphics::ParticleSystem* particleSystem = nullptr);
//-------------------------------------------------------

//------------[Render Ice Walls - Draw Crystalline Ice Pillars and Debug Hitboxes]-------------------
  void renderIceWalls(sf::RenderWindow& window, bool showHitbox = false);
//-------------------------------------------------------

//------------[Update Projectiles - Step Projectile CCD Physics and Lifetimes]-------------------
  void updateProjectiles(float dt, const Physics::PhysicsWorld& physicsWorld, Engine::Graphics::ParticleSystem* particleSystem = nullptr);
//-------------------------------------------------------

//------------[Render Projectiles - Draw Magic Bolts and Debug Boxes]-------------------
  void renderProjectiles(sf::RenderWindow& window, bool showHitbox = false);
//-------------------------------------------------------

//------------[Clear Projectiles - Remove All Active Projectiles]-------------------
  void clearProjectiles();
//-------------------------------------------------------

//------------[Get Projectiles Const - Access Active Projectiles Const]-------------------
  const std::vector<std::unique_ptr<Projectile>>& getProjectiles() const { return mProjectiles; }
//-------------------------------------------------------

//------------[Get Projectiles Mutable - Access Active Projectiles]-------------------
  std::vector<std::unique_ptr<Projectile>>& getProjectiles() { return mProjectiles; }
//-------------------------------------------------------

//------------[Get Ice Walls Const - Access Active Ice Walls Const]-------------------
  const std::vector<std::unique_ptr<IceWall>>& getIceWalls() const { return mIceWalls; }
//-------------------------------------------------------

//------------[Get Ice Walls Mutable - Access Active Ice Walls]-------------------
  std::vector<std::unique_ptr<IceWall>>& getIceWalls() { return mIceWalls; }
//-------------------------------------------------------

//------------[Cleanup Destroyed - Remove Destroyed Props and Dead Projectiles]-------------------
  void cleanupDestroyed();
//-------------------------------------------------------

//------------[Clear - Remove All Spawned Props, Enemies and Projectiles]-------------------
  void clear();
//-------------------------------------------------------

//------------[Render - Draw All Active Props]-------------------
  void render(sf::RenderWindow& window);
//-------------------------------------------------------

//------------[Settle Props - Drop Environmental Props to Floor Colliders]-------------------
  void settleProps(const Physics::PhysicsWorld& physicsWorld);
//-------------------------------------------------------

//------------[Get Props Const - Access Spawned Interactive Props Const]-------------------
  const std::vector<std::unique_ptr<InteractiveProp2D>>& getProps() const { return mSpawnedProps; }
//-------------------------------------------------------

//------------[Get Props Mutable - Access Spawned Interactive Props]-------------------
  std::vector<std::unique_ptr<InteractiveProp2D>>& getProps() { return mSpawnedProps; }
//-------------------------------------------------------

//------------[Get Enemies - Access Spawned Enemies Const]-------------------
  const std::vector<std::unique_ptr<Enemy>>& getEnemies() const { return mEnemies; }
//-------------------------------------------------------

//------------[Get Enemies Mutable - Access Spawned Enemies]-------------------
  std::vector<std::unique_ptr<Enemy>>& getEnemies() { return mEnemies; }
//-------------------------------------------------------

//------------[Get Boss - Access Spawned Boss Encounter Pointer]-------------------
  GrandInquisitor* getBoss() const { return mBoss; }
//-------------------------------------------------------

//------------[Get Entity Count - Query Total Managed Entity Count]-------------------
  size_t getEntityCount() const { return mSpawnedProps.size() + mEnemies.size() + mProjectiles.size() + mIceWalls.size(); }
//-------------------------------------------------------

private:
  std::vector<std::unique_ptr<InteractiveProp2D>> mSpawnedProps;
  std::vector<std::unique_ptr<sf::Shape>> mSpawnedShapes;
  std::vector<std::unique_ptr<Enemy>> mEnemies;
  std::vector<std::unique_ptr<Projectile>> mProjectiles;
  std::vector<std::unique_ptr<IceWall>> mIceWalls;
  GrandInquisitor* mBoss{nullptr};
};
