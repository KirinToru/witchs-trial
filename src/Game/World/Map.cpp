#include <Game/World/Map.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

//------------[Constructor - Initialize Tile Shape and Map Font]-------------------
Map::Map() {
  tileShape.setSize({TILE_SIZE, TILE_SIZE});
  tileShape.setFillColor(sf::Color::White);

  // Load font for text objects
  fontLoaded = font.openFromFile("assets/fonts/font.ttf");
  if (!fontLoaded) {
    std::cerr << "Failed to load font for map text" << std::endl;
  }
}
//-------------------------------------------------------

//------------[Load From File - Parse TMX Map and Optionally Populate Physics Colliders]-------------------
bool Map::loadFromFile(const std::string &filename, Physics::PhysicsWorld *physicsWorld) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open map file: " << filename << std::endl;
    return false;
  }

  // Read entire file content
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Check file extension
  bool isTMX = filename.substr(filename.find_last_of(".") + 1) == "tmx";
  if (!isTMX) {
    std::cerr << "Error: Only .tmx map files are supported." << std::endl;
    return false;
  }

  std::string basePath = "";
  size_t slashPos = filename.find_last_of("/\\");
  if (slashPos != std::string::npos) {
    basePath = filename.substr(0, slashPos + 1);
  }

  bool success = parseTMX(content, basePath);
  if (success && physicsWorld) {
    generateColliders(*physicsWorld);
  }
  return success;
}
//-------------------------------------------------------

// Helper function to extract attribute value from XML tag
std::string extractAttribute(const std::string &tag,
                             const std::string &attrName) {
  std::string search = attrName + "=\"";
  size_t start = tag.find(search);
  if (start == std::string::npos)
    return "";
  start += search.length();
  size_t end = tag.find("\"", start);
  if (end == std::string::npos)
    return "";
  return tag.substr(start, end - start);
}

