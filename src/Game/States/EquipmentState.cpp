#include <Game/States/EquipmentState.hpp>
#include <Game/Entities/Player.hpp>
#include <Game/Game.hpp>
#include <Engine/Audio/AudioManager.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <iomanip>
#include <sstream>

//------------[Constructor - Initialize Equipment Menu and HUD Panels]-------------------
EquipmentState::EquipmentState(Game* game, Player* player)
    : State(game), mPlayer(player) {
    mFontLoaded = mFont.openFromFile("assets/fonts/trebuc.ttf");
    if (!mFontLoaded) {
        mFontLoaded = mFont.openFromFile("assets/fonts/arial.ttf");
    }

    mDarkOverlay.setFillColor(sf::Color(10, 8, 18, 210));

    mMainPanel.setFillColor(sf::Color(22, 20, 34, 245));
    mMainPanel.setOutlineColor(sf::Color(160, 120, 240, 220));
    mMainPanel.setOutlineThickness(2.f);

    mHeaderBar.setFillColor(sf::Color(35, 28, 55, 255));
    mHeaderBar.setOutlineColor(sf::Color(200, 160, 80, 200));
    mHeaderBar.setOutlineThickness(1.f);

    // Initial Grimoire Spells
    mSpells.push_back({"Arcane Gun (K)", "18 MP", "Fast auto-aim bullet with immediate backward+upward recoil burst.", true});
    mSpells.push_back({"Pogo Magic Orb (L)", "25 MP", "Slow-moving aerial orb. Strike with melee (J) to super-bounce & reset dash.", mPlayer && mPlayer->getActiveSpell() == ActiveSpell::PogoOrb});
    mSpells.push_back({"Ice Wall (L)", "25 MP", "Solid crystalline ice pillar in front of player for wall-jumping. Lasts 4s.", mPlayer && mPlayer->getActiveSpell() == ActiveSpell::IceWall});
    mSpells.push_back({"Thunder Strike (L)", "20 MP", "Fast lightning bolt that stuns and electrifies foes. Touch to shock-jump.", mPlayer && mPlayer->getActiveSpell() == ActiveSpell::Thunder});
    mSpells.push_back({"Spirit Slash (J)", "20 STA", "Rapid melee cleave. Slices physical barriers and deflects pogo orbs.", true});

    // Initial Relics
    mRelics.push_back({"Silver Witch Pendant", "Rare", "+25% Natural Mana Regeneration Rate", true});
    mRelics.push_back({"Feather of Zephyr", "Relic", "Resets Air-Dash on Pogo Strike and extends apex hang", true});
    mRelics.push_back({"Obsidian Claw Charm", "Epic", "+35% Beast Form Heavy Strike Stagger Damage", false});
    mRelics.push_back({"Heart of the Coven", "Legendary", "+50 Max HP and 15% Damage Reduction", false});

    updateLayout();
}
//-------------------------------------------------------

//------------[Update Layout - Recompute Centered Panels & Responsive Bounds]-------------------
void EquipmentState::updateLayout() {
    sf::Vector2f viewSize = mGame->getWindow().getDefaultView().getSize();
    mDarkOverlay.setSize(viewSize);

    float panelW = std::min(viewSize.x - 120.f, 860.f);
    float panelH = std::min(viewSize.y - 100.f, 560.f);
    mMainPanel.setSize({panelW, panelH});
    mMainPanel.setOrigin({panelW * 0.5f, panelH * 0.5f});
    mMainPanel.setPosition({viewSize.x * 0.5f, viewSize.y * 0.5f});

    mHeaderBar.setSize({panelW, 50.f});
    mHeaderBar.setOrigin({panelW * 0.5f, 0.f});
    mHeaderBar.setPosition({viewSize.x * 0.5f, viewSize.y * 0.5f - panelH * 0.5f});
}
//-------------------------------------------------------

