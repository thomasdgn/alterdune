#include "Game.h"

#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

using namespace std;

namespace
{
bool isValidMonsterCategoryString(const string& value)
{
    return value == "NORMAL" || value == "MINIBOSS" || value == "BOSS";
}
}

Game::Game()
    : m_player(),
      m_rng(random_device{}())
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
            cout << "Thanks for playing ALTERDUNE.\n";
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
    cout << "Enter player name: ";
    string name;
    getline(cin, name);
    name = trim(name);

    if (name.empty())
    {
        name = "Traveler";
    }

    m_player.setName(name);
    return true;
}

bool Game::loadItemsFromCsv(const string& filePath)
{
    ifstream inputFile(filePath);
    if (!inputFile)
    {
        cerr << "Error: unable to open required file '" << filePath << "'.\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    while (getline(inputFile, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        const vector<string> columns = split(line, ';');
        if (columns.size() != 4)
        {
            cerr << "Warning: malformed line ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
            continue;
        }

        try
        {
            const string name = columns[0];
            const ItemType type = Item::itemTypeFromString(columns[1]);
            const int value = stoi(columns[2]);
            const int quantity = stoi(columns[3]);

            if (name.empty() || type == ItemType::UNKNOWN || value < 0 || quantity < 0)
            {
                cerr << "Warning: invalid item data ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            m_player.addItem(Item(name, type, value, quantity));
        }
        catch (const exception&)
        {
            cerr << "Warning: invalid numeric value ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
        }
    }

    return true;
}

bool Game::loadMonstersFromCsv(const string& filePath)
{
    ifstream inputFile(filePath);
    if (!inputFile)
    {
        cerr << "Error: unable to open required file '" << filePath << "'.\n";
        return false;
    }

    string line;
    int lineNumber = 0;
    while (getline(inputFile, line))
    {
        ++lineNumber;
        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        const vector<string> columns = split(line, ';');
        if (columns.size() < 6)
        {
            cerr << "Warning: malformed line ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
            continue;
        }

        try
        {
            if (!isValidMonsterCategoryString(columns[0]))
            {
                cerr << "Warning: invalid monster category ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            const MonsterCategory category = Monster::categoryFromString(columns[0]);
            const string name = columns[1];
            const int hp = stoi(columns[2]);
            const int atk = stoi(columns[3]);
            const int def = stoi(columns[4]);
            const int mercyGoal = stoi(columns[5]);

            vector<string> actIds;
            for (size_t i = 6; i < columns.size(); ++i)
            {
                if (columns[i] != "-" && !columns[i].empty())
                {
                    actIds.push_back(columns[i]);
                }
            }

            if (name.empty() || hp <= 0 || atk < 0 || def < 0 || mercyGoal <= 0)
            {
                cerr << "Warning: invalid monster data ignored in " << filePath
                          << " at line " << lineNumber << ".\n";
                continue;
            }

            bool unknownActFound = false;
            for (const string& actId : actIds)
            {
                if (m_actCatalog.find(actId) == m_actCatalog.end())
                {
                    unknownActFound = true;
                }
            }

            if (unknownActFound)
            {
                cerr << "Warning: unknown ACT id ignored with monster '" << name
                          << "' at line " << lineNumber << ".\n";
            }

            vector<string> filteredActIds;
            for (const string& actId : actIds)
            {
                if (m_actCatalog.find(actId) != m_actCatalog.end())
                {
                    filteredActIds.push_back(actId);
                }
            }

            m_monsterCatalog.emplace_back(name, category, hp, atk, def, mercyGoal, filteredActIds);
        }
        catch (const exception&)
        {
            cerr << "Warning: invalid numeric value ignored in " << filePath
                      << " at line " << lineNumber << ".\n";
        }
    }

    if (m_monsterCatalog.empty())
    {
        cerr << "Error: no valid monster data loaded from '" << filePath << "'.\n";
        return false;
    }

    return true;
}

void Game::displayStartupSummary() const
{
    cout << "\n=== ALTERDUNE - Startup Summary ===\n";
    cout << "Player: " << m_player.getName() << "\n";
    cout << "HP: " << m_player.getHp() << "/" << m_player.getMaxHp() << "\n";
    cout << "Loaded monsters: " << m_monsterCatalog.size() << "\n";
    m_player.displayItems(cout);
    cout << "\n";
}

void Game::displayMainMenu() const
{
    cout << "\n=== Main Menu ===\n";
    cout << "1. Bestiary\n";
    cout << "2. Start Battle\n";
    cout << "3. Player Stats\n";
    cout << "4. Items\n";
    cout << "5. Quit\n";
    cout << "Choice: ";
}

int Game::readIntChoice(int minValue, int maxValue) const
{
    int choice = 0;

    while (true)
    {
        if (cin >> choice && choice >= minValue && choice <= maxValue)
        {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return choice;
        }

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Please enter a valid choice (" << minValue << "-" << maxValue << "): ";
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
    cout << "\n=== Bestiary ===\n";
    if (m_bestiary.empty())
    {
        cout << "No monster has been encountered yet.\n";
        return;
    }

    for (const BestiaryEntry& entry : m_bestiary)
    {
        cout << entry.getName()
             << " | Category: " << Monster::categoryToString(entry.getCategory())
             << " | HP: " << entry.getMaxHp()
             << " | ATK: " << entry.getAtk()
             << " | DEF: " << entry.getDef()
             << " | Result: " << entry.getResult() << "\n";
    }
}

void Game::showPlayerStats() const
{
    cout << "\n";
    m_player.displayStats(cout);
}

void Game::showItems()
{
    cout << "\n";
    m_player.displayItems(cout);

    if (m_player.getInventory().empty())
    {
        return;
    }

    cout << "Use an item now? (0 = no, item number = yes): ";
    const int choice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (choice > 0)
    {
        m_player.useItem(static_cast<size_t>(choice - 1), cout);
    }
}

void Game::startBattle()
{
    Monster monster = createRandomMonster();

    cout << "\n=== Battle Start ===\n";
    cout << "A wild " << monster.getName() << " appears.\n";

    bool battleEnded = false;
    while (!battleEnded && m_player.isAlive() && monster.isAlive())
    {
        cout << "\n";
        m_player.printStatus(cout);
        cout << "\n";
        monster.printStatus(cout);
        cout << "\n";

        cout << "1. Fight\n";
        cout << "2. ACT\n";
        cout << "3. Item\n";
        cout << "4. Mercy\n";
        cout << "5. Run\n";
        cout << "Choice: ";

        const int choice = readIntChoice(1, 5);

        if (choice == 1)
        {
            const int damage = randomInt(m_player.getAtk() - 2, m_player.getAtk() + 3);
            monster.takeDamage(damage);
            cout << m_player.getName() << " attacks " << monster.getName() << ".\n";

            if (!monster.isAlive())
            {
                cout << monster.getName() << " was defeated.\n";
                m_player.recordKill();
                m_player.recordVictory();
                recordBattleResult(monster, "Killed");
                battleEnded = true;
            }
        }
        else if (choice == 2)
        {
            const vector<string>& actIds = monster.getActIds();
            if (actIds.empty())
            {
                cout << "This monster has no ACT options configured yet.\n";
            }
            else
            {
                cout << "Available ACT actions:\n";
                for (size_t i = 0; i < actIds.size(); ++i)
                {
                    const ActAction& action = m_actCatalog.at(actIds[i]);
                    cout << i + 1 << ". " << action.getId()
                         << " (" << action.getMercyImpact() << " mercy) - "
                         << action.getText() << "\n";
                }
                cout << "Choose an ACT: ";
                const int actChoice = readIntChoice(1, static_cast<int>(actIds.size()));
                const ActAction& selectedAction = m_actCatalog.at(actIds[actChoice - 1]);
                monster.addMercy(selectedAction.getMercyImpact());
                cout << selectedAction.getText() << "\n";
                cout << "Mercy is now " << monster.getMercy() << "/" << monster.getMercyGoal() << ".\n";
            }
        }
        else if (choice == 3)
        {
            m_player.displayItems(cout);
            if (!m_player.getInventory().empty())
            {
                cout << "Choose an item: ";
                const int itemChoice = readIntChoice(1, static_cast<int>(m_player.getInventory().size()));
                m_player.useItem(static_cast<size_t>(itemChoice - 1), cout);
            }
        }
        else if (choice == 4)
        {
            if (monster.isSpareable())
            {
                cout << "You spare " << monster.getName() << ".\n";
                m_player.recordSpare();
                m_player.recordVictory();
                recordBattleResult(monster, "Spared");
                battleEnded = true;
            }
            else
            {
                cout << monster.getName() << " is not ready to be spared yet.\n";
            }
        }
        else if (choice == 5)
        {
            cout << "You retreat for now. The battle structure remains ready for future expansion.\n";
            battleEnded = true;
        }

        if (!battleEnded && monster.isAlive())
        {
            const int monsterDamage = randomInt(monster.getAtk() - 2, monster.getAtk() + 2);
            m_player.takeDamage(monsterDamage);
            cout << monster.getName() << " attacks back.\n";

            if (!m_player.isAlive())
            {
                cout << m_player.getName() << " has fallen. Restoring HP for the prototype build.\n";
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

    uniform_int_distribution<int> distribution(0, static_cast<int>(m_monsterCatalog.size()) - 1);
    return m_monsterCatalog[distribution(m_rng)];
}

int Game::randomInt(int minValue, int maxValue)
{
    if (minValue > maxValue)
    {
        swap(minValue, maxValue);
    }

    uniform_int_distribution<int> distribution(minValue, maxValue);
    return distribution(m_rng);
}

void Game::recordBattleResult(const Monster& monster, const string& result)
{
    m_bestiary.emplace_back(monster.getName(),
                            monster.getCategory(),
                            monster.getMaxHp(),
                            monster.getAtk(),
                            monster.getDef(),
                            result);
}

string Game::trim(const string& value)
{
    const size_t start = value.find_first_not_of(" \t\r\n");
    if (start == string::npos)
    {
        return "";
    }

    const size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(start, end - start + 1);
}

vector<string> Game::split(const string& line, char delimiter)
{
    vector<string> tokens;
    stringstream stream(line);
    string token;

    while (getline(stream, token, delimiter))
    {
        tokens.push_back(trim(token));
    }

    return tokens;
}