bool Map::parseTMX(const std::string &content, const std::string &basePath) {
  layers.clear();
  textObjects.clear();
  finishAreas.clear();
  tilesets.clear(); // Clear previously loaded tilesets

  // Extract map dimensions
  size_t mapTagStart = content.find("<map ");
  size_t mapTagEnd = content.find(">", mapTagStart);
  std::string mapTag = content.substr(mapTagStart, mapTagEnd - mapTagStart);

  int mapWidth = std::stoi(extractAttribute(mapTag, "width"));
  int mapHeight = std::stoi(extractAttribute(mapTag, "height"));

  // Parse all <tileset> tags
  size_t pos = 0;
  while (true) {
    size_t tilesetStart = content.find("<tileset ", pos);
    if (tilesetStart == std::string::npos)
      break;

    size_t tilesetTagEnd = content.find(">", tilesetStart);
    std::string tilesetTag =
        content.substr(tilesetStart, tilesetTagEnd - tilesetStart);

    TilesetInfo ts;
    ts.firstgid = std::stoi(extractAttribute(tilesetTag, "firstgid"));

    std::string sourceAttr = extractAttribute(tilesetTag, "source");
    if (!sourceAttr.empty()) {
      // External TSX file
      std::string tsxPath = sourceAttr;
      size_t tsxSlash = sourceAttr.find_last_of("/\\");
      std::string tsxFilename = (tsxSlash == std::string::npos)
                                    ? sourceAttr
                                    : sourceAttr.substr(tsxSlash + 1);
      tsxPath = "assets/tilesets/" + tsxFilename;
      std::ifstream tsxFile(tsxPath);
      if (tsxFile.is_open()) {
        std::stringstream tsxBuffer;
        tsxBuffer << tsxFile.rdbuf();
        std::string tsxContent = tsxBuffer.str();

        size_t tsStart = tsxContent.find("<tileset ");
        if (tsStart != std::string::npos) {
          size_t tsEnd = tsxContent.find(">", tsStart);
          std::string externalTag = tsxContent.substr(tsStart, tsEnd - tsStart);

          ts.name = extractAttribute(externalTag, "name");

          std::string twStr = extractAttribute(externalTag, "tilewidth");
          if (!twStr.empty())
            ts.tilewidth = std::stoi(twStr);

          std::string thStr = extractAttribute(externalTag, "tileheight");
          if (!thStr.empty())
            ts.tileheight = std::stoi(thStr);

          std::string tcStr = extractAttribute(externalTag, "tilecount");
          if (!tcStr.empty())
            ts.tilecount = std::stoi(tcStr);

          std::string colStr = extractAttribute(externalTag, "columns");
          if (!colStr.empty())
            ts.columns = std::stoi(colStr);

          size_t imgStart = tsxContent.find("<image ");
          if (imgStart != std::string::npos) {
            size_t imgEnd = tsxContent.find(">", imgStart);
            std::string imageTag =
                tsxContent.substr(imgStart, imgEnd - imgStart);
            std::string rawSource = extractAttribute(imageTag, "source");
            size_t lastSlash = rawSource.find_last_of("/\\");
            std::string filename = (lastSlash == std::string::npos)
                                       ? rawSource
                                       : rawSource.substr(lastSlash + 1);
            ts.imageSource = "assets/tilesets/" + filename;

            if (!ts.texture.loadFromFile(ts.imageSource)) {
              std::cerr << "Failed to load external tileset image: "
                        << ts.imageSource << std::endl;
            }
          }
        }
      } else {
        std::cerr << "Failed to open external tileset file: " << tsxPath
                  << std::endl;
      }

      tilesets.push_back(std::move(ts));
      // Find closing tag if it exists (or just move past current tag)
      size_t tilesetBlockEnd = content.find("</tileset>", tilesetStart);
      if (tilesetBlockEnd != std::string::npos &&
          tilesetBlockEnd < content.find("<layer ", tilesetStart)) {
        pos = tilesetBlockEnd + 10;
      } else {
        // Self-closing <tileset .../>
        pos = tilesetTagEnd + 1;
      }
    } else {
      // Embedded tileset
      size_t tilesetEnd = content.find("</tileset>", tilesetStart);
      std::string tilesetContent =
          content.substr(tilesetStart, tilesetEnd - tilesetStart);

      ts.name = extractAttribute(tilesetTag, "name");
      ts.tilewidth = std::stoi(extractAttribute(tilesetTag, "tilewidth"));
      ts.tileheight = std::stoi(extractAttribute(tilesetTag, "tileheight"));
      ts.tilecount = std::stoi(extractAttribute(tilesetTag, "tilecount"));
      ts.columns = std::stoi(extractAttribute(tilesetTag, "columns"));

      size_t imageStart = tilesetContent.find("<image ");
      if (imageStart != std::string::npos) {
        size_t imageEnd = tilesetContent.find(">", imageStart);
        std::string imageTag =
            tilesetContent.substr(imageStart, imageEnd - imageStart);
        std::string rawSource = extractAttribute(imageTag, "source");

        size_t lastSlash = rawSource.find_last_of("/\\");
        std::string filename = (lastSlash == std::string::npos)
                                   ? rawSource
                                   : rawSource.substr(lastSlash + 1);
        ts.imageSource = "assets/tilesets/" + filename;

        if (!ts.texture.loadFromFile(ts.imageSource)) {
          std::cerr << "Failed to load tileset image: " << ts.imageSource
                    << std::endl;
        }
      }

      tilesets.push_back(std::move(ts));
      pos = tilesetEnd + 10;
    }
  }

  // Parse layers
  pos = 0;
  while (true) {
    size_t layerStart = content.find("<layer ", pos);
    if (layerStart == std::string::npos)
      break;

    size_t layerTagEnd = content.find(">", layerStart);
    std::string layerTag = content.substr(layerStart, layerTagEnd - layerStart);
    std::string layerName = extractAttribute(layerTag, "name");

    // Find the CSV data within this layer
    size_t dataStart = content.find("<data encoding=\"csv\">", layerStart);
    size_t dataEnd = content.find("</data>", dataStart);

    if (dataStart != std::string::npos && dataEnd != std::string::npos) {
      std::string csvData = content.substr(
          dataStart + 21,
          dataEnd - (dataStart + 21)); // 21 = length of opening tag

      auto grid = parseLayerData(csvData, mapWidth, mapHeight);
      layers.push_back({layerName, grid});
    }

    pos = dataEnd + 7; // Move past </data>
  }

  // Find spawn and finish in all layers
  int spawnCount = 0;
  for (const auto &layer : layers) {
    for (size_t y = 0; y < layer.grid.size(); ++y) {
      for (size_t x = 0; x < layer.grid[y].size(); ++x) {
        uint32_t rawId = layer.grid[y][x];
        if (rawId == 0)
          continue;

        int id = static_cast<int>(rawId & TILE_MASK);
        const TilesetInfo *ts = getTilesetForId(id);
        if (!ts)
          continue;

        if (ts->name == "ts_main" || ts->name == "MainTileset") {
          int type = (id - ts->firstgid) % ts->columns;
          if (type == TileType::Start) {
            if (spawnCount > 0) {
              std::cerr << "Warning: Multiple spawn points found!" << std::endl;
            }
            startPosition = {
                static_cast<float>(x) * TILE_SIZE + TILE_SIZE / 2.f,
                static_cast<float>(y) * TILE_SIZE + TILE_SIZE / 2.f};
            spawnCount++;
          } else if (type == TileType::Finish) {
            bool flipH = (rawId & FLIP_H);
            bool flipV = (rawId & FLIP_V);
            bool flipD = (rawId & FLIP_D);

            float rot = 0.f;
            if (!flipD && !flipH && !flipV) {
              rot = 0.f;
            } else if (flipD && flipH && !flipV) {
              rot = 90.f;
            } else if (!flipD && flipH && flipV) {
              rot = 180.f;
            } else if (flipD && !flipH && flipV) {
              rot = 270.f;
            }
            // Consider mirrored cases, but standard rotations are these 4.
            else if (flipH) {
              rot = 0.f;
            } // H flip only
            else if (flipV) {
              rot = 180.f;
            } // V flip only

            sf::FloatRect trigger;
            if (rot == 0.f) {
              trigger = sf::FloatRect({static_cast<float>(x) * TILE_SIZE,
                                       static_cast<float>(y) * TILE_SIZE},
                                      {TILE_SIZE, 1.f});
            } else if (rot == 90.f) {
              trigger = sf::FloatRect(
                  {static_cast<float>(x) * TILE_SIZE + TILE_SIZE - 1.f,
                   static_cast<float>(y) * TILE_SIZE},
                  {1.f, TILE_SIZE});
            } else if (rot == 180.f) {
              trigger = sf::FloatRect(
                  {static_cast<float>(x) * TILE_SIZE,
                   static_cast<float>(y) * TILE_SIZE + TILE_SIZE - 1.f},
                  {TILE_SIZE, 1.f});
            } else if (rot == 270.f) {
              trigger = sf::FloatRect({static_cast<float>(x) * TILE_SIZE,
                                       static_cast<float>(y) * TILE_SIZE},
                                      {1.f, TILE_SIZE});
            } else {
              trigger = sf::FloatRect({static_cast<float>(x) * TILE_SIZE,
                                       static_cast<float>(y) * TILE_SIZE},
                                      {TILE_SIZE, 1.f});
            }

            finishAreas.push_back(trigger);
          }
        }
      }
    }
  }

  // Parse object groups (for text)
  parseObjectGroup(content);

  // Prepare cached text objects
  prepareTextObjects();

  std::cout << "Loaded TMX map: " << mapWidth << "x" << mapHeight << " tiles"
            << std::endl;
  std::cout << "Text objects found: " << textObjects.size() << std::endl;

  return !layers.empty();
}

