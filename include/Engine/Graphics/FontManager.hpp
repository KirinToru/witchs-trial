#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace Engine::Graphics {

enum class FontType {
    Title,
    Pixel,
    Default
};

//------------[Round Coordinate - Align Scalar to Nearest Integer]-------------------
inline float roundCoord(float val) {
    return std::round(val);
}
//-------------------------------------------------------

//------------[Round Position - Align 2D Position to Integer Grid for Pixel-Perfect Fonts]-------------------
inline sf::Vector2f roundPosition(sf::Vector2f pos) {
    return {std::round(pos.x), std::round(pos.y)};
}
//-------------------------------------------------------

class FontManager {
public:
//------------[Get Instance - Singleton Access Point]-------------------
    static FontManager& getInstance();
//-------------------------------------------------------

//------------[Constructor - Initialize Font Manager Subsystem]-------------------
    FontManager();
//------------[Destructor - Default Cleanup]-------------------
    ~FontManager() = default;
//-------------------------------------------------------

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

//------------[Get Font - Access Loaded Font by Category Type]-------------------
    const sf::Font& getFont(FontType type = FontType::Default);
//-------------------------------------------------------

//------------[Has Font - Check If Category Font Successfully Loaded]-------------------
    bool hasFont(FontType type) const;
//-------------------------------------------------------

private:
//------------[Load Font With Fallback - Try Paths in Order and Apply Smoothing Configuration]-------------------
    bool loadFontWithFallback(FontType type, const std::vector<std::string>& paths, bool smooth);
//-------------------------------------------------------

    std::unordered_map<FontType, std::unique_ptr<sf::Font>> mFonts;
    sf::Font mFallbackFont;
    bool mFallbackLoaded{false};
};

} // namespace Engine::Graphics