//------------[Handle Input - Process Navigation, Tab Switching & Upgrades]-------------------
void EquipmentState::handleInput(sf::Event& event) {
    if (const auto* keyPress = event.getIf<sf::Event::KeyPressed>()) {
        // Close menu
        if (keyPress->code == sf::Keyboard::Key::I || keyPress->code == sf::Keyboard::Key::Escape) {
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_close.wav", 80.f);
            mGame->popState();
            return;
        }

        // Switch Tabs with Tab or A/D or Left/Right
        if (keyPress->code == sf::Keyboard::Key::Tab) {
            int nextTab = (static_cast<int>(mCurrentTab) + 1) % 3;
            mCurrentTab = static_cast<Tab>(nextTab);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 75.f);
            return;
        }
        if (keyPress->code == sf::Keyboard::Key::Left) {
            int prevTab = (static_cast<int>(mCurrentTab) + 2) % 3;
            mCurrentTab = static_cast<Tab>(prevTab);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 75.f);
            return;
        }
        if (keyPress->code == sf::Keyboard::Key::Right) {
            int nextTab = (static_cast<int>(mCurrentTab) + 1) % 3;
            mCurrentTab = static_cast<Tab>(nextTab);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 75.f);
            return;
        }

        // Navigate within tab with W/S or Up/Down
        if (keyPress->code == sf::Keyboard::Key::W || keyPress->code == sf::Keyboard::Key::Up) {
            if (mCurrentTab == Tab::Grimoire && !mSpells.empty()) {
                mSelectedGrimoireIndex = (mSelectedGrimoireIndex - 1 + static_cast<int>(mSpells.size())) % static_cast<int>(mSpells.size());
            } else if (mCurrentTab == Tab::Attributes) {
                mSelectedStatIndex = (mSelectedStatIndex - 1 + 3) % 3;
            } else if (mCurrentTab == Tab::Relics && !mRelics.empty()) {
                mSelectedRelicIndex = (mSelectedRelicIndex - 1 + static_cast<int>(mRelics.size())) % static_cast<int>(mRelics.size());
            }
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 70.f);
        }
        if (keyPress->code == sf::Keyboard::Key::S || keyPress->code == sf::Keyboard::Key::Down) {
            if (mCurrentTab == Tab::Grimoire && !mSpells.empty()) {
                mSelectedGrimoireIndex = (mSelectedGrimoireIndex + 1) % static_cast<int>(mSpells.size());
            } else if (mCurrentTab == Tab::Attributes) {
                mSelectedStatIndex = (mSelectedStatIndex + 1) % 3;
            } else if (mCurrentTab == Tab::Relics && !mRelics.empty()) {
                mSelectedRelicIndex = (mSelectedRelicIndex + 1) % static_cast<int>(mRelics.size());
            }
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 70.f);
        }

        // Action: Enter or Space
        if (keyPress->code == sf::Keyboard::Key::Enter || keyPress->code == sf::Keyboard::Key::Space) {
            if (mCurrentTab == Tab::Grimoire && mPlayer) {
                if (mSelectedGrimoireIndex == 1) {
                    mPlayer->setActiveSpell(ActiveSpell::PogoOrb);
                    mSpells[1].isEquipped = true;
                    mSpells[2].isEquipped = false;
                    mSpells[3].isEquipped = false;
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/equip.wav", 85.f);
                } else if (mSelectedGrimoireIndex == 2) {
                    mPlayer->setActiveSpell(ActiveSpell::IceWall);
                    mSpells[1].isEquipped = false;
                    mSpells[2].isEquipped = true;
                    mSpells[3].isEquipped = false;
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/equip.wav", 85.f);
                } else if (mSelectedGrimoireIndex == 3) {
                    mPlayer->setActiveSpell(ActiveSpell::Thunder);
                    mSpells[1].isEquipped = false;
                    mSpells[2].isEquipped = false;
                    mSpells[3].isEquipped = true;
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/equip.wav", 85.f);
                } else {
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_click.wav", 75.f);
                }
            } else if (mCurrentTab == Tab::Attributes && mPlayer) {
                if (mPlayer->spendSkillPoint()) {
                    if (mSelectedStatIndex == 0) {
                        mPlayer->upgradeMaxHealth(20.f);
                    } else if (mSelectedStatIndex == 1) {
                        mPlayer->upgradeMaxMana(20.f);
                    } else if (mSelectedStatIndex == 2) {
                        mPlayer->upgradeMaxStamina(20.f);
                    }
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/upgrade_success.wav", 90.f);
                } else {
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/menu_error.wav", 75.f);
                }
            } else if (mCurrentTab == Tab::Relics) {
                if (mSelectedRelicIndex >= 0 && mSelectedRelicIndex < static_cast<int>(mRelics.size())) {
                    mRelics[mSelectedRelicIndex].isEquipped = !mRelics[mSelectedRelicIndex].isEquipped;
                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/equip.wav", 85.f);
                }
            }
        }
    }
}
//-------------------------------------------------------