std::vector<std::vector<uint32_t>>
Map::parseLayerData(const std::string &csvData, int width, int height) {
  std::vector<std::vector<uint32_t>> grid;
  std::stringstream ss(csvData);
  std::string line;

  while (std::getline(ss, line)) {
    // Skip empty lines
    if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos)
      continue;

    std::vector<uint32_t> row;
    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
      // Remove whitespace
      cell.erase(std::remove_if(cell.begin(), cell.end(), ::isspace),
                 cell.end());
      if (!cell.empty()) {
        try {
          uint32_t rawId = std::stoul(cell);
          row.push_back(rawId);
        } catch (...) {
          row.push_back(0);
        }
      }
    }

    if (!row.empty()) {
      grid.push_back(row);
    }
  }

  return grid;
}

void Map::parseObjectGroup(const std::string &content) {
  size_t pos = 0;

  while (true) {
    size_t objGroupStart = content.find("<objectgroup ", pos);
    if (objGroupStart == std::string::npos)
      break;

    size_t objGroupEnd = content.find("</objectgroup>", objGroupStart);
    if (objGroupEnd == std::string::npos)
      break;

    std::string objGroupContent =
        content.substr(objGroupStart, objGroupEnd - objGroupStart);

    // Check if this is the text layer
    size_t tagEnd = objGroupContent.find(">");
    std::string groupTag = objGroupContent.substr(0, tagEnd);
    std::string groupName = extractAttribute(groupTag, "name");

    if (groupName == "text") {
      // Parse each object in this group
      size_t objPos = 0;
      while (true) {
        size_t objStart = objGroupContent.find("<object ", objPos);
        if (objStart == std::string::npos)
          break;

        size_t objEnd = objGroupContent.find("</object>", objStart);
        if (objEnd == std::string::npos)
          break;

        std::string objContent =
            objGroupContent.substr(objStart, objEnd - objStart);

        // Extract object attributes
        size_t objTagEnd = objContent.find(">");
        std::string objTag = objContent.substr(0, objTagEnd);

        MapText text;
        text.name = extractAttribute(objTag, "name");

        std::string xStr = extractAttribute(objTag, "x");
        std::string yStr = extractAttribute(objTag, "y");
        std::string wStr = extractAttribute(objTag, "width");
        std::string hStr = extractAttribute(objTag, "height");

        if (!xStr.empty())
          text.position.x = std::stof(xStr);
        if (!yStr.empty())
          text.position.y = std::stof(yStr);
        if (!wStr.empty())
          text.size.x = std::stof(wStr);
        if (!hStr.empty())
          text.size.y = std::stof(hStr);

        // Extract text content between <text> tags
        size_t textStart = objContent.find("<text");
        if (textStart != std::string::npos) {
          size_t textContentStart = objContent.find(">", textStart) + 1;
          size_t textContentEnd = objContent.find("</text>", textContentStart);
          if (textContentEnd != std::string::npos) {
            text.content = objContent.substr(textContentStart,
                                             textContentEnd - textContentStart);
          }
        }

        if (!text.content.empty()) {
          textObjects.push_back(text);
        }

        objPos = objEnd + 9; // Move past </object>
      }
    }

    pos = objGroupEnd + 14; // Move past </objectgroup>
  }
}

