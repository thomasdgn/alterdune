#include "Game.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

namespace
{
bool isValidMonsterCategoryString(const std::string& value)
{
    return value == "NORMAL" || value == "MINIBOSS" || value == "BOSS";
}
}

Game::Game()
    : m_player(),
      m_rng(std::random_device{}())
{
}

bool Game::initialize()
{
    initializeActCatalog();

    if (!promptPlayerName())
    {
        return false;
    }

    if (!loadItemsFromCsv("data/items.csv"))
    {
        return false;
    }

    if (!loadMonstersFromCsv("data/monsters.csv"))
    {
        return false;
    }

    displayStartupSummary();
    return true;
}

void Game::run()
{
    bool isRunning = true;

    while (isRunning)
    {
        displayMainMenu();
        const int choice = readIntChoice(1, 5);
        if (choice == 5)
        {
            std::cout << "Thanks for playing ALTERDUNE TD11-12.\n";
            isRunning = false;
        }
        else
        {
            handleMenuChoice(choice);
        }
    }
}

void Game::initializeActCatalog()
{
    m_actCatalog.clear();
    m_actCatalog.emplace("JOKE", ActAction("JOKE", "You crack a weird desert joke.", 18));
    m_actCatalog.emplace("COMPLIMENT", ActAction("COMPLIMENT", "You compliment the monster's style.", 25));
    m_actCatalog.emplace("DISCUSS", ActAction("DISCUSS", "You try to calmly discuss the situation.", 20));
    m_actCatalog.emplace("OBSERVE", ActAction("OBSERVE", "You observe carefully and learn a weakness.", 10));
    m_actCatalog.emplace("PET", ActAction("PET", "You offer a friendly gesture.", 22));
    m_actCatalog.emplace("OFFER_SNACK", ActAction("OFFER_SNACK", "You offer a snack from your bag.", 30));
    m_actCatalog.emplace("REASON", ActAction("REASON", "You appeal to reason and empathy.", 28));
    m_actCatalog.emplace("DANCE", ActAction("DANCE", "You start an awkward but sincere dance.", 16));
    m_actCatalog.emplace("INSULT", ActAction("INSULT", "You provoke the monster.", -20));
    m_actCatalog.emplace("THREATEN", ActAction("THREATEN", "You make the tension worse.", -30));
}

bool Game::promptPlayerName()
{
    std::cout << "Enter player name: ";
    std::string name;
    std::getline(std::cin, name);
    name = trim(name);

    if (name.empty())
    {
        name = "Traveler";
    }

    m_player.setName(name);
    return true;
}