//------------[Update - Step Menu Animations & Timers]-------------------
void EquipmentState::update(sf::Time dt) {
    mAnimationTimer += dt.asSeconds();
    updateLayout();
}
//-------------------------------------------------------

//------------[Render - Draw Semi-Transparent Overlay and High-Tech Fantasy UI]-------------------
void EquipmentState::render(sf::RenderWindow& window) {
    sf::View oldView = window.getView();
    window.setView(window.getDefaultView());

    // Dark backdrop
    window.draw(mDarkOverlay);

    // Main frame
    window.draw(mMainPanel);
    window.draw(mHeaderBar);

    if (!mFontLoaded) {
        window.setView(oldView);
        return;
    }

    sf::FloatRect panelBounds = mMainPanel.getGlobalBounds();

    // Title
    sf::Text titleText(mFont, "EQUIPMENT & GRIMOIRE", 22);
    titleText.setFillColor(sf::Color(255, 220, 120));
    titleText.setStyle(sf::Text::Bold);
    titleText.setPosition({panelBounds.position.x + 24.f, panelBounds.position.y + 12.f});
    window.draw(titleText);

    // Tab Headers
    const char* tabNames[3] = {"[1] Grimoire Magic", "[2] Stats & Upgrades", "[3] Relics & Charms"};
    for (int i = 0; i < 3; ++i) {
        sf::Text tabText(mFont, tabNames[i], 16);
        float tabX = panelBounds.position.x + 320.f + i * 175.f;
        tabText.setPosition({tabX, panelBounds.position.y + 16.f});

        if (static_cast<int>(mCurrentTab) == i) {
            tabText.setFillColor(sf::Color(140, 240, 255));
            tabText.setStyle(sf::Text::Bold | sf::Text::Underlined);
        } else {
            tabText.setFillColor(sf::Color(180, 180, 195));
        }
        window.draw(tabText);
    }

    // Render active tab contents
    if (mCurrentTab == Tab::Grimoire) {
        renderGrimoirePanel(window);
    } else if (mCurrentTab == Tab::Attributes) {
        renderAttributesPanel(window);
    } else if (mCurrentTab == Tab::Relics) {
        renderRelicsPanel(window);
    }

    // Footer Navigation Help
    std::string footerStr = "[TAB/Left/Right] Switch Tab  |  [W/S] Navigate  |  [ENTER/SPACE] Equip/Upgrade  |  [I/ESC] Close";
    sf::Text footerText(mFont, footerStr, 13);
    footerText.setFillColor(sf::Color(160, 160, 180));
    footerText.setPosition({panelBounds.position.x + 24.f, panelBounds.position.y + panelBounds.size.y - 28.f});
    window.draw(footerText);

    window.setView(oldView);
}
//-------------------------------------------------------