void Map::render(sf::RenderWindow &window, sf::Vector2f playerPos,
                 bool showHitboxes) {
  // Get the current view bounds for culling
  sf::View view = window.getView();
  sf::Vector2f viewCenter = view.getCenter();
  sf::Vector2f viewSize = view.getSize();

  // Calculate visible tile range (with 1 tile margin for safety)
  // Use first layer for bounds since all layers should have same dimensions
  int gridHeight = layers.empty() ? 0 : static_cast<int>(layers[0].grid.size());
  int gridWidth =
      gridHeight > 0 ? static_cast<int>(layers[0].grid[0].size()) : 0;

  int startX = std::max(
      0, static_cast<int>((viewCenter.x - viewSize.x / 2.f) / TILE_SIZE) - 1);
  int startY = std::max(
      0, static_cast<int>((viewCenter.y - viewSize.y / 2.f) / TILE_SIZE) - 1);
  int endX = std::min(
      gridWidth,
      static_cast<int>((viewCenter.x + viewSize.x / 2.f) / TILE_SIZE) + 2);
  int endY = std::min(
      gridHeight,
      static_cast<int>((viewCenter.y + viewSize.y / 2.f) / TILE_SIZE) + 2);

  // Render all layers (back to front)
  for (const auto &layer : layers) {
    const auto &grid = layer.grid;
    if (grid.empty())
      continue;

    for (int y = startY; y < endY; ++y) {
      for (int x = startX; x < endX; ++x) {
        if (y >= 0 && y < static_cast<int>(grid.size()) && x >= 0 &&
            x < static_cast<int>(grid[y].size())) {
          uint32_t rawId = grid[y][x];

          // Skip empty tiles
          if (rawId == 0)
            continue;

          // Extract flip flags
          bool flipH = (rawId & FLIP_H);
          bool flipV = (rawId & FLIP_V);
          bool flipD = (rawId & FLIP_D);

          // Get actual tile ID (mask out any flip flags)
          int tileId = static_cast<int>(rawId & TILE_MASK);

          const TilesetInfo *ts = getTilesetForId(tileId);
          if (!ts)
            continue;

          // ts_main (collision block) should only render if showHitboxes is
          // true.
          if (ts->name == "ts_main" || ts->name == "MainTileset") {
            if (!showHitboxes)
              continue;
          }

          // Texture Rect logic based on this specific tileset
          int localId = tileId - ts->firstgid;
          int tileCol = localId % ts->columns;
          int tileRow = localId / ts->columns;

          // Calculate texture rect from tileset position
          int texX = tileCol * ts->tilewidth;
          int texY = tileRow * ts->tileheight;

          sf::Sprite tileSprite(ts->texture);
          tileSprite.setTextureRect(
              sf::IntRect({texX, texY}, {ts->tilewidth, ts->tileheight}));

          // Rotation and Flip Logic (Tiled to SFML mapping)
          float rot = 0.f;
          float sx = 1.f;
          float sy = 1.f;

          if (!flipD && !flipH && !flipV) {
            rot = 0.f;
          } else if (!flipD && flipH && !flipV) {
            sx = -1.f;
          } else if (!flipD && !flipH && flipV) {
            sy = -1.f;
          } else if (!flipD && flipH && flipV) {
            rot = 180.f;
          } else if (flipD && !flipH && !flipV) {
            rot = 270.f;
            sx = -1.f;
          } else if (flipD && flipH && !flipV) {
            rot = 90.f;
          } else if (flipD && !flipH && flipV) {
            rot = 270.f;
          } else if (flipD && flipH && flipV) {
            rot = 90.f;
            sx = -1.f;
          }

          // Use center origin so rotation and scaling behave independently
          tileSprite.setOrigin({TILE_SIZE / 2.f, TILE_SIZE / 2.f});
          tileSprite.setScale({sx, sy});
          tileSprite.setRotation(sf::degrees(rot));

          // Position must shift by half a tile to compensate for center origin
          tileSprite.setPosition(
              {static_cast<float>(x) * TILE_SIZE + TILE_SIZE / 2.f,
               static_cast<float>(y) * TILE_SIZE + TILE_SIZE / 2.f});

          window.draw(tileSprite);
        }
      }
    }
  }

  // Render cached text objects (no allocation in render loop)
  for (auto &text : cachedTexts) {
    // Proximity Fade Logic
    sf::Vector2f textPos = text.getPosition();
    sf::Vector2f textSize = text.getGlobalBounds().size;

    // Estimate text center for distance
    sf::Vector2f textCenter = {textPos.x + textSize.x / 2.f,
                               textPos.y + textSize.y / 2.f};

    // Calculate squared distance to player (avoid sqrt if possible, but for
    // linear fade we need sqrt)
    float dx = textCenter.x - playerPos.x;
    float dy = textCenter.y - playerPos.y;
    float distSq = dx * dx + dy * dy;
    float dist = std::sqrt(distSq);

    // Fade parameters (pixels)
    float fadeStartDist = 75.f; // Fully visible within this range
    float fadeEndDist = 150.f;  // Fully invisible outside this range

    float alpha = 0.f;
    if (dist < fadeStartDist) {
      alpha = 255.f;
    } else if (dist < fadeEndDist) {
      // Linearly interpolate
      float t = (dist - fadeStartDist) / (fadeEndDist - fadeStartDist);
      alpha = 255.f * (1.f - t);
    }

    // SFML 3.x uses u8 for Color constructor
    sf::Color color = text.getFillColor();
    color.a = static_cast<uint8_t>(alpha);
    text.setFillColor(color);

    sf::Color outline = text.getOutlineColor();
    outline.a = static_cast<uint8_t>(alpha);
    text.setOutlineColor(outline);

    if (alpha > 0) {
      window.draw(text);
    }
  }

  // Draw hitboxes for debugging if enabled
  if (showHitboxes) {
    sf::RectangleShape hazardShape;
    hazardShape.setFillColor(
        sf::Color(128, 0, 128, 100)); // Semi-transparent purple
    hazardShape.setOutlineColor(sf::Color(128, 0, 128));
    hazardShape.setOutlineThickness(1.f);

    for (const auto &layer : layers) {
      for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
          if (y >= 0 && y < static_cast<int>(layer.grid.size()) && x >= 0 &&
              x < static_cast<int>(layer.grid[y].size())) {
            int id = layer.grid[y][x] & TILE_MASK;
            if (id > 0) {
              const TilesetInfo *ts = getTilesetForId(id);
              if (!ts)
                continue;

              int localId = id - ts->firstgid;
              int functionalId = localId % ts->columns;

              if ((ts->name == "ts_main" || ts->name == "MainTileset") &&
                  functionalId == TileType::Spikes) {
                sf::FloatRect bounds({x * TILE_SIZE, y * TILE_SIZE},
                                     {TILE_SIZE, TILE_SIZE});
                bounds.position.x += 4.f;
                bounds.size.x -= 8.f;
                bounds.position.y += 10.f;
                bounds.size.y -= 10.f;

                hazardShape.setPosition(bounds.position);
                hazardShape.setSize(bounds.size);
                window.draw(hazardShape);
              }
            }
          }
        }
      }
    }
  }
}