bool Game::loadItemsFromCsv(const std::string& filePath)
{
    std::ifstream inputFile(filePath);
    if (!inputFile)
    {
        std::cerr << "Error: unable to open required file '" << filePath << "'.\n";
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(inputFile, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        const std::vector<std::string> columns = split(line, ';');
        if (columns.size() != 4)
        {
            std::cerr << "Warning: malformed line ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
            continue;
        }

        try
        {
            const std::string name = columns[0];
            const ItemType type = Item::itemTypeFromString(columns[1]);
            const int value = std::stoi(columns[2]);
            const int quantity = std::stoi(columns[3]);

            if (name.empty() || type == ItemType::UNKNOWN || value < 0 || quantity < 0)
            {
                std::cerr << "Warning: invalid item data ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            m_player.addItem(Item(name, type, value, quantity));
        }
        catch (const std::exception&)
        {
            std::cerr << "Warning: invalid numeric value ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
        }
    }

    return true;
}

bool Game::loadMonstersFromCsv(const std::string& filePath)
{
    std::ifstream inputFile(filePath);
    if (!inputFile)
    {
        std::cerr << "Error: unable to open required file '" << filePath << "'.\n";
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while (std::getline(inputFile, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        const std::vector<std::string> columns = split(line, ';');
        if (columns.size() < 6)
        {
            std::cerr << "Warning: malformed line ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
            continue;
        }

        try
        {
            if (!isValidMonsterCategoryString(columns[0]))
            {
                std::cerr << "Warning: invalid monster category ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            const MonsterCategory category = Monster::categoryFromString(columns[0]);
            const std::string name = columns[1];
            const int hp = std::stoi(columns[2]);
            const int atk = std::stoi(columns[3]);
            const int def = std::stoi(columns[4]);
            const int mercyGoal = std::stoi(columns[5]);

            std::vector<std::string> actIds;
            for (std::size_t i = 6; i < columns.size(); ++i)
            {
                if (columns[i] != "-" && !columns[i].empty())
                {
                    actIds.push_back(columns[i]);
                }
            }

            if (name.empty() || hp <= 0 || atk < 0 || def < 0 || mercyGoal <= 0)
            {
                std::cerr << "Warning: invalid monster data ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            bool unknownActFound = false;
            for (const std::string& actId : actIds)
            {
                if (m_actCatalog.find(actId) == m_actCatalog.end())
                {
                    unknownActFound = true;
                }
            }

            if (unknownActFound)
            {
                std::cerr << "Warning: unknown ACT id ignored with monster '" << name
                          << "' at line " << lineNumber << ".\n";
            }

            std::vector<std::string> filteredActIds;
            for (const std::string& actId : actIds)
            {
                if (m_actCatalog.find(actId) != m_actCatalog.end())
                {
                    filteredActIds.push_back(actId);
                }
            }

            m_monsterCatalog.emplace_back(name, category, hp, atk, def, mercyGoal, filteredActIds);
        }
        catch (const std::exception&)
        {
            std::cerr << "Warning: invalid numeric value ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
        }
    }

    if (m_monsterCatalog.empty())
    {
        std::cerr << "Error: no valid monster data loaded from '" << filePath << "'.\n";
        return false;
    }

    return true;
}

void Game::displayStartupSummary() const
{
    std::cout << "\n=== ALTERDUNE - Startup Summary ===\n";
    std::cout << "Player: " << m_player.getName() << "\n";
    std::cout << "HP: " << m_player.getHp() << "/" << m_player.getMaxHp() << "\n";
    std::cout << "Loaded monsters: " << m_monsterCatalog.size() << "\n";
    m_player.displayItems(std::cout);
    std::cout << "\n";
}

void Game::displayMainMenu() const
{
    std::cout << "\n=== Main Menu ===\n";
    std::cout << "1. Bestiary\n";
    std::cout << "2. Start Battle\n";
    std::cout << "3. Player Stats\n";
    std::cout << "4. Items\n";
    std::cout << "5. Quit\n";
    std::cout << "Choice: ";
}

int Game::readIntChoice(int minValue, int maxValue) const
{
    int choice = 0;

    while (true)
    {
        if (std::cin >> choice && choice >= minValue && choice <= maxValue)
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }

        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid choice (" << minValue << "-" << maxValue << "): ";
    }
}

void Game::handleMenuChoice(int choice)
{
    switch (choice)
    {
    case 1:
        showBestiary();
        break;
    case 2:
        startBattle();
        break;
    case 3:
        showPlayerStats();
        break;
    case 4:
        showItems();
        break;
    default:
        break;
    }
}

void Game::showBestiary() const
{
    std::cout << "\n=== Bestiary ===\n";
    if (m_bestiary.empty())
    {
        std::cout << "No monster has been encountered yet.\n";
        return;
    }

    for (const BestiaryEntry& entry : m_bestiary)
    {
        std::cout << entry.getName()
                  << " | Category: " << Monster::categoryToString(entry.getCategory())
                  << " | HP: " << entry.getMaxHp()
                  << " | ATK: " << entry.getAtk()
                  << " | DEF: " << entry.getDef()
                  << " | Result: " << entry.getResult() << "\n";
    }
}

void Game::showPlayerStats() const
{
    std::cout << "\n";
    m_player.displayStats(std::cout);
}

void Game::showItems()
{
    std::cout << "\n";
    m_player.displayItems(std::cout);

    if (m_player.getInventory().empty())
    {
        return;
    }

    std::cout << "Use an item now? (0 = no, item number = yes): ";
    const int choice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (choice > 0)
    {
        m_player.useItem(static_cast<std::size_t>(choice - 1), std::cout);
    }
}

void Game::startBattle()
{
    Monster monster = createRandomMonster();

    std::cout << "\n=== Battle Start ===\n";
    std::cout << "A wild " << monster.getName() << " appears.\n";

    bool battleEnded = false;
    while (!battleEnded && m_player.isAlive() && monster.isAlive())
    {
        std::cout << "\n";
        m_player.printStatus(std::cout);
        std::cout << "\n";
        monster.printStatus(std::cout);
        std::cout << "\n";

        std::cout << "1. Fight\n";
        std::cout << "2. ACT\n";
        std::cout << "3. Item\n";
        std::cout << "4. Mercy\n";
        std::cout << "5. Run\n";
        std::cout << "Choice: ";

        const int choice = readIntChoice(1, 5);

        if (choice == 1)
        {
            const int damage = randomInt(m_player.getAtk() - 2, m_player.getAtk() + 3);
            monster.takeDamage(damage);
            std::cout << m_player.getName() << " attacks " << monster.getName() << ".\n";

            if (!monster.isAlive())
            {
                std::cout << monster.getName() << " was defeated.\n";
                m_player.recordKill();
                m_player.recordVictory();
                recordBattleResult(monster, "Killed");
                battleEnded = true;
            }
        }
        else if (choice == 2)
        {
            const std::vector<std::string>& actIds = monster.getActIds();
            if (actIds.empty())
            {
                std::cout << "This monster has no ACT options configured yet.\n";
            }
            else
            {
                std::cout << "Available ACT actions:\n";
                for (std::size_t i = 0; i < actIds.size(); ++i)
                {
                    const ActAction& action = m_actCatalog.at(actIds[i]);
                    std::cout << i + 1 << ". " << action.getId()
                              << " (" << action.getMercyImpact() << " mercy) - "
                              << action.getText() << "\n";
                }
                std::cout << "Choose an ACT: ";
                const int actChoice = readIntChoice(1, static_cast<int>(actIds.size()));
                const ActAction& selectedAction = m_actCatalog.at(actIds[actChoice - 1]);
                monster.addMercy(selectedAction.getMercyImpact());
                std::cout << selectedAction.getText() << "\n";
                std::cout << "Mercy is now " << monster.getMercy() << "/" << monster.getMercyGoal() << ".\n";
            }
        }
        else if (choice == 3)
        {
            m_player.displayItems(std::cout);
            if (!m_player.getInventory().empty())
            {
                std::cout << "Choose an item: ";
                const int itemChoice = readIntChoice(1, static_cast<int>(m_player.getInventory().size()));
                m_player.useItem(static_cast<std::size_t>(itemChoice - 1), std::cout);
            }
        }
        else if (choice == 4)
        {
            if (monster.isSpareable())
            {
                std::cout << "You spare " << monster.getName() << ".\n";
                m_player.recordSpare();
                m_player.recordVictory();
                recordBattleResult(monster, "Spared");
                battleEnded = true;
            }
            else
            {
                std::cout << monster.getName() << " is not ready to be spared yet.\n";
            }
        }
        else if (choice == 5)
        {
            std::cout << "You retreat for now. The battle structure remains ready for future expansion.\n";
            battleEnded = true;
        }

        if (!battleEnded && monster.isAlive())
        {
            const int monsterDamage = randomInt(monster.getAtk() - 2, monster.getAtk() + 2);
            m_player.takeDamage(monsterDamage);
            std::cout << monster.getName() << " attacks back.\n";

            if (!m_player.isAlive())
            {
                std::cout << m_player.getName() << " has fallen. Restoring HP for the prototype build.\n";
                m_player.setHp(m_player.getMaxHp());
                battleEnded = true;
            }
        }
    }
}

Monster Game::createRandomMonster()
{
    if (m_monsterCatalog.size() == 1)
    {
        return m_monsterCatalog.front();
    }

    std::uniform_int_distribution<int> distribution(0, static_cast<int>(m_monsterCatalog.size()) - 1);
    return m_monsterCatalog[distribution(m_rng)];
}

int Game::randomInt(int minValue, int maxValue)
{
    if (minValue > maxValue)
    {
        std::swap(minValue, maxValue);
    }

    std::uniform_int_distribution<int> distribution(minValue, maxValue);
    return distribution(m_rng);
}

void Game::recordBattleResult(const Monster& monster, const std::string& result)
{
    m_bestiary.emplace_back(monster.getName(),
                            monster.getCategory(),
                            monster.getMaxHp(),
                            monster.getAtk(),
                            monster.getDef(),
                            result);
}

std::string Game::trim(const std::string& value)
{
    const std::size_t start = value.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return "";
    }

    const std::size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

std::vector<std::string> Game::split(const std::string& line, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream stream(line);
    std::string token;

    while (std::getline(stream, token, delimiter))
    {
        tokens.push_back(trim(token));
    }

    return tokens;
}