//------------[Render Grimoire Panel - Draw Spell Selection Slots and Descriptions]-------------------
void EquipmentState::renderGrimoirePanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds = mMainPanel.getGlobalBounds();
    float startX = panelBounds.position.x + 30.f;
    float startY = panelBounds.position.y + 70.f;

    for (size_t i = 0; i < mSpells.size(); ++i) {
        bool selected = (static_cast<int>(i) == mSelectedGrimoireIndex);
        float cardY = startY + i * 82.f;

        sf::RectangleShape card({panelBounds.size.x - 60.f, 74.f});
        card.setPosition({startX, cardY});
        if (selected) {
            card.setFillColor(sf::Color(45, 38, 70, 240));
            card.setOutlineColor(sf::Color(255, 215, 100));
            card.setOutlineThickness(2.f);
        } else {
            card.setFillColor(sf::Color(28, 25, 42, 220));
            card.setOutlineColor(sf::Color(90, 75, 120, 180));
            card.setOutlineThickness(1.f);
        }
        window.draw(card);

        // Spell Name & Cost
        sf::Text nameText(mFont, mSpells[i].name, 17);
        nameText.setFillColor(selected ? sf::Color(255, 235, 160) : sf::Color(230, 230, 240));
        nameText.setStyle(sf::Text::Bold);
        nameText.setPosition({startX + 16.f, cardY + 8.f});
        window.draw(nameText);

        sf::Text costText(mFont, mSpells[i].cost, 15);
        costText.setFillColor(sf::Color(120, 200, 255));
        costText.setPosition({startX + 280.f, cardY + 10.f});
        window.draw(costText);

        // Status badge
        sf::Text statusText(mFont, mSpells[i].isEquipped ? "[ EQUIPPED ]" : "[ AVAILABLE ]", 14);
        statusText.setFillColor(mSpells[i].isEquipped ? sf::Color(100, 255, 140) : sf::Color(170, 170, 180));
        statusText.setPosition({startX + panelBounds.size.x - 190.f, cardY + 10.f});
        window.draw(statusText);

        // Description
        sf::Text descText(mFont, mSpells[i].description, 14);
        descText.setFillColor(sf::Color(190, 190, 210));
        descText.setPosition({startX + 16.f, cardY + 38.f});
        window.draw(descText);
    }
}
//-------------------------------------------------------

//------------[Render Attributes Panel - Draw Player Stats and Upgrade Buttons]-------------------
void EquipmentState::renderAttributesPanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds = mMainPanel.getGlobalBounds();
    float startX = panelBounds.position.x + 30.f;
    float startY = panelBounds.position.y + 70.f;

    // Skill Points Header
    int points = mPlayer ? mPlayer->getSkillPoints() : 0;
    std::string pointsStr = "Available Shard Points: " + std::to_string(points);
    sf::Text pointsText(mFont, pointsStr, 18);
    pointsText.setFillColor(sf::Color(255, 215, 100));
    pointsText.setStyle(sf::Text::Bold);
    pointsText.setPosition({startX, startY});
    window.draw(pointsText);

    struct StatRow {
        std::string name;
        float currentVal;
        float maxVal;
        std::string upgradeText;
        sf::Color barColor;
    };

    std::vector<StatRow> statRows;
    if (mPlayer) {
        statRows.push_back({"Vitality (Max Health)", mPlayer->getHealth(), mPlayer->getMaxHealth(), "+20 Max HP", sf::Color(230, 70, 70)});
        statRows.push_back({"Arcana (Max Mana)", mPlayer->getMana(), mPlayer->getMaxMana(), "+20 Max MP", sf::Color(70, 160, 255)});
        statRows.push_back({"Endurance (Max Stamina)", mPlayer->getStamina(), mPlayer->getMaxStamina(), "+20 Max STA", sf::Color(70, 220, 120)});
    }

    for (size_t i = 0; i < statRows.size(); ++i) {
        bool selected = (static_cast<int>(i) == mSelectedStatIndex);
        float rowY = startY + 50.f + i * 95.f;

        sf::RectangleShape box({panelBounds.size.x - 60.f, 80.f});
        box.setPosition({startX, rowY});
        if (selected) {
            box.setFillColor(sf::Color(45, 38, 70, 240));
            box.setOutlineColor(sf::Color(255, 215, 100));
            box.setOutlineThickness(2.f);
        } else {
            box.setFillColor(sf::Color(28, 25, 42, 220));
            box.setOutlineColor(sf::Color(90, 75, 120, 180));
            box.setOutlineThickness(1.f);
        }
        window.draw(box);

        // Stat Title & Values
        std::ostringstream ss;
        ss << statRows[i].name << " : " << static_cast<int>(statRows[i].maxVal);
        sf::Text labelText(mFont, ss.str(), 17);
        labelText.setFillColor(selected ? sf::Color(255, 235, 160) : sf::Color(230, 230, 240));
        labelText.setPosition({startX + 16.f, rowY + 10.f});
        window.draw(labelText);

        // Progress Gauge Background
        float gaugeW = 320.f;
        sf::RectangleShape gaugeBg({gaugeW, 14.f});
        gaugeBg.setPosition({startX + 16.f, rowY + 44.f});
        gaugeBg.setFillColor(sf::Color(20, 20, 30));
        gaugeBg.setOutlineColor(sf::Color(70, 70, 90));
        gaugeBg.setOutlineThickness(1.f);
        window.draw(gaugeBg);

        // Progress Gauge Fill
        float fillRatio = std::clamp(statRows[i].maxVal / 250.f, 0.1f, 1.0f);
        sf::RectangleShape gaugeFill({gaugeW * fillRatio, 14.f});
        gaugeFill.setPosition({startX + 16.f, rowY + 44.f});
        gaugeFill.setFillColor(statRows[i].barColor);
        window.draw(gaugeFill);

        // Upgrade button prompt
        sf::Text btnText(mFont, "[ENTER: Upgrade " + statRows[i].upgradeText + "]", 15);
        btnText.setFillColor(points > 0 ? (selected ? sf::Color(255, 220, 100) : sf::Color(160, 240, 160)) : sf::Color(120, 120, 130));
        btnText.setPosition({startX + panelBounds.size.x - 310.f, rowY + 28.f});
        window.draw(btnText);
    }
}
//-------------------------------------------------------