// Prepare text objects once (called after map loading)
void Map::prepareTextObjects() {
  cachedTexts.clear();

  if (!fontLoaded)
    return;

  // Reserve space to avoid reallocations
  cachedTexts.reserve(textObjects.size());

  for (const auto &textObj : textObjects) {
    sf::Text text(font);
    text.setCharacterSize(12);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(1.f);

    // Manual word wrap based on object width from Tiled
    std::string wrappedText;
    std::string currentLine;
    std::string word;
    float maxWidth = textObj.size.x > 0 ? textObj.size.x : 100.f;

    for (size_t i = 0; i <= textObj.content.size(); ++i) {
      char c = (i < textObj.content.size()) ? textObj.content[i] : ' ';

      if (c == ' ' || c == '\n' || i == textObj.content.size()) {
        std::string testLine =
            currentLine.empty() ? word : currentLine + " " + word;
        text.setString(testLine);
        float lineWidth = text.getLocalBounds().size.x;

        if (lineWidth > maxWidth && !currentLine.empty()) {
          if (!wrappedText.empty())
            wrappedText += "\n";
          wrappedText += currentLine;
          currentLine = word;
        } else {
          currentLine = testLine;
        }
        word.clear();

        if (c == '\n') {
          if (!wrappedText.empty())
            wrappedText += "\n";
          wrappedText += currentLine;
          currentLine.clear();
        }
      } else {
        word += c;
      }
    }

    if (!currentLine.empty()) {
      if (!wrappedText.empty())
        wrappedText += "\n";
      wrappedText += currentLine;
    }

    text.setString(wrappedText);
    text.setPosition(textObj.position);
    cachedTexts.push_back(std::move(text));
  }
}

