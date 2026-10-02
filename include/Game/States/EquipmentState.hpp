#pragma once

#include <Engine/States/State.hpp>
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class Player;

class EquipmentState : public State {
public:
//------------[Constructor - Initialize Equipment Menu and HUD Panels]-------------------
    EquipmentState(Game* game, Player* player);
//-------------------------------------------------------

//------------[Destructor - Cleanup Resources]-------------------
    ~EquipmentState() override = default;
//-------------------------------------------------------

//------------[Handle Input - Process Navigation, Tab Switching & Upgrades]-------------------
    void handleInput(sf::Event& event) override;
//-------------------------------------------------------

//------------[Update - Step Menu Animations & Timers]-------------------
    void update(sf::Time dt) override;
//-------------------------------------------------------

//------------[Render - Draw Semi-Transparent Overlay and High-Tech Fantasy UI]-------------------
    void render(sf::RenderWindow& window) override;
//-------------------------------------------------------

private:
//------------[Update Layout - Recompute Centered Panels & Responsive Bounds]-------------------
    void updateLayout();
//-------------------------------------------------------

//------------[Render Grimoire Panel - Draw Spell Selection Slots and Descriptions]-------------------
    void renderGrimoirePanel(sf::RenderWindow& window);
//-------------------------------------------------------

//------------[Render Attributes Panel - Draw Player Stats and Upgrade Buttons]-------------------
    void renderAttributesPanel(sf::RenderWindow& window);
//-------------------------------------------------------

//------------[Render Relics Panel - Draw Equipment Inventory and Charms]-------------------
    void renderRelicsPanel(sf::RenderWindow& window);
//-------------------------------------------------------

    Player* mPlayer{nullptr};
    sf::Font mFont;
    bool mFontLoaded{false};

    enum class Tab {
        Grimoire = 0,
        Attributes = 1,
        Relics = 2
    };
    Tab mCurrentTab{Tab::Grimoire};

    int mSelectedGrimoireIndex{0};
    int mSelectedStatIndex{0};
    int mSelectedRelicIndex{0};

    sf::RectangleShape mDarkOverlay;
    sf::RectangleShape mMainPanel;
    sf::RectangleShape mHeaderBar;

    struct SpellInfo {
        std::string name;
        std::string cost;
        std::string description;
        bool isEquipped;
    };
    std::vector<SpellInfo> mSpells;

    struct RelicInfo {
        std::string name;
        std::string rarity;
        std::string effect;
        bool isEquipped;
    };
    std::vector<RelicInfo> mRelics;

    float mAnimationTimer{0.f};
};
