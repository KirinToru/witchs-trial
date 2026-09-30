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
    Start = 0,
    Finish = 1,
    Wall = 2,
    Platform = 3,
    Spikes = 4
  };

  Map();

  bool loadFromFile(const std::string &filename, Physics::PhysicsWorld *physicsWorld = nullptr);
  void generateColliders(Physics::PhysicsWorld &physicsWorld) const;

  float getWidth() const {
    if (layers.empty() || layers[0].grid.empty())
      return 0.f;
    return layers[0].grid[0].size() * TILE_SIZE;
  }

  float getHeight() const {
    return layers.empty() ? 0.f : layers[0].grid.size() * TILE_SIZE;
  }

  sf::Vector2f getStartPosition() const { return startPosition; }

  void render(sf::RenderWindow &window, sf::Vector2f playerPos = {0, 0},
              bool showHitboxes = false);

  std::vector<sf::FloatRect> checkCollision(const sf::FloatRect &bounds) const;
  bool checkFinish(const sf::FloatRect &bounds) const;
  std::vector<sf::FloatRect> checkPlatformCollision(const sf::FloatRect &bounds) const;
  bool checkSpikeCollision(const sf::FloatRect &bounds) const;

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
  std::vector<sf::FloatRect> finishAreas;

  sf::RectangleShape tileShape;
  std::vector<TilesetInfo> tilesets;

  sf::Font font;
  bool fontLoaded = false;
  const TilesetInfo *getTilesetForId(int globalId) const;

//------------[Get Tileset Index For Id - Query Index of Containing Tileset]-------------------
  int getTilesetIndexForId(int globalId) const;
//-------------------------------------------------------

  std::vector<sf::VertexArray> mTilesetBatches;
};