std::vector<sf::FloatRect>
Map::checkCollision(const sf::FloatRect &bounds) const {
  std::vector<sf::FloatRect> collisions;

  // Calculate tile range to check
  int left_tile = static_cast<int>(bounds.position.x / TILE_SIZE);
  int top_tile = static_cast<int>(bounds.position.y / TILE_SIZE);
  int right_tile =
      static_cast<int>((bounds.position.x + bounds.size.x) / TILE_SIZE);
  int bottom_tile =
      static_cast<int>((bounds.position.y + bounds.size.y) / TILE_SIZE);

  // Clamp to map bounds
  if (left_tile < 0)
    left_tile = 0;
  if (top_tile < 0)
    top_tile = 0;
  if (layers.empty() || layers[0].grid.empty())
    return collisions;
  if (right_tile >= static_cast<int>(layers[0].grid[0].size()))
    right_tile = static_cast<int>(layers[0].grid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(layers[0].grid.size()))
    bottom_tile = static_cast<int>(layers[0].grid.size()) - 1;

  for (const auto &layer : layers) {
    const auto &grid = layer.grid;
    for (int y = top_tile; y <= bottom_tile; ++y) {
      for (int x = left_tile; x <= right_tile; ++x) {
        if (y >= 0 && y < grid.size() && x >= 0 && x < grid[y].size()) {
          uint32_t rawId = grid[y][x];
          if (rawId != 0) {
            int id = static_cast<int>(rawId & TILE_MASK);
            const TilesetInfo *ts = getTilesetForId(id);
            if (ts && (ts->name == "ts_main" || ts->name == "MainTileset")) {
              int localId = id - ts->firstgid;
              int functionalId = localId % ts->columns;

              if (functionalId == TileType::Wall) {
                collisions.push_back(sf::FloatRect(
                    {x * TILE_SIZE, y * TILE_SIZE}, {TILE_SIZE, TILE_SIZE}));
              }
            }
          }
        }
      }
    }
  }

  return collisions;
}

bool Map::checkFinish(const sf::FloatRect &bounds) const {
  for (const auto &finishArea : finishAreas) {
    if (bounds.findIntersection(finishArea).has_value()) {
      return true;
    }
  }
  return false;
}

