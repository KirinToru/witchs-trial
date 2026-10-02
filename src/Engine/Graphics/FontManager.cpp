#include <Engine/Graphics/FontManager.hpp>
#include <filesystem>
#include <iostream>

namespace Engine::Graphics {

//------------[Get Instance - Singleton Access Point]-------------------
FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}
//-------------------------------------------------------

//------------[Constructor - Initialize Font Manager Subsystem]-------------------
FontManager::FontManager() {
    const std::vector<std::string> titlePaths = {
        "assets/fonts/Cinzel.ttf",
        "assets/fonts/Cinzel-Regular.ttf",
        "assets/fonts/trebuc.ttf",
        "C:/Windows/Fonts/georgia.ttf",
        "C:/Windows/Fonts/times.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    const std::vector<std::string> pixelPaths = {
        "assets/fonts/PixeloidSans.ttf",
        "assets/fonts/Pixeloid.ttf",
        "assets/fonts/PixeloidSans-Bold.ttf",
        "assets/fonts/trebuc.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    const std::vector<std::string> defaultPaths = {
        "assets/fonts/trebuc.ttf",
        "assets/fonts/Cinzel.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    loadFontWithFallback(FontType::Title, titlePaths, true);
    loadFontWithFallback(FontType::Pixel, pixelPaths, false);
    loadFontWithFallback(FontType::Default, defaultPaths, true);

    if (std::filesystem::exists("assets/fonts/trebuc.ttf")) {
        mFallbackLoaded = mFallbackFont.openFromFile("assets/fonts/trebuc.ttf");
    } else if (std::filesystem::exists("C:/Windows/Fonts/arial.ttf")) {
        mFallbackLoaded = mFallbackFont.openFromFile("C:/Windows/Fonts/arial.ttf");
    }
}
//-------------------------------------------------------

//------------[Load Font With Fallback - Try Paths in Order and Apply Smoothing Configuration]-------------------
bool FontManager::loadFontWithFallback(FontType type, const std::vector<std::string>& paths, bool smooth) {
    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            auto font = std::make_unique<sf::Font>();
            if (font->openFromFile(path)) {
                font->setSmooth(smooth);
                mFonts[type] = std::move(font);
                return true;
            }
        }
    }
    return false;
}
//-------------------------------------------------------

//------------[Get Font - Access Loaded Font by Category Type]-------------------
const sf::Font& FontManager::getFont(FontType type) {
    auto it = mFonts.find(type);
    if (it != mFonts.end() && it->second) {
        return *(it->second);
    }
    auto defIt = mFonts.find(FontType::Default);
    if (defIt != mFonts.end() && defIt->second) {
        return *(defIt->second);
    }
    return mFallbackFont;
}
//-------------------------------------------------------

//------------[Has Font - Check If Category Font Successfully Loaded]-------------------
bool FontManager::hasFont(FontType type) const {
    auto it = mFonts.find(type);
    return (it != mFonts.end() && it->second != nullptr);
}
//-------------------------------------------------------

} // namespace Engine::Graphics
