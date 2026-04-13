#ifndef FRONTENDVIEWMODELS_H
#define FRONTENDVIEWMODELS_H

#include <string>
#include <vector>

struct FrontendStatBarViewData
{
    std::string label;
    int currentValue;
    int maxValue;
};

struct FrontendActionButtonViewData
{
    std::string id;
    std::string label;
    bool enabled;
};

struct FrontendMenuViewData
{
    std::string playerName;
    std::string progressText;
    std::string unlockedLandsText;
    std::vector<FrontendActionButtonViewData> buttons;
};

struct FrontendPlayerStatsViewData
{
    std::string playerName;
    std::string routeText;
    std::string milestoneText;
    std::string appearanceText;
    FrontendStatBarViewData hpBar;
    int atk;
    int def;
    int victories;
    int kills;
    int spares;
    int inventorySlots;
    int totalHealingStock;
};

struct FrontendMonsterCardViewData
{
    std::string name;
    std::string category;
    std::string elementType;
    std::string land;
    std::string threat;
    std::string physique;
    std::string description;
    int hp;
    int atk;
    int def;
    bool unlocked;
};

struct FrontendBattleViewData
{
    std::string regionName;
    std::string openingText;
    std::string moodText;
    std::string hintText;
    std::string battleLogText;
    std::string playerActionText;
    std::string monsterActionText;
    std::string activeBonusesText;
    std::string playerName;
    std::string monsterName;
    std::string monsterCategory;
    std::string monsterElementType;
    std::string monsterPhysique;
    std::string monsterDescription;
    FrontendStatBarViewData playerHpBar;
    FrontendStatBarViewData monsterHpBar;
    FrontendStatBarViewData mercyBar;
    std::vector<FrontendActionButtonViewData> primaryActions;
    std::vector<FrontendActionButtonViewData> secondaryActions;
};

struct FrontendBestiaryEntryViewData
{
    std::string name;
    std::string category;
    std::string elementType;
    std::string land;
    std::string physique;
    std::string result;
    std::string description;
    int hp;
    int atk;
    int def;
};

struct FrontendInventoryItemViewData
{
    std::string name;
    std::string type;
    std::string tacticalEffect;
    int value;
    int quantity;
};

#endif