std::vector<sf::FloatRect>
Map::checkPlatformCollision(const sf::FloatRect &bounds) const {
  std::vector<sf::FloatRect> platforms;

  // Calculate tile range to check
  int left_tile = static_cast<int>(bounds.position.x / TILE_SIZE);
  int top_tile = static_cast<int>(bounds.position.y / TILE_SIZE);
  int right_tile =
      static_cast<int>((bounds.position.x + bounds.size.x) / TILE_SIZE);
  int bottom_tile =
      static_cast<int>((bounds.position.y + bounds.size.y) / TILE_SIZE);

  // Clamp to map bounds
  if (left_tile < 0)
    left_tile = 0;
  if (top_tile < 0)
    top_tile = 0;
  if (layers.empty() || layers[0].grid.empty())
    return platforms;
  if (right_tile >= static_cast<int>(layers[0].grid[0].size()))
    right_tile = static_cast<int>(layers[0].grid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(layers[0].grid.size()))
    bottom_tile = static_cast<int>(layers[0].grid.size()) - 1;

  for (const auto &layer : layers) {
    const auto &grid = layer.grid;
    for (int y = top_tile; y <= bottom_tile; ++y) {
      for (int x = left_tile; x <= right_tile; ++x) {
        if (y >= 0 && y < grid.size() && x >= 0 && x < grid[y].size()) {
          uint32_t rawId = grid[y][x];
          if (rawId != 0) {
            int id = static_cast<int>(rawId & TILE_MASK);
            const TilesetInfo *ts = getTilesetForId(id);
            if (ts && (ts->name == "ts_main" || ts->name == "MainTileset")) {
              int localId = id - ts->firstgid;
              int functionalId = localId % ts->columns;

              // Platform IDs
              if (functionalId == TileType::Platform) {
                platforms.push_back(sf::FloatRect(
                    {x * TILE_SIZE, y * TILE_SIZE}, {TILE_SIZE, TILE_SIZE}));
              }
            }
          }
        }
      }
    }
  }

  return platforms;
}

bool Map::checkSpikeCollision(const sf::FloatRect &bounds) const {
  int left_tile = static_cast<int>(bounds.position.x / TILE_SIZE);
  int top_tile = static_cast<int>(bounds.position.y / TILE_SIZE);
  int right_tile =
      static_cast<int>((bounds.position.x + bounds.size.x) / TILE_SIZE);
  int bottom_tile =
      static_cast<int>((bounds.position.y + bounds.size.y) / TILE_SIZE);

  if (left_tile < 0)
    left_tile = 0;
  if (top_tile < 0)
    top_tile = 0;
  if (layers.empty() || layers[0].grid.empty())
    return false;
  if (right_tile >= static_cast<int>(layers[0].grid[0].size()))
    right_tile = static_cast<int>(layers[0].grid[0].size()) - 1;
  if (bottom_tile >= static_cast<int>(layers[0].grid.size()))
    bottom_tile = static_cast<int>(layers[0].grid.size()) - 1;

  for (const auto &layer : layers) {
    const auto &grid = layer.grid;
    for (int y = top_tile; y <= bottom_tile; ++y) {
      for (int x = left_tile; x <= right_tile; ++x) {
        if (y >= 0 && y < grid.size() && x >= 0 && x < grid[y].size()) {
          uint32_t rawId = grid[y][x];
          if (rawId != 0) {
            int id = static_cast<int>(rawId & TILE_MASK);
            const TilesetInfo *ts = getTilesetForId(id);
            if (ts && (ts->name == "ts_main" || ts->name == "MainTileset")) {
              int localId = id - ts->firstgid;
              int functionalId = localId % ts->columns;

              if (functionalId == TileType::Spikes) {
                sf::FloatRect spikeBounds({x * TILE_SIZE, y * TILE_SIZE},
                                          {TILE_SIZE, TILE_SIZE});

                spikeBounds.position.x += 4.f;
                spikeBounds.size.x -= 8.f;
                spikeBounds.position.y += 10.f;
                spikeBounds.size.y -= 10.f;

                if (bounds.findIntersection(spikeBounds).has_value()) {
                  return true;
                }
              }
            }
          }
        }
      }
    }
  }

  return false;
}

const Map::TilesetInfo *Map::getTilesetForId(int globalId) const {
  if (globalId < 0 || tilesets.empty())
    return nullptr;

  // Tilesets are usually sorted by firstgid
  const TilesetInfo *bestMatch = nullptr;
  for (const auto &ts : tilesets) {
    if (globalId >= ts.firstgid) {
      if (!bestMatch || ts.firstgid > bestMatch->firstgid) {
        bestMatch = &ts;
      }
    }
  }
  return bestMatch;
}

