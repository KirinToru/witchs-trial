#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace Physics {
  class PhysicsWorld;
}

struct MapText {
  sf::Vector2f position;
  sf::Vector2f size;
  std::string content;
  std::string name;
};

class Map {
public:
  static constexpr float TILE_SIZE = 32.f;

  static constexpr uint32_t FLIP_H = 0x80000000;
  static constexpr uint32_t FLIP_V = 0x40000000;
  static constexpr uint32_t FLIP_D = 0x20000000;
  static constexpr uint32_t TILE_MASK = 0x1FFFFFFF;

  struct TilesetInfo {
    int firstgid;
    int tilewidth;
    int tileheight;
    int tilecount;
    int columns;
    std::string name;
    std::string imageSource;
    sf::Texture texture;
  };

  enum TileType {
    PlayerSpawn = 0,
    EnemySpawn = 1,
    BossSpawn = 2,
    SavePoint = 3,
    ArenaTrigger = 4,
    LevelTrigger = 5,
    LightMarker = 6,
    SystemMarker = 7,
    SolidWall = 8,
    OneWayPlatform = 9,
    IceModifier = 10,
    TrampolineModifier = 11,
    MothFloor = 12,
    Spikes = 13,
    PendulumTrap = 14,
    Reserve = 15,

    Start = PlayerSpawn,
    Finish = LevelTrigger,
    Wall = SolidWall,
    Platform = OneWayPlatform,
    Hazard = MothFloor
  };

//------------[Constructor - Initialize Map Instance]-------------------
  Map();
//-------------------------------------------------------

//------------[Load From File - Parse TMX File and Setup Colliders]-------------------
  bool loadFromFile(const std::string &filename, Physics::PhysicsWorld *physicsWorld = nullptr);
//-------------------------------------------------------

//------------[Generate Colliders - Build Static Physics Colliders from Map Grid]-------------------
  void generateColliders(Physics::PhysicsWorld &physicsWorld) const;
//-------------------------------------------------------

//------------[Get Width - Query Total Map Width in Pixels]-------------------
  float getWidth() const {
    if (layers.empty() || layers[0].grid.empty())
      return 0.f;
    return layers[0].grid[0].size() * TILE_SIZE;
  }
//-------------------------------------------------------

//------------[Get Height - Query Total Map Height in Pixels]-------------------
  float getHeight() const {
    return layers.empty() ? 0.f : layers[0].grid.size() * TILE_SIZE;
  }
//-------------------------------------------------------

//------------[Get Start Position - Query Player Spawn Coordinate]-------------------
  sf::Vector2f getStartPosition() const { return startPosition; }
//-------------------------------------------------------

//------------[Get Enemy Spawns - Query List of Parsed Enemy Spawn Coordinates]-------------------
  const std::vector<sf::Vector2f>& getEnemySpawns() const { return mEnemySpawns; }
//-------------------------------------------------------

//------------[Has Boss Spawn - Check If Map Contains Boss Spawn]-------------------
  bool hasBossSpawn() const { return mHasBossSpawn; }
//-------------------------------------------------------

//------------[Get Boss Spawn - Query Boss Spawn Coordinate]-------------------
  sf::Vector2f getBossSpawn() const { return mBossSpawnPos; }
//-------------------------------------------------------

//------------[Has Arena Trigger - Check If Map Contains Arena Trigger]-------------------
  bool hasArenaTrigger() const { return mHasArenaTrigger; }
//-------------------------------------------------------

//------------[Get Arena Trigger Position - Query Arena Trigger Coordinate]-------------------
  sf::Vector2f getArenaTriggerPosition() const { return mArenaTriggerPos; }
//-------------------------------------------------------

//------------[Has Level Trigger - Check If Map Contains Level Trigger]-------------------
  bool hasLevelTrigger() const { return mHasLevelTrigger; }
//-------------------------------------------------------

//------------[Get Level Trigger Position - Query Level Trigger Coordinate]-------------------
  sf::Vector2f getLevelTriggerPosition() const { return mLevelTriggerPos; }
//-------------------------------------------------------

//------------[Get Save Points - Query List of Parsed Save Point Coordinates]-------------------
  const std::vector<sf::Vector2f>& getSavePoints() const { return mSavePoints; }
//-------------------------------------------------------

//------------[Get Light Markers - Query List of Light Source Coordinates]-------------------
  const std::vector<sf::Vector2f>& getLightMarkers() const { return mLightMarkers; }
//-------------------------------------------------------

//------------[Render - Render Visible Map Tiles and Background]-------------------
  void render(sf::RenderWindow &window, sf::Vector2f playerPos = {0, 0},
              bool showHitboxes = false);
//-------------------------------------------------------

//------------[Check Collision - Test Bounds Against Solid Tiles]-------------------
  std::vector<sf::FloatRect> checkCollision(const sf::FloatRect &bounds) const;
//-------------------------------------------------------

//------------[Check Finish - Test Bounds Against Level Finish Areas]-------------------
  bool checkFinish(const sf::FloatRect &bounds) const;
//-------------------------------------------------------

//------------[Check Platform Collision - Test Bounds Against One-Way Platforms]-------------------
  std::vector<sf::FloatRect> checkPlatformCollision(const sf::FloatRect &bounds) const;
//-------------------------------------------------------

//------------[Check Spike Collision - Test Bounds Against Hazard Tiles]-------------------
  bool checkSpikeCollision(const sf::FloatRect &bounds) const;
//-------------------------------------------------------

private:
  bool parseTMX(const std::string &content, const std::string &basePath);
  std::vector<std::vector<uint32_t>> parseLayerData(const std::string &csvData,
                                                    int width, int height);
  void parseObjectGroup(const std::string &content);
  void prepareTextObjects();

  struct Layer {
    std::string name;
    std::vector<std::vector<uint32_t>> grid;
  };

  std::vector<Layer> layers;
  std::vector<MapText> textObjects;
  std::vector<sf::Text> cachedTexts;

  sf::Vector2f startPosition{100.f, 100.f};
  std::vector<sf::Vector2f> mEnemySpawns;
  sf::Vector2f mBossSpawnPos{0.f, 0.f};
  bool mHasBossSpawn{false};
  sf::Vector2f mArenaTriggerPos{0.f, 0.f};
  bool mHasArenaTrigger{false};
  sf::Vector2f mLevelTriggerPos{0.f, 0.f};
  bool mHasLevelTrigger{false};
  std::vector<sf::Vector2f> mSavePoints;
  std::vector<sf::Vector2f> mLightMarkers;

  std::vector<sf::FloatRect> finishAreas;

  sf::RectangleShape tileShape;
  std::vector<TilesetInfo> tilesets;

  sf::Font font;
  bool fontLoaded = false;
  const TilesetInfo *getTilesetForId(int globalId) const;

//------------[Get Tileset Index For Id - Query Index of Containing Tileset]-------------------
  int getTilesetIndexForId(int globalId) const;
//-------------------------------------------------------

//------------[Is Solid Tile At - Check If Specified Grid Cell Contains Solid Wall]-------------------
  bool isSolidTileAt(int gridX, int gridY) const;
//-------------------------------------------------------

//------------[Is Hazard Tile At - Check If Specified Grid Cell Contains Hazard Tile]-------------------
  bool isHazardTileAt(int gridX, int gridY) const;
//-------------------------------------------------------

//------------[Is Special Modifier At - Check If Specified Grid Cell Contains Modifier]-------------------
  bool isSpecialModifierAt(int gridX, int gridY) const;
//-------------------------------------------------------

  std::vector<sf::VertexArray> mTilesetBatches;
  sf::VertexArray mTopShadowBatch;
};
