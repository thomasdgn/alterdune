#ifndef GAME_H
#define GAME_H

#include "ActAction.h"
#include "BestiaryEntry.h"
#include "Monster.h"
#include "Player.h"

#include <map>
#include <random>
#include <string>
#include <vector>

class Game
{
public:
    Game();

    bool initialize();
    void run();

private:
    Player m_player;
    std::vector<Monster> m_monsterCatalog;
    std::vector<BestiaryEntry> m_bestiary;
    std::map<std::string, ActAction> m_actCatalog;
    std::mt19937 m_rng;

    void initializeActCatalog();
    bool promptPlayerName();
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

    Monster createRandomMonster();
    int randomInt(int minValue, int maxValue);
    void recordBattleResult(const Monster& monster, const std::string& result);

    static std::string trim(const std::string& value);
    static std::vector<std::string> split(const std::string& line, char delimiter);
};

#endif