//------------[Generate Colliders - Populate PhysicsWorld with Static Map Geometry]-------------------
void Map::generateColliders(Physics::PhysicsWorld &physicsWorld) const {
  for (const auto &layer : layers) {
    int h = static_cast<int>(layer.grid.size());
    if (h == 0) continue;
    int w = static_cast<int>(layer.grid[0].size());

    // 1. Solid Walls - Merged Horizontally
    for (int y = 0; y < h; ++y) {
      int startX = -1;
      for (int x = 0; x < w; ++x) {
        uint32_t rawId = layer.grid[y][x];
        bool isWall = false;
        if (rawId != 0) {
          int id = static_cast<int>(rawId & TILE_MASK);
          const TilesetInfo *ts = getTilesetForId(id);
          if (ts && (ts->name == "ts_main" || ts->name == "MainTileset")) {
            int localId = id - ts->firstgid;
            int functionalId = localId % ts->columns;
            if (functionalId == TileType::Wall) {
              isWall = true;
            }
          }
        }

        if (isWall) {
          if (startX == -1) startX = x;
        } else {
          if (startX != -1) {
            float width = static_cast<float>(x - startX) * TILE_SIZE;
            Physics::RigidBodyDef def;
            def.type = Physics::BodyType::Static;
            def.tag = Physics::ColliderTag::SolidWall;
            def.isOneWay = false;
            def.position = {static_cast<float>(startX) * TILE_SIZE, static_cast<float>(y) * TILE_SIZE};
            def.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {width, TILE_SIZE});
            def.friction = 0.5f;
            def.restitution = 0.0f;
            physicsWorld.createBody(def);
            startX = -1;
          }
        }
      }
      if (startX != -1) {
        float width = static_cast<float>(w - startX) * TILE_SIZE;
        Physics::RigidBodyDef def;
        def.type = Physics::BodyType::Static;
        def.tag = Physics::ColliderTag::SolidWall;
        def.isOneWay = false;
        def.position = {static_cast<float>(startX) * TILE_SIZE, static_cast<float>(y) * TILE_SIZE};
        def.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {width, TILE_SIZE});
        def.friction = 0.5f;
        def.restitution = 0.0f;
        physicsWorld.createBody(def);
      }
    }

    // 2. One-Way Platforms - Merged Horizontally
    for (int y = 0; y < h; ++y) {
      int startX = -1;
      for (int x = 0; x < w; ++x) {
        uint32_t rawId = layer.grid[y][x];
        bool isPlatform = false;
        if (rawId != 0) {
          int id = static_cast<int>(rawId & TILE_MASK);
          const TilesetInfo *ts = getTilesetForId(id);
          if (ts && (ts->name == "ts_main" || ts->name == "MainTileset")) {
            int localId = id - ts->firstgid;
            int functionalId = localId % ts->columns;
            if (functionalId == TileType::Platform) {
              isPlatform = true;
            }
          }
        }

        if (isPlatform) {
          if (startX == -1) startX = x;
        } else {
          if (startX != -1) {
            float width = static_cast<float>(x - startX) * TILE_SIZE;
            Physics::RigidBodyDef def;
            def.type = Physics::BodyType::Static;
            def.tag = Physics::ColliderTag::OneWayPlatform;
            def.isOneWay = true;
            def.position = {static_cast<float>(startX) * TILE_SIZE, static_cast<float>(y) * TILE_SIZE};
            def.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {width, TILE_SIZE});
            def.friction = 0.5f;
            def.restitution = 0.0f;
            physicsWorld.createBody(def);
            startX = -1;
          }
        }
      }
      if (startX != -1) {
        float width = static_cast<float>(w - startX) * TILE_SIZE;
        Physics::RigidBodyDef def;
        def.type = Physics::BodyType::Static;
        def.tag = Physics::ColliderTag::OneWayPlatform;
        def.isOneWay = true;
        def.position = {static_cast<float>(startX) * TILE_SIZE, static_cast<float>(y) * TILE_SIZE};
        def.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {width, TILE_SIZE});
        def.friction = 0.5f;
        def.restitution = 0.0f;
        physicsWorld.createBody(def);
      }
    }
  }
}
//-------------------------------------------------------
