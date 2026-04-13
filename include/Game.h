#ifndef GAME_H
#define GAME_H

#include "ActAction.h"
#include "BestiaryEntry.h"
#include "Monster.h"
#include "FrontendViewModels.h"
#include "Player.h"

#include <memory>
#include <map>
#include <random>
#include <string>
#include <vector>

class Game
{
public:
    Game();

    bool initialize();
    bool initializeForFrontend(const std::string& playerName = "Traveler");
    void setPlayerAppearance(const std::string& appearanceId);
    const std::string& getPlayerAppearance() const;
    void run();
    FrontendMenuViewData buildFrontendMenuViewData() const;
    FrontendPlayerStatsViewData buildFrontendPlayerStatsViewData() const;
    std::vector<FrontendMonsterCardViewData> buildFrontendMonsterSelectionViewData() const;
    FrontendBattleViewData buildFrontendBattlePreviewViewData(std::size_t displayIndex = 0) const;
    std::vector<FrontendBestiaryEntryViewData> buildFrontendBestiaryViewData() const;
    std::vector<FrontendInventoryItemViewData> buildFrontendInventoryViewData() const;
    bool startFrontendBattle(std::size_t displayIndex);
    FrontendBattleViewData buildActiveFrontendBattleViewData() const;
    std::string performFrontendBattleAction(const std::string& actionId);
    std::vector<FrontendActionButtonViewData> buildFrontendFightOptionViewData() const;
    std::vector<FrontendActionButtonViewData> buildFrontendActOptionViewData() const;
    std::vector<FrontendActionButtonViewData> buildFrontendItemOptionViewData() const;
    std::string performFrontendFightByIndex(std::size_t optionIndex);
    std::string performFrontendActByIndex(std::size_t optionIndex);
    std::string performFrontendItemByIndex(std::size_t optionIndex);
    bool hasActiveFrontendBattle() const;
    void leaveFrontendBattle();
    static std::vector<std::string> getAvailableHeroAppearanceIds();
    static std::string getHeroAppearanceLabel(const std::string& appearanceId);

private:
    enum class BattleTurnResult
    {
        CONTINUE,
        PLAYER_WON_KILL,
        PLAYER_WON_SPARE,
        PLAYER_FLED,
        PLAYER_LOST,
        NO_TURN_SPENT
    };

    Player m_player;
    std::vector<std::unique_ptr<Monster>> m_monsterCatalog;
    std::vector<BestiaryEntry> m_bestiary;
    std::map<std::string, ActAction> m_actCatalog;
    std::mt19937 m_rng;
    int m_nextFightBonus;
    int m_nextActBonus;
    int m_guardCharges;
    int m_nextFightPenalty;
    int m_nextActPenalty;
    int m_battleTurnCounter;
    bool m_hpMilestoneUnlocked;
    bool m_atkMilestoneUnlocked;
    bool m_defMilestoneUnlocked;
    std::unique_ptr<Monster> m_frontendBattleMonster;
    std::map<std::string, int> m_frontendActUsage;
    std::string m_frontendBattleLog;
    std::string m_lastPlayerTurnSummary;
    std::string m_lastMonsterTurnSummary;

    void initializeActCatalog();
    bool promptPlayerName();
    void promptPlayerAppearance();
    bool loadItemsFromCsv(const std::string& filePath);
    bool loadMonstersFromCsv(const std::string& filePath);

    void displayStartupSummary() const;
    void displayMainMenu() const;
    int readIntChoice(int minValue, int maxValue) const;
    void handleMenuChoice(int choice);

    void showBestiary() const;
    void showPlayerStats() const;
    void showItems();
    void startBattle();
    int chooseMonsterIndex() const;
    std::vector<std::size_t> getAvailableMonsterIndices() const;
    bool isMonsterUnlocked(const Monster& monster) const;
    void displayBattleState(const Monster& monster) const;
    BattleTurnResult handleFightAction(Monster& monster);
    BattleTurnResult handleActAction(Monster& monster, std::map<std::string, int>& actUsage);
    BattleTurnResult handleItemAction(Monster& monster);
    BattleTurnResult handleMercyAction(Monster& monster);
    BattleTurnResult handleMonsterTurn(Monster& monster);
    void concludeBattle(const Monster& monster, BattleTurnResult result);
    std::string getMercyComment(const Monster& monster) const;
    std::string buildMercyBar(const Monster& monster) const;
    std::string getBattleOpeningText(const Monster& monster) const;
    std::string getBattleHint(const Monster& monster) const;
    std::string getMonsterDescription(const Monster& monster) const;
    std::string getMonsterRegionName(const Monster& monster) const;
    std::string getMonsterPhysicalProfile(const Monster& monster) const;
    std::string getHeroAppearanceBonusText() const;
    std::string getMonsterMoodText(const Monster& monster) const;
    std::string getMonsterAttackText(const Monster& monster) const;
    std::string getMonsterAttackType(const Monster& monster) const;
    std::string getMonsterElementType(const Monster& monster) const;
    std::string getAttackElementType(const std::string& attackStyleId) const;
    float getElementalEffectiveness(const std::string& attackElement, const std::string& targetElement) const;
    std::string getAttackStyleLabel(const std::string& attackStyleId) const;
    std::string getAttackStyleDescription(const std::string& attackStyleId) const;
    std::vector<std::string> getAvailableAttackStyleIds() const;
    float getPlayerAttackMultiplier(const Monster& monster, const std::string& attackStyleId) const;
    float getMonsterAttackMultiplier(const Monster& monster) const;
    int resolvePlayerFightAction(Monster& monster,
                                 const std::string& attackStyleId,
                                 bool& criticalHit,
                                 std::string& effectivenessText,
                                 std::string& attackLabel);
    std::string getItemTacticalEffect(const std::string& itemName) const;
    int computeActMercyImpact(const Monster& monster,
                              const ActAction& action,
                              const std::map<std::string, int>& actUsage) const;
    std::string getActReactionText(const Monster& monster, const ActAction& action, int delta) const;
    void displayActOutcome(const Monster& monster, const ActAction& action, int previousMercy) const;
    void grantBattleReward(const Monster& monster, BattleTurnResult result);
    void grantRandomItemReward(const Monster& monster, BattleTurnResult result);
    void tryGrantRareBattleBlessing(const Monster& monster);
    void applyItemBattleEffect(const Item& item);
    void resetBattleModifiers();
    void checkProgressMilestones();
    const Item* findFirstUsableInventoryItem() const;
    const ActAction* chooseBestFrontendAct(const Monster& monster) const;
    FrontendBattleViewData buildFrontendBattleViewData(const Monster& monster, const std::string& battleLogText) const;
    bool hasReachedEnding() const;
    void displayEndingAndExit();

    std::unique_ptr<Monster> createRandomMonster();
    int randomInt(int minValue, int maxValue);
    void recordBattleResult(const Monster& monster, const std::string& result);

    static std::string trim(const std::string& value);
    static std::vector<std::string> split(const std::string& line, char delimiter);
};

#endif