//------------[Render Relics Panel - Draw Equipment Inventory and Charms]-------------------
void EquipmentState::renderRelicsPanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds = mMainPanel.getGlobalBounds();
    float startX = panelBounds.position.x + 30.f;
    float startY = panelBounds.position.y + 70.f;

    for (size_t i = 0; i < mRelics.size(); ++i) {
        bool selected = (static_cast<int>(i) == mSelectedRelicIndex);
        float cardY = startY + i * 90.f;

        sf::RectangleShape card({panelBounds.size.x - 60.f, 78.f});
        card.setPosition({startX, cardY});
        if (selected) {
            card.setFillColor(sf::Color(45, 38, 70, 240));
            card.setOutlineColor(sf::Color(255, 215, 100));
            card.setOutlineThickness(2.f);
        } else {
            card.setFillColor(sf::Color(28, 25, 42, 220));
            card.setOutlineColor(sf::Color(90, 75, 120, 180));
            card.setOutlineThickness(1.f);
        }
        window.draw(card);

        // Relic Name & Rarity
        sf::Text nameText(mFont, mRelics[i].name, 18);
        nameText.setFillColor(selected ? sf::Color(255, 235, 160) : sf::Color(230, 230, 240));
        nameText.setStyle(sf::Text::Bold);
        nameText.setPosition({startX + 16.f, cardY + 10.f});
        window.draw(nameText);

        sf::Text rarityText(mFont, "(" + mRelics[i].rarity + ")", 14);
        rarityText.setFillColor(sf::Color(220, 160, 255));
        rarityText.setPosition({startX + 280.f, cardY + 12.f});
        window.draw(rarityText);

        // Status badge
        sf::Text statusText(mFont, mRelics[i].isEquipped ? "[ EQUIPPED ]" : "[ UNEQUIPPED ]", 14);
        statusText.setFillColor(mRelics[i].isEquipped ? sf::Color(100, 255, 140) : sf::Color(170, 170, 180));
        statusText.setPosition({startX + panelBounds.size.x - 190.f, cardY + 12.f});
        window.draw(statusText);

        // Effect
        sf::Text effectText(mFont, mRelics[i].effect, 14);
        effectText.setFillColor(sf::Color(190, 190, 210));
        effectText.setPosition({startX + 16.f, cardY + 42.f});
        window.draw(effectText);
    }
}
//-------------------------------------------------------
