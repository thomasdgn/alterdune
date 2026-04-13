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

constexpr float kCriticalMultiplier = 1.6f;
}

Game::Game()
    : m_player(),
      m_rng(random_device{}()),
      m_nextFightBonus(0),
      m_nextActBonus(0),
      m_guardCharges(0),
      m_nextFightPenalty(0),
      m_nextActPenalty(0),
      m_battleTurnCounter(0),
      m_hpMilestoneUnlocked(false),
      m_atkMilestoneUnlocked(false),
      m_defMilestoneUnlocked(false),
      m_frontendBattleMonster(),
      m_frontendActUsage(),
      m_frontendBattleLog("A battle preview is ready."),
      m_lastPlayerTurnSummary(),
      m_lastMonsterTurnSummary()
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

bool Game::initializeForFrontend(const string& playerName)
{
    initializeActCatalog();
    m_player = Player();
    m_monsterCatalog.clear();
    m_bestiary.clear();
    m_nextFightBonus = 0;
    m_nextActBonus = 0;
    m_guardCharges = 0;
    m_nextFightPenalty = 0;
    m_nextActPenalty = 0;
    m_battleTurnCounter = 0;
    m_hpMilestoneUnlocked = false;
    m_atkMilestoneUnlocked = false;
    m_defMilestoneUnlocked = false;
    m_frontendBattleMonster.reset();
    m_frontendActUsage.clear();
    m_frontendBattleLog = "A battle preview is ready.";
    m_lastPlayerTurnSummary.clear();
    m_lastMonsterTurnSummary.clear();

    const string trimmedName = trim(playerName);
    m_player.setName(trimmedName.empty() ? "Traveler" : trimmedName);
    m_player.setAppearanceId("wanderer");

    if (!loadItemsFromCsv("data/items.csv"))
    {
        return false;
    }

    if (!loadMonstersFromCsv("data/monsters.csv"))
    {
        return false;
    }

    return true;
}

void Game::setPlayerAppearance(const string& appearanceId)
{
    m_player.setAppearanceId(appearanceId);
}

const string& Game::getPlayerAppearance() const
{
    return m_player.getAppearanceId();
}

vector<string> Game::getAvailableHeroAppearanceIds()
{
    return {"wanderer", "vanguard", "mystic"};
}

string Game::getHeroAppearanceLabel(const string& appearanceId)
{
    if (appearanceId == "wanderer")
    {
        return "Wanderer";
    }
    if (appearanceId == "vanguard")
    {
        return "Vanguard";
    }
    if (appearanceId == "mystic")
    {
        return "Mystic";
    }

    return "Traveler";
}

void Game::run()
{
    bool isRunning = true;

    while (isRunning)
    {
        if (hasReachedEnding())
        {
            displayEndingAndExit();
            break;
        }

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

FrontendMenuViewData Game::buildFrontendMenuViewData() const
{
    FrontendMenuViewData data;
    data.playerName = m_player.getName();
    data.progressText = "Victories: " + to_string(m_player.getVictories()) + "/10  |  Kills: "
        + to_string(m_player.getKills()) + "  |  Spares: " + to_string(m_player.getSpares());

    data.unlockedLandsText = "Unlocked lands: Sunken Mire";
    if (m_player.getVictories() >= 2)
    {
        data.unlockedLandsText += ", Glass Dunes";
    }
    if (m_player.getVictories() >= 4)
    {
        data.unlockedLandsText += ", Signal Wastes";
    }
    if (m_player.getVictories() >= 7)
    {
        data.unlockedLandsText += ", Ancient Vault";
    }

    data.buttons = {
        {"bestiary", "Bestiary", true},
        {"start_battle", "Start Battle", true},
        {"player_stats", "Player Stats", true},
        {"items", "Items", true},
        {"quit", "Quit", true}
    };

    return data;
}

FrontendPlayerStatsViewData Game::buildFrontendPlayerStatsViewData() const
{
    FrontendPlayerStatsViewData data;
    data.playerName = m_player.getName() + " - " + getHeroAppearanceLabel(m_player.getAppearanceId());
    data.appearanceText = getHeroAppearanceBonusText();
    data.hpBar = {"HP", m_player.getHp(), m_player.getMaxHp()};
    data.atk = m_player.getAtk();
    data.def = m_player.getDef();
    data.victories = m_player.getVictories();
    data.kills = m_player.getKills();
    data.spares = m_player.getSpares();
    data.inventorySlots = static_cast<int>(m_player.getInventory().size());
    data.totalHealingStock = 0;

    for (const Item& item : m_player.getInventory())
    {
        data.totalHealingStock += item.getValue() * item.getQuantity();
    }

    data.routeText = "Route: ";
    if (m_player.getVictories() >= 7)
    {
        data.routeText += "Ancient Vault unlocked";
    }
    else if (m_player.getVictories() >= 4)
    {
        data.routeText += "Signal Wastes unlocked";
    }
    else if (m_player.getVictories() >= 2)
    {
        data.routeText += "Glass Dunes unlocked";
    }
    else
    {
        data.routeText += "Sunken Mire scouting";
    }

    data.milestoneText = "Milestones: 3 wins +10 HP max, 6 wins +2 ATK, 9 wins +1 DEF";
    return data;
}

vector<FrontendMonsterCardViewData> Game::buildFrontendMonsterSelectionViewData() const
{
    vector<FrontendMonsterCardViewData> cards;
    for (const unique_ptr<Monster>& monsterPtr : m_monsterCatalog)
    {
        const Monster& monster = *monsterPtr;
        FrontendMonsterCardViewData card;
        card.name = monster.getName();
        card.category = Monster::categoryToString(monster.getCategory());
        card.elementType = getMonsterElementType(monster);
        card.land = getMonsterRegionName(monster);
        card.physique = getMonsterPhysicalProfile(monster);
        card.description = getMonsterDescription(monster);
        card.hp = monster.getMaxHp();
        card.atk = monster.getAtk();
        card.def = monster.getDef();
        card.unlocked = isMonsterUnlocked(monster);

        if (monster.getCategory() == MonsterCategory::NORMAL)
        {
            card.threat = "Low";
        }
        else if (monster.getCategory() == MonsterCategory::MINIBOSS)
        {
            card.threat = "Medium";
        }
        else
        {
            card.threat = "High";
        }

        cards.push_back(card);
    }

    return cards;
}

FrontendBattleViewData Game::buildFrontendBattlePreviewViewData(size_t displayIndex) const
{
    const vector<size_t> availableIndices = getAvailableMonsterIndices();

    if (availableIndices.empty())
    {
        FrontendBattleViewData data;
        data.regionName = "Unknown Frontier";
        data.openingText = "No battle data available.";
        data.hintText = "Load monsters first.";
        data.playerName = m_player.getName();
        data.monsterName = "Unknown";
        data.monsterCategory = "NONE";
        data.playerHpBar = {"HP", m_player.getHp(), m_player.getMaxHp()};
        data.monsterHpBar = {"HP", 0, 1};
        data.mercyBar = {"MERCY", 0, 100};
        return data;
    }

    if (displayIndex >= availableIndices.size())
    {
        displayIndex = 0;
    }

    const Monster& monster = *m_monsterCatalog[availableIndices[displayIndex]];
    return buildFrontendBattleViewData(monster, getMonsterDescription(monster));
}

vector<FrontendBestiaryEntryViewData> Game::buildFrontendBestiaryViewData() const
{
    vector<FrontendBestiaryEntryViewData> entries;
    for (const BestiaryEntry& entry : m_bestiary)
    {
        FrontendBestiaryEntryViewData viewEntry;
        viewEntry.name = entry.getName();
        viewEntry.category = Monster::categoryToString(entry.getCategory());
        viewEntry.elementType = "Neutral";
        viewEntry.physique = "Unknown physique";
        for (const unique_ptr<Monster>& monsterPtr : m_monsterCatalog)
        {
            if (monsterPtr->getName() == entry.getName())
            {
                viewEntry.elementType = getMonsterElementType(*monsterPtr);
                viewEntry.physique = getMonsterPhysicalProfile(*monsterPtr);
                break;
            }
        }
        viewEntry.result = entry.getResult();
        viewEntry.description = entry.getDescription();
        viewEntry.hp = entry.getMaxHp();
        viewEntry.atk = entry.getAtk();
        viewEntry.def = entry.getDef();

        const string name = entry.getName();
        if (name == "Froggit" || name == "Dustling")
        {
            viewEntry.land = "Sunken Mire";
        }
        else if (name == "MimicBox" || name == "GlimmerMoth" || name == "Pebblimp")
        {
            viewEntry.land = "Glass Dunes";
        }
        else if (name == "HowlScreen" || name == "BloomCobra" || name == "QueenByte")
        {
            viewEntry.land = "Signal Wastes";
        }
        else if (name == "Archivore")
        {
            viewEntry.land = "Ancient Vault";
        }
        else
        {
            viewEntry.land = "Unknown Frontier";
        }

        entries.push_back(viewEntry);
    }

    return entries;
}

vector<FrontendInventoryItemViewData> Game::buildFrontendInventoryViewData() const
{
    vector<FrontendInventoryItemViewData> items;
    for (const Item& item : m_player.getInventory())
    {
        FrontendInventoryItemViewData viewItem;
        viewItem.name = item.getName();
        viewItem.type = Item::itemTypeToString(item.getType());
        viewItem.tacticalEffect = getItemTacticalEffect(item.getName());
        viewItem.value = item.getValue();
        viewItem.quantity = item.getQuantity();
        items.push_back(viewItem);
    }

    return items;
}

bool Game::startFrontendBattle(size_t displayIndex)
{
    const vector<size_t> availableIndices = getAvailableMonsterIndices();
    if (availableIndices.empty())
    {
        return false;
    }

    if (displayIndex >= availableIndices.size())
    {
        displayIndex = 0;
    }

    resetBattleModifiers();
    m_frontendActUsage.clear();
    m_frontendBattleMonster = m_monsterCatalog[availableIndices[displayIndex]]->clone();
    m_frontendBattleLog = getBattleOpeningText(*m_frontendBattleMonster);
    m_lastPlayerTurnSummary.clear();
    m_lastMonsterTurnSummary.clear();
    return true;
}

FrontendBattleViewData Game::buildActiveFrontendBattleViewData() const
{
    if (!m_frontendBattleMonster)
    {
        return buildFrontendBattlePreviewViewData(0);
    }

    return buildFrontendBattleViewData(*m_frontendBattleMonster, m_frontendBattleLog);
}

string Game::performFrontendBattleAction(const string& actionId)
{
    if (!m_frontendBattleMonster)
    {
        return "No frontend battle is active.";
    }

    Monster& monster = *m_frontendBattleMonster;
    string logText;

    if (actionId == "fight")
    {
        const vector<string> attackIds = getAvailableAttackStyleIds();
        if (attackIds.empty())
        {
            m_frontendBattleLog = "No fight style is available.";
            return m_frontendBattleLog;
        }

        bool criticalHit = false;
        string effectivenessText;
        string attackLabel;
        const int dealtDamage = resolvePlayerFightAction(monster, attackIds.front(), criticalHit, effectivenessText, attackLabel);
        logText = m_player.getName() + " uses " + attackLabel + " and deals " + to_string(dealtDamage) + " damage.";
        if (criticalHit)
        {
            logText += " Critical hit!";
        }
        if (!effectivenessText.empty())
        {
            logText += " " + effectivenessText;
        }

        if (!monster.isAlive())
        {
            m_lastPlayerTurnSummary = logText + " " + monster.getName() + " is defeated.";
            m_frontendBattleLog = m_lastPlayerTurnSummary;
            concludeBattle(monster, BattleTurnResult::PLAYER_WON_KILL);
            m_frontendBattleMonster.reset();
            return m_frontendBattleLog;
        }
    }
    else if (actionId == "act")
    {
        const ActAction* chosenAction = chooseBestFrontendAct(monster);
        if (!chosenAction)
        {
            m_frontendBattleLog = "This monster has no ACT option available.";
            return m_frontendBattleLog;
        }

        const int previousMercy = monster.getMercy();
        monster.addMercy(computeActMercyImpact(monster, *chosenAction, m_frontendActUsage));
        ++m_frontendActUsage[chosenAction->getId()];
        const int delta = monster.getMercy() - previousMercy;
        logText = chosenAction->getText() + " " + getActReactionText(monster, *chosenAction, delta);
        if (m_nextActBonus > 0)
        {
            logText += " The prepared ACT boost is consumed.";
            m_nextActBonus = 0;
        }
    }
    else if (actionId == "item")
    {
        const Item* item = findFirstUsableInventoryItem();
        if (!item)
        {
            m_frontendBattleLog = "No usable item is available.";
            return m_frontendBattleLog;
        }

        const string itemName = item->getName();
        for (size_t i = 0; i < m_player.getInventory().size(); ++i)
        {
            if (m_player.getInventory()[i].getName() == itemName && m_player.getInventory()[i].getQuantity() > 0)
            {
                m_player.useItem(i, cout);
                applyItemBattleEffect(*item);
                break;
            }
        }

        m_lastPlayerTurnSummary = itemName + " used. " + getItemTacticalEffect(itemName) + ".";
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        return m_frontendBattleLog;
    }
    else if (actionId == "mercy")
    {
        if (monster.isSpareable())
        {
            m_lastPlayerTurnSummary = "You spare " + monster.getName() + ".";
            m_frontendBattleLog = m_lastPlayerTurnSummary;
            concludeBattle(monster, BattleTurnResult::PLAYER_WON_SPARE);
            m_frontendBattleMonster.reset();
            return m_frontendBattleLog;
        }

        m_lastPlayerTurnSummary = monster.getName() + " is not ready to be spared.";
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        return m_frontendBattleLog;
    }
    else
    {
        m_lastPlayerTurnSummary = "Unknown action.";
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        return m_frontendBattleLog;
    }

    m_lastPlayerTurnSummary = logText;

    if (m_frontendBattleMonster)
    {
        const BattleTurnResult monsterTurnResult = handleMonsterTurn(*m_frontendBattleMonster);
        if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
        {
            concludeBattle(*m_frontendBattleMonster, monsterTurnResult);
            m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " You were overwhelmed, but the prototype restores you.";
            m_frontendBattleMonster.reset();
            return m_frontendBattleLog;
        }

        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary;
    }

    return m_frontendBattleLog;
}

vector<FrontendActionButtonViewData> Game::buildFrontendFightOptionViewData() const
{
    vector<FrontendActionButtonViewData> options;
    for (const string& attackStyleId : getAvailableAttackStyleIds())
    {
        FrontendActionButtonViewData button;
        button.id = attackStyleId;
        button.label = getAttackStyleLabel(attackStyleId) + " - " + getAttackStyleDescription(attackStyleId);
        button.enabled = true;
        options.push_back(button);
    }
    return options;
}

vector<FrontendActionButtonViewData> Game::buildFrontendActOptionViewData() const
{
    vector<FrontendActionButtonViewData> options;
    if (!m_frontendBattleMonster)
    {
        return options;
    }

    const vector<string> actIds = m_frontendBattleMonster->getAvailableActIds();
    for (size_t i = 0; i < actIds.size(); ++i)
    {
        const auto actionIt = m_actCatalog.find(actIds[i]);
        if (actionIt == m_actCatalog.end())
        {
            continue;
        }

        FrontendActionButtonViewData button;
        button.id = actionIt->second.getId();
        button.label = actionIt->second.getId() + " (" + to_string(actionIt->second.getMercyImpact()) + ")";
        button.enabled = true;
        options.push_back(button);
    }

    return options;
}

vector<FrontendActionButtonViewData> Game::buildFrontendItemOptionViewData() const
{
    vector<FrontendActionButtonViewData> options;
    for (const Item& item : m_player.getInventory())
    {
        FrontendActionButtonViewData button;
        button.id = item.getName();
        button.label = item.getName() + " x" + to_string(item.getQuantity());
        button.enabled = item.getQuantity() > 0;
        options.push_back(button);
    }

    return options;
}

string Game::performFrontendFightByIndex(size_t optionIndex)
{
    if (!m_frontendBattleMonster)
    {
        return "No frontend battle is active.";
    }

    const vector<string> attackIds = getAvailableAttackStyleIds();
    if (optionIndex >= attackIds.size())
    {
        return "Invalid fight style selection.";
    }

    Monster& monster = *m_frontendBattleMonster;
    bool criticalHit = false;
    string effectivenessText;
    string attackLabel;
    const int dealtDamage = resolvePlayerFightAction(monster, attackIds[optionIndex], criticalHit, effectivenessText, attackLabel);

    string logText = m_player.getName() + " uses " + attackLabel + " and deals " + to_string(dealtDamage) + " damage.";
    if (criticalHit)
    {
        logText += " Critical hit!";
    }
    if (!effectivenessText.empty())
    {
        logText += " " + effectivenessText;
    }

    if (!monster.isAlive())
    {
        m_lastPlayerTurnSummary = logText + " " + monster.getName() + " is defeated.";
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        concludeBattle(monster, BattleTurnResult::PLAYER_WON_KILL);
        m_frontendBattleMonster.reset();
        return m_frontendBattleLog;
    }

    m_lastPlayerTurnSummary = logText;
    const BattleTurnResult monsterTurnResult = handleMonsterTurn(monster);
    if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
    {
        concludeBattle(monster, monsterTurnResult);
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " You were overwhelmed, but the prototype restores you.";
        m_frontendBattleMonster.reset();
        return m_frontendBattleLog;
    }

    m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary;
    return m_frontendBattleLog;
}

string Game::performFrontendActByIndex(size_t optionIndex)
{
    if (!m_frontendBattleMonster)
    {
        return "No frontend battle is active.";
    }

    const vector<string> actIds = m_frontendBattleMonster->getAvailableActIds();
    if (optionIndex >= actIds.size())
    {
        return "Invalid ACT selection.";
    }

    const auto actionIt = m_actCatalog.find(actIds[optionIndex]);
    if (actionIt == m_actCatalog.end())
    {
        return "Unknown ACT.";
    }

    Monster& monster = *m_frontendBattleMonster;
    const ActAction& chosenAction = actionIt->second;
    const int previousMercy = monster.getMercy();
    monster.addMercy(computeActMercyImpact(monster, chosenAction, m_frontendActUsage));
    ++m_frontendActUsage[chosenAction.getId()];
    const int delta = monster.getMercy() - previousMercy;

    string logText = chosenAction.getText() + " " + getActReactionText(monster, chosenAction, delta);
    if (m_nextActBonus > 0)
    {
        logText += " The prepared ACT boost is consumed.";
        m_nextActBonus = 0;
    }

    m_lastPlayerTurnSummary = logText;
    const BattleTurnResult monsterTurnResult = handleMonsterTurn(monster);
    if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
    {
        concludeBattle(monster, monsterTurnResult);
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " You were overwhelmed, but the prototype restores you.";
        m_frontendBattleMonster.reset();
        return m_frontendBattleLog;
    }

    m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary;
    return m_frontendBattleLog;
}

string Game::performFrontendItemByIndex(size_t optionIndex)
{
    if (!m_frontendBattleMonster)
    {
        return "No frontend battle is active.";
    }

    if (optionIndex >= m_player.getInventory().size())
    {
        return "Invalid item selection.";
    }

    const Item& selectedItem = m_player.getInventory()[optionIndex];
    if (selectedItem.getQuantity() <= 0)
    {
        return selectedItem.getName() + " is out of stock.";
    }

    const string itemName = selectedItem.getName();
    if (m_player.useItem(optionIndex, cout))
    {
        applyItemBattleEffect(selectedItem);
    }

    m_lastPlayerTurnSummary = itemName + " used. " + getItemTacticalEffect(itemName) + ".";
    m_frontendBattleLog = m_lastPlayerTurnSummary;

    Monster& monster = *m_frontendBattleMonster;
    const BattleTurnResult monsterTurnResult = handleMonsterTurn(monster);
    if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
    {
        concludeBattle(monster, monsterTurnResult);
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " You were overwhelmed, but the prototype restores you.";
        m_frontendBattleMonster.reset();
        return m_frontendBattleLog;
    }

    m_frontendBattleLog += " " + m_lastMonsterTurnSummary;
    return m_frontendBattleLog;
}

bool Game::hasActiveFrontendBattle() const
{
    return m_frontendBattleMonster != nullptr;
}

void Game::leaveFrontendBattle()
{
    m_frontendBattleMonster.reset();
    m_frontendActUsage.clear();
    resetBattleModifiers();
    m_frontendBattleLog = "You step back from the battle preview.";
    m_lastPlayerTurnSummary.clear();
    m_lastMonsterTurnSummary.clear();
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
    promptPlayerAppearance();
    return true;
}

void Game::promptPlayerAppearance()
{
    const vector<string> appearanceIds = getAvailableHeroAppearanceIds();
    cout << "\nChoose your hero style:\n";
    for (size_t i = 0; i < appearanceIds.size(); ++i)
    {
        cout << i + 1 << ". " << getHeroAppearanceLabel(appearanceIds[i]) << "\n";
    }
    cout << "Choice: ";
    const int choice = readIntChoice(1, static_cast<int>(appearanceIds.size()));
    m_player.setAppearanceId(appearanceIds[static_cast<size_t>(choice - 1)]);
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

            switch (category)
            {
            case MonsterCategory::NORMAL:
                m_monsterCatalog.push_back(make_unique<NormalMonster>(name, hp, atk, def, mercyGoal, filteredActIds));
                break;
            case MonsterCategory::MINIBOSS:
                m_monsterCatalog.push_back(make_unique<MiniBossMonster>(name, hp, atk, def, mercyGoal, filteredActIds));
                break;
            case MonsterCategory::BOSS:
                m_monsterCatalog.push_back(make_unique<BossMonster>(name, hp, atk, def, mercyGoal, filteredActIds));
                break;
            }
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
    cout << "Progress: " << m_player.getVictories() << "/10 victories"
         << " | Kills: " << m_player.getKills()
         << " | Spares: " << m_player.getSpares() << "\n";
    cout << "Milestones: 3 wins (+10 HP), 6 wins (+2 ATK), 9 wins (+1 DEF)\n";
    cout << "Unlocked lands: Sunken Mire";
    if (m_player.getVictories() >= 2)
    {
        cout << ", Glass Dunes";
    }
    if (m_player.getVictories() >= 4)
    {
        cout << ", Signal Wastes";
    }
    if (m_player.getVictories() >= 7)
    {
        cout << ", Ancient Vault";
    }
    cout << "\n";
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

    cout << "Entries unlocked: " << m_bestiary.size() << "\n";

    for (const BestiaryEntry& entry : m_bestiary)
    {
        cout << entry.getName()
             << " | Category: " << Monster::categoryToString(entry.getCategory())
             << " | HP: " << entry.getMaxHp()
             << " | ATK: " << entry.getAtk()
             << " | DEF: " << entry.getDef()
             << " | Result: " << entry.getResult() << "\n";
        cout << "   Notes: " << entry.getDescription() << "\n";
    }
}

void Game::showPlayerStats() const
{
    cout << "\n";
    m_player.displayStats(cout);
    int totalHealingValue = 0;
    int totalConsumables = 0;
    for (const Item& item : m_player.getInventory())
    {
        totalHealingValue += item.getValue() * item.getQuantity();
        totalConsumables += item.getQuantity();
    }
    cout << "Healing reserves: " << totalHealingValue << " total HP across "
         << totalConsumables << " consumables.\n";
    cout << "Victories needed for an ending: " << max(0, 10 - m_player.getVictories()) << "\n";
}

void Game::showItems()
{
    cout << "\n";
    m_player.displayItems(cout);

    if (m_player.getInventory().empty())
    {
        return;
    }

    cout << "Tactical effects in battle:\n";
    for (const Item& item : m_player.getInventory())
    {
        cout << "- " << item.getName() << ": " << getItemTacticalEffect(item.getName()) << "\n";
    }

    cout << "Use an item now? (0 = no, item number = yes): ";
    const int choice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (choice > 0)
    {
        m_player.useItem(static_cast<size_t>(choice - 1), cout);
    }
}

int Game::chooseMonsterIndex() const
{
    const vector<size_t> availableIndices = getAvailableMonsterIndices();

    cout << "\n=== Choose Opponent ===\n";
    cout << "0. Random monster\n";

    for (size_t i = 0; i < availableIndices.size(); ++i)
    {
        const Monster& monster = *m_monsterCatalog[availableIndices[i]];
        cout << i + 1 << ". " << monster.getName()
             << " [" << Monster::categoryToString(monster.getCategory()) << "]"
             << " | HP: " << monster.getMaxHp()
             << " | ATK: " << monster.getAtk()
             << " | DEF: " << monster.getDef()
             << " | Land: " << getMonsterRegionName(monster);

        if (monster.getCategory() == MonsterCategory::NORMAL)
        {
            cout << " | Threat: Low";
        }
        else if (monster.getCategory() == MonsterCategory::MINIBOSS)
        {
            cout << " | Threat: Medium";
        }
        else
        {
            cout << " | Threat: High";
        }

        cout << "\n";
    }

    const int lockedMonsters = static_cast<int>(m_monsterCatalog.size() - availableIndices.size());
    if (lockedMonsters > 0)
    {
        cout << lockedMonsters << " opponents remain hidden in farther lands.\n";
    }

    cout << "Choice: ";
    const int displayedChoice = readIntChoice(0, static_cast<int>(availableIndices.size()));
    if (displayedChoice == 0)
    {
        return 0;
    }

    return static_cast<int>(availableIndices[displayedChoice - 1]) + 1;
}

vector<size_t> Game::getAvailableMonsterIndices() const
{
    vector<size_t> availableIndices;
    for (size_t i = 0; i < m_monsterCatalog.size(); ++i)
    {
        if (isMonsterUnlocked(*m_monsterCatalog[i]))
        {
            availableIndices.push_back(i);
        }
    }

    return availableIndices;
}

bool Game::isMonsterUnlocked(const Monster& monster) const
{
    const int victories = m_player.getVictories();
    const string& name = monster.getName();

    if (name == "MimicBox" || name == "GlimmerMoth" || name == "Pebblimp")
    {
        return victories >= 2;
    }
    if (name == "HowlScreen" || name == "BloomCobra" || name == "QueenByte")
    {
        return victories >= 4;
    }
    if (name == "Archivore")
    {
        return victories >= 7;
    }

    return true;
}

void Game::displayBattleState(const Monster& monster) const
{
    cout << "\n";
    m_player.printStatus(cout);
    cout << "\n";
    monster.printStatus(cout);
    cout << "\nMercy: " << buildMercyBar(monster)
         << " " << monster.getMercy() << "/" << monster.getMercyGoal() << "\n";
    cout << getMercyComment(monster) << "\n";
    cout << "Mood: " << getMonsterMoodText(monster) << "\n";
    cout << "Hint: " << getBattleHint(monster) << "\n";
    cout << "Physique: " << getMonsterPhysicalProfile(monster) << "\n";
    cout << "Hero style: " << getHeroAppearanceLabel(m_player.getAppearanceId())
         << " | " << getHeroAppearanceBonusText() << "\n";

    if (m_nextFightBonus > 0 || m_nextActBonus > 0 || m_guardCharges > 0 || m_nextFightPenalty > 0 || m_nextActPenalty > 0)
    {
        cout << "Active bonuses:";
        if (m_nextFightBonus > 0)
        {
            cout << " Fight +" << m_nextFightBonus;
        }
        if (m_nextActBonus > 0)
        {
            cout << " ACT +" << m_nextActBonus;
        }
        if (m_guardCharges > 0)
        {
            cout << " Guard x" << m_guardCharges;
        }
        if (m_nextFightPenalty > 0)
        {
            cout << " Fight -" << m_nextFightPenalty;
        }
        if (m_nextActPenalty > 0)
        {
            cout << " ACT -" << m_nextActPenalty;
        }
        cout << "\n";
    }
}

Game::BattleTurnResult Game::handleFightAction(Monster& monster)
{
    const vector<string> attackIds = getAvailableAttackStyleIds();
    cout << "Choose a fight style:\n";
    for (size_t i = 0; i < attackIds.size(); ++i)
    {
        cout << i + 1 << ". " << getAttackStyleLabel(attackIds[i])
             << " - " << getAttackStyleDescription(attackIds[i]) << "\n";
    }
    cout << "0. Cancel\n";
    cout << "Choice: ";
    const int attackChoice = readIntChoice(0, static_cast<int>(attackIds.size()));
    if (attackChoice == 0)
    {
        cout << "You hold your stance and wait.\n";
        return BattleTurnResult::NO_TURN_SPENT;
    }

    bool criticalHit = false;
    string effectivenessText;
    string attackLabel;
    const int dealtDamage = resolvePlayerFightAction(monster, attackIds[attackChoice - 1], criticalHit, effectivenessText, attackLabel);
    cout << m_player.getName() << " uses " << attackLabel
         << " and deals " << dealtDamage << " damage.\n";
    if (criticalHit)
    {
        cout << "Critical hit!\n";
    }
    if (!effectivenessText.empty())
    {
        cout << effectivenessText << "\n";
    }

    if (!monster.isAlive())
    {
        return BattleTurnResult::PLAYER_WON_KILL;
    }

    return BattleTurnResult::CONTINUE;
}

Game::BattleTurnResult Game::handleActAction(Monster& monster, map<string, int>& actUsage)
{
    const vector<string> actIds = monster.getAvailableActIds();
    if (actIds.empty())
    {
        cout << "This monster has no ACT options configured yet.\n";
        return BattleTurnResult::NO_TURN_SPENT;
    }

    cout << "Available ACT actions:\n";
    for (size_t i = 0; i < actIds.size(); ++i)
    {
        const ActAction& action = m_actCatalog.at(actIds[i]);
        cout << i + 1 << ". " << action.getId()
             << " (" << action.getMercyImpact() << " mercy) - "
             << action.getText() << "\n";
    }
    cout << "0. Cancel\n";
    cout << "Choose an ACT: ";
    const int actChoice = readIntChoice(0, static_cast<int>(actIds.size()));
    if (actChoice == 0)
    {
        cout << "You keep thinking.\n";
        return BattleTurnResult::NO_TURN_SPENT;
    }

    const ActAction& selectedAction = m_actCatalog.at(actIds[actChoice - 1]);
    const int previousMercy = monster.getMercy();
    monster.addMercy(computeActMercyImpact(monster, selectedAction, actUsage));
    ++actUsage[selectedAction.getId()];
    cout << selectedAction.getText() << "\n";
    displayActOutcome(monster, selectedAction, previousMercy);
    if (m_nextActBonus > 0)
    {
        cout << "Your prepared social item boost has been consumed.\n";
        m_nextActBonus = 0;
    }
    return BattleTurnResult::CONTINUE;
}

Game::BattleTurnResult Game::handleItemAction(Monster& monster)
{
    (void)monster;
    m_player.displayItems(cout);
    if (m_player.getInventory().empty())
    {
        return BattleTurnResult::NO_TURN_SPENT;
    }

    cout << "0. Cancel\n";
    cout << "Choose an item: ";
    const int itemChoice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (itemChoice == 0)
    {
        cout << "You put the item back in your bag.\n";
        return BattleTurnResult::NO_TURN_SPENT;
    }

    const Item& item = m_player.getInventory()[static_cast<size_t>(itemChoice - 1)];
    const string itemName = item.getName();
    if (m_player.useItem(static_cast<size_t>(itemChoice - 1), cout))
    {
        applyItemBattleEffect(item);
        cout << "Tactical effect: " << getItemTacticalEffect(itemName) << "\n";
    }
    return BattleTurnResult::CONTINUE;
}

Game::BattleTurnResult Game::handleMercyAction(Monster& monster)
{
    if (monster.isSpareable())
    {
        return BattleTurnResult::PLAYER_WON_SPARE;
    }

    cout << monster.getName() << " is not ready to be spared yet.\n";
    return BattleTurnResult::NO_TURN_SPENT;
}

Game::BattleTurnResult Game::handleMonsterTurn(Monster& monster)
{
    ++m_battleTurnCounter;
    m_lastMonsterTurnSummary = monster.getName();
    int totalDamage = 0;
    const string attackText = getMonsterAttackText(monster);
    const string attackType = getMonsterAttackType(monster);
    const float attackMultiplier = getMonsterAttackMultiplier(monster);
    const int mercySoftening = monster.getMercy() >= monster.getMercyGoal() / 2 ? 2 : 0;
    const int guardReduction = m_guardCharges > 0 ? 4 : 0;
    const bool criticalHit = randomInt(1, 100) <= (monster.getCategory() == MonsterCategory::BOSS ? 18 : (monster.getCategory() == MonsterCategory::MINIBOSS ? 12 : 8));

    if (monster.getName() == "QueenByte")
    {
        const bool pulseSurgeTurn = (m_battleTurnCounter % 3) == 0;
        const bool commandBreakTurn = (m_battleTurnCounter % 2) == 0;
        const float firstPhaseMultiplier = pulseSurgeTurn ? 1.15f : 1.f;
        const float secondPhaseMultiplier = commandBreakTurn ? 1.1f : 1.f;
        const int firstRaw = static_cast<int>((randomInt(monster.getAtk() - 4, monster.getAtk() + 1) * attackMultiplier * firstPhaseMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
        const int secondRaw = static_cast<int>((randomInt(monster.getAtk() - 7, monster.getAtk() - 2) * attackMultiplier * secondPhaseMultiplier) * (criticalHit ? 1.25f : 1.f)) - mercySoftening - guardReduction;
        const int firstStrike = m_player.takeDamage(firstRaw);
        const int secondStrike = m_player.takeDamage(secondRaw);
        totalDamage = firstStrike + secondStrike;
        cout << attackText << " A royal " << attackType << " combo crashes down for " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " unleashes a royal combo for " + to_string(totalDamage) + " damage.";
        if (pulseSurgeTurn)
        {
            cout << "QueenByte enters a pulse-surge phase. The arena vibrates with hostile code.\n";
            monster.addMercy(-6);
            m_lastMonsterTurnSummary += " Pulse surge destabilizes mercy.";
        }
        if (criticalHit)
        {
            cout << "QueenByte lands a critical sequence.\n";
        }
        if (commandBreakTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 3);
            cout << "QueenByte issues a command break. Your next FIGHT loses momentum.\n";
            m_lastMonsterTurnSummary += " Your next FIGHT is hindered.";
        }
        if (m_nextActBonus > 0)
        {
            m_nextActBonus = max(0, m_nextActBonus - 4);
            cout << "QueenByte disrupts your rhythm and weakens your prepared ACT bonus.\n";
        }
    }
    else if (monster.getName() == "Archivore")
    {
        const bool suppressTurn = (m_battleTurnCounter % 2) == 1;
        const bool judgmentTurn = (m_battleTurnCounter % 4) == 0;
        const float openingMultiplier = judgmentTurn ? 1.2f : 1.f;
        const int firstRaw = static_cast<int>((randomInt(monster.getAtk() - 6, monster.getAtk() - 1) * attackMultiplier * openingMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
        const int firstStrike = m_player.takeDamage(firstRaw);
        totalDamage = firstStrike;
        cout << attackText << " A " << attackType << " volley deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " launches a vault volley for " + to_string(totalDamage) + " damage.";
        if (criticalHit)
        {
            cout << "Archivore finds a critical opening.\n";
        }

        if (suppressTurn)
        {
            m_nextActPenalty = max(m_nextActPenalty, 5);
            cout << "Archivore suppresses your composure. Your next ACT will be harder to land.\n";
            m_lastMonsterTurnSummary += " Your next ACT is suppressed.";
        }

        if (monster.getMercy() < monster.getMercyGoal() / 2)
        {
            monster.addMercy(-8);
            cout << "Archivore rewrites the mood of the battle. Mercy falls slightly.\n";
            m_lastMonsterTurnSummary += " Mercy slips backward.";
        }
        else
        {
            const int secondRaw = static_cast<int>((randomInt(monster.getAtk() - 8, monster.getAtk() - 3) * attackMultiplier)) - mercySoftening - guardReduction;
            const int secondStrike = m_player.takeDamage(secondRaw);
            totalDamage += secondStrike;
            cout << "Ancient pages return for a second pass. Total damage: " << totalDamage << ".\n";
            m_lastMonsterTurnSummary += " A second pass raises the total to " + to_string(totalDamage) + ".";
        }

        if (judgmentTurn)
        {
            monster.heal(4);
            cout << "Archivore enters a judgment phase and restores some of its structure.\n";
            m_lastMonsterTurnSummary += " Judgment phase restores HP.";
        }
    }
    else if (monster.getName() == "Froggit")
    {
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        m_nextFightPenalty = max(m_nextFightPenalty, 2);
        cout << "Mud clings to your weapon. Your next FIGHT will be slightly weaker.\n";
    }
    else if (monster.getName() == "MimicBox")
    {
        if (randomInt(1, 100) <= 35)
        {
            monster.heal(6);
            m_nextFightPenalty = max(m_nextFightPenalty, 2);
            cout << "MimicBox slams shut, reinforces its shell, and recovers HP.\n";
            m_lastMonsterTurnSummary += " fortifies itself and restores HP.";
        }
        else
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
            monster.heal(4);
            cout << "MimicBox snaps shut and reinforces itself, recovering a little HP.\n";
        }
    }
    else if (monster.getName() == "Dustling")
    {
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        m_nextActPenalty = max(m_nextActPenalty, 4);
        cout << "The storm rattles your focus. Your next ACT will be less effective.\n";
    }
    else if (monster.getName() == "GlimmerMoth")
    {
        const bool hazeTurn = randomInt(1, 100) <= 30;
        if (!hazeTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? 1.35f : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        if (monster.getMercy() < monster.getMercyGoal() / 2)
        {
            monster.addMercy(-5);
            cout << "The glimmering haze unsettles the mood. Mercy falls slightly.\n";
            m_lastMonsterTurnSummary += hazeTurn ? " releases a calming haze that lowers mercy." : " A glimmering haze lowers mercy.";
        }
    }
    else if (monster.getName() == "Pebblimp")
    {
        const bool reboundTurn = randomInt(1, 100) <= 28;
        if (!reboundTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        if (randomInt(1, 100) <= 35 || reboundTurn)
        {
            monster.heal(reboundTurn ? 6 : 3);
            cout << "Pebblimp ricochets away and regains its footing, recovering a little HP.\n";
            m_lastMonsterTurnSummary += reboundTurn ? " rebounds and restores HP." : " It recovers a little HP.";
        }
    }
    else if (monster.getName() == "HowlScreen")
    {
        const bool shriekTurn = randomInt(1, 100) <= 32;
        if (!shriekTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        m_nextActPenalty = max(m_nextActPenalty, shriekTurn ? 7 : 6);
        cout << "The static shriek scrambles your thoughts. Your next ACT will be much weaker.\n";
        m_lastMonsterTurnSummary += shriekTurn ? " It unleashes a pure static shriek that cripples your next ACT." : " Static interference weakens your next ACT.";
    }
    else if (monster.getName() == "BloomCobra")
    {
        const bool rootTurn = randomInt(1, 100) <= 30;
        if (!rootTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        m_nextFightPenalty = max(m_nextFightPenalty, rootTurn ? 4 : 3);
        cout << "Thorns catch your movement. Your next FIGHT loses momentum.\n";
        m_lastMonsterTurnSummary += rootTurn ? " Roots surge from the floor and heavily slow your next FIGHT." : " Thorns slow your next FIGHT.";
    }
    else
    {
        switch (monster.getCategory())
        {
        case MonsterCategory::NORMAL:
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals "
                 << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
            if (criticalHit)
            {
                cout << monster.getName() << " scores a critical hit.\n";
            }
            break;
        }
        case MonsterCategory::MINIBOSS:
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals "
                 << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
            if (criticalHit)
            {
                cout << monster.getName() << " scores a critical hit.\n";
            }
            break;
        }
        case MonsterCategory::BOSS:
        {
            const int firstStrike = m_player.takeDamage(static_cast<int>((randomInt(monster.getAtk() - 6, monster.getAtk() - 1) * attackMultiplier) * (criticalHit ? 1.35f : 1.f)) - mercySoftening - guardReduction);
            const int secondStrike = m_player.takeDamage(static_cast<int>((randomInt(monster.getAtk() - 6, monster.getAtk() - 1) * attackMultiplier) * (criticalHit ? 1.15f : 1.f)) - mercySoftening - guardReduction);
            totalDamage = firstStrike + secondStrike;
            cout << attackText << " Its " << attackType << " barrage deals total damage: "
                 << totalDamage << ".\n";
            m_lastMonsterTurnSummary += " unleashes a barrage for " + to_string(totalDamage) + " damage.";
            if (criticalHit)
            {
                cout << monster.getName() << " scores a critical sequence.\n";
            }
            break;
        }
        }
    }

    if (m_guardCharges > 0)
    {
        cout << "Your guard softens the incoming blow.\n";
        m_lastMonsterTurnSummary += " Guard softens part of the impact.";
        --m_guardCharges;
    }

    if (!m_player.isAlive())
    {
        return BattleTurnResult::PLAYER_LOST;
    }

    return BattleTurnResult::CONTINUE;
}

void Game::concludeBattle(const Monster& monster, BattleTurnResult result)
{
    switch (result)
    {
    case BattleTurnResult::PLAYER_WON_KILL:
        cout << monster.getName() << " was defeated.\n";
        m_player.recordKill();
        m_player.recordVictory();
        recordBattleResult(monster, "Killed");
        grantBattleReward(monster, result);
        checkProgressMilestones();
        cout << "Progress toward ending: " << m_player.getVictories() << "/10 victories.\n";
        break;
    case BattleTurnResult::PLAYER_WON_SPARE:
        cout << "You spare " << monster.getName() << ".\n";
        m_player.recordSpare();
        m_player.recordVictory();
        recordBattleResult(monster, "Spared");
        grantBattleReward(monster, result);
        checkProgressMilestones();
        cout << "Progress toward ending: " << m_player.getVictories() << "/10 victories.\n";
        break;
    case BattleTurnResult::PLAYER_FLED:
        cout << "You retreat for now. No victory is counted.\n";
        break;
    case BattleTurnResult::PLAYER_LOST:
        cout << m_player.getName() << " has fallen. Restoring HP for the prototype build.\n";
        m_player.setHp(m_player.getMaxHp());
        break;
    default:
        break;
    }
}

string Game::getMercyComment(const Monster& monster) const
{
    if (monster.isSpareable())
    {
        return "Mercy is high enough: you can spare this monster now.";
    }

    const int mercy = monster.getMercy();
    const int goal = monster.getMercyGoal();
    if (mercy == 0)
    {
        return "The monster is still hostile.";
    }
    if (mercy * 2 < goal)
    {
        return "Your ACT choices are starting to have an effect.";
    }
    return "The monster is hesitating. Mercy is getting close to the goal.";
}

string Game::buildMercyBar(const Monster& monster) const
{
    const int barWidth = 10;
    const int filledCells = monster.getMercyGoal() <= 0
        ? 0
        : min(barWidth, (monster.getMercy() * barWidth + monster.getMercyGoal() - 1) / monster.getMercyGoal());

    string bar = "[";
    for (int i = 0; i < barWidth; ++i)
    {
        bar += (i < filledCells) ? '#' : '-';
    }
    bar += "]";
    return bar;
}

string Game::getBattleOpeningText(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "Froggit hops forward, unsure whether this is a duel or a conversation.";
    }
    if (name == "MimicBox")
    {
        return "MimicBox rattles nervously. Something inside it wants to be understood.";
    }
    if (name == "QueenByte")
    {
        return "QueenByte descends with theatrical confidence and expects a worthy performance.";
    }
    if (name == "Dustling")
    {
        return "Dustling spins in jittery circles, as if it cannot decide whether to flee or fight.";
    }
    if (name == "GlimmerMoth")
    {
        return "GlimmerMoth scatters shimmering dust and watches you with curious caution.";
    }
    if (name == "Pebblimp")
    {
        return "Pebblimp bounces in place, trying very hard to look tougher than it feels.";
    }
    if (name == "HowlScreen")
    {
        return "HowlScreen crackles to life, projecting static like a warning siren.";
    }
    if (name == "BloomCobra")
    {
        return "BloomCobra rises from the sand, beautiful and dangerous in equal measure.";
    }
    if (name == "Archivore")
    {
        return "Archivore opens ancient metal wings, as if testing whether you deserve to be remembered.";
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return "A cautious foe stands in your way.";
    case MonsterCategory::MINIBOSS:
        return "A tougher opponent studies your every move.";
    case MonsterCategory::BOSS:
        return "The air grows heavy. This battle will demand more than raw force.";
    }

    return "A monster challenges you.";
}

string Game::getBattleHint(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "Kindness and calm discussion seem promising.";
    }
    if (name == "MimicBox")
    {
        return "Observation and a gentle approach may open it up.";
    }
    if (name == "QueenByte")
    {
        return "She seems to appreciate wit, reason and spectacle.";
    }
    if (name == "Dustling")
    {
        return "It looks restless. Humor might work better than pressure.";
    }
    if (name == "GlimmerMoth")
    {
        return "Careful observation and gentle praise might keep it from panicking.";
    }
    if (name == "Pebblimp")
    {
        return "It seems easier to calm than to frighten.";
    }
    if (name == "HowlScreen")
    {
        return "Reason and calm dialogue may cut through the noise.";
    }
    if (name == "BloomCobra")
    {
        return "A respectful approach should work better than threats.";
    }
    if (name == "Archivore")
    {
        return "It values understanding and thoughtful words over brute force.";
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return "Try positive ACT choices before fighting.";
    case MonsterCategory::MINIBOSS:
        return "You may need a stronger sequence of ACT choices.";
    case MonsterCategory::BOSS:
        return "Bosses resist mercy, but they usually have favorite interactions.";
    }

    return "Watch the mercy bar and adapt your strategy.";
}

string Game::getMonsterDescription(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "A nervous marsh wanderer that responds best to kindness and patient conversation.";
    }
    if (name == "MimicBox")
    {
        return "A suspicious living chest whose hostility fades once it feels understood or fed.";
    }
    if (name == "QueenByte")
    {
        return "A theatrical ruler of code who respects wit, logic and dramatic flair.";
    }
    if (name == "Dustling")
    {
        return "A restless sand spirit. Humor and empathy calm it far better than intimidation.";
    }
    if (name == "GlimmerMoth")
    {
        return "A fragile luminous creature that reacts well to observation and gentle praise.";
    }
    if (name == "Pebblimp")
    {
        return "A bluffing stone puffball that is easier to reassure than to scare.";
    }
    if (name == "HowlScreen")
    {
        return "A noisy signal-being whose static weakens when someone speaks with clarity.";
    }
    if (name == "BloomCobra")
    {
        return "A proud desert serpent-flower that dislikes threats but respects thoughtful gestures.";
    }
    if (name == "Archivore")
    {
        return "An ancient keeper of records that values insight, reason and measured dialogue.";
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return "A common foe with a simple emotional pattern.";
    case MonsterCategory::MINIBOSS:
        return "A sturdier opponent that requires a more deliberate approach.";
    case MonsterCategory::BOSS:
        return "A major enemy with high pressure and stronger resistance to mercy.";
    }

    return "An unusual creature from ALTERDUNE.";
}

string Game::getMonsterRegionName(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit" || name == "Dustling")
    {
        return "Sunken Mire";
    }
    if (name == "MimicBox" || name == "GlimmerMoth" || name == "Pebblimp")
    {
        return "Glass Dunes";
    }
    if (name == "HowlScreen" || name == "BloomCobra" || name == "QueenByte")
    {
        return "Signal Wastes";
    }
    if (name == "Archivore")
    {
        return "Ancient Vault";
    }

    return "Unknown Frontier";
}

string Game::getMonsterPhysicalProfile(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "Light amphibian - slippery and reactive";
    }
    if (name == "MimicBox")
    {
        return "Heavy armored shell - strong resistance to frontal pressure";
    }
    if (name == "QueenByte")
    {
        return "Regal pulse construct - elegant but dangerous";
    }
    if (name == "Dustling")
    {
        return "Fast drifting spirit - unstable and evasive";
    }
    if (name == "GlimmerMoth")
    {
        return "Fragile aerial body - luminous and elusive";
    }
    if (name == "Pebblimp")
    {
        return "Compact rocky body - bouncy and stubborn";
    }
    if (name == "HowlScreen")
    {
        return "Angular signal frame - volatile static emitter";
    }
    if (name == "BloomCobra")
    {
        return "Flexible serpent vine - graceful and venomous";
    }
    if (name == "Archivore")
    {
        return "Ancient metal avian - dense and disciplined";
    }

    return "Unknown physique";
}

string Game::getHeroAppearanceBonusText() const
{
    const string& appearanceId = m_player.getAppearanceId();
    if (appearanceId == "vanguard")
    {
        return "Vanguard bonus: better BLADE pressure, slightly steadier against heavy attacks.";
    }
    if (appearanceId == "mystic")
    {
        return "Mystic bonus: stronger PULSE affinity and calmer ACT rhythm.";
    }

    return "Wanderer bonus: balanced style with slightly safer DUST handling.";
}

string Game::getMonsterMoodText(const Monster& monster) const
{
    if (monster.isSpareable())
    {
        return "The monster is ready to give up the fight.";
    }

    const int mercy = monster.getMercy();
    const int mercyGoal = monster.getMercyGoal();
    const int hp = monster.getHp();
    const int maxHp = monster.getMaxHp();

    if (hp * 3 <= maxHp)
    {
        return "It looks shaken and much less confident than before.";
    }
    if (mercy * 3 >= mercyGoal * 2)
    {
        return "Its guard is dropping. A final kind gesture might be enough.";
    }
    if (mercy > 0)
    {
        return "It is still tense, but your actions are getting through.";
    }
    return "It is focused entirely on the fight.";
}

string Game::getMonsterAttackText(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "Froggit splashes muddy water around the arena.";
    }
    if (name == "MimicBox")
    {
        return "MimicBox snaps open and throws sharp debris.";
    }
    if (name == "QueenByte")
    {
        return "QueenByte floods the battlefield with royal code strikes.";
    }
    if (name == "Dustling")
    {
        return "Dustling whirls into a stinging sand spiral.";
    }
    if (name == "GlimmerMoth")
    {
        return "GlimmerMoth sends a haze of sparkling scales drifting toward you.";
    }
    if (name == "Pebblimp")
    {
        return "Pebblimp hurls itself forward in a flurry of clumsy stone bumps.";
    }
    if (name == "HowlScreen")
    {
        return "HowlScreen blasts the arena with waves of buzzing static.";
    }
    if (name == "BloomCobra")
    {
        return "BloomCobra lashes out with thorned vines and a sudden strike.";
    }
    if (name == "Archivore")
    {
        return "Archivore summons razor-sharp pages of light around the battlefield.";
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return monster.getName() + " lashes out quickly.";
    case MonsterCategory::MINIBOSS:
        return monster.getName() + " launches a heavier attack.";
    case MonsterCategory::BOSS:
        return monster.getName() + " unleashes a brutal combo.";
    }

    return monster.getName() + " attacks.";
}

string Game::getMonsterAttackType(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit" || name == "BloomCobra")
    {
        return "Venom";
    }
    if (name == "Dustling" || name == "Pebblimp")
    {
        return "Dust";
    }
    if (name == "MimicBox" || name == "Archivore")
    {
        return "Metal";
    }
    if (name == "QueenByte" || name == "HowlScreen")
    {
        return "Pulse";
    }
    if (name == "GlimmerMoth")
    {
        return "Light";
    }

    return "Strike";
}

string Game::getMonsterElementType(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit")
    {
        return "Water";
    }
    if (name == "MimicBox")
    {
        return "Metal";
    }
    if (name == "QueenByte")
    {
        return "Fire";
    }
    if (name == "Dustling")
    {
        return "Shadow";
    }
    if (name == "GlimmerMoth")
    {
        return "Light";
    }
    if (name == "Pebblimp")
    {
        return "Earth";
    }
    if (name == "HowlScreen")
    {
        return "Storm";
    }
    if (name == "BloomCobra")
    {
        return "Nature";
    }
    if (name == "Archivore")
    {
        return "Shadow";
    }

    return "Neutral";
}

string Game::getAttackElementType(const string& attackStyleId) const
{
    if (attackStyleId == "blade")
    {
        return "Metal";
    }
    if (attackStyleId == "pulse")
    {
        return "Arcane";
    }
    if (attackStyleId == "dust")
    {
        return "Earth";
    }
    if (attackStyleId == "ember")
    {
        return "Fire";
    }
    if (attackStyleId == "tide")
    {
        return "Water";
    }

    return "Neutral";
}

float Game::getElementalEffectiveness(const string& attackElement, const string& targetElement) const
{
    if (attackElement == targetElement && attackElement != "Neutral")
    {
        return 0.8f;
    }

    if (attackElement == "Water" && (targetElement == "Fire" || targetElement == "Earth"))
    {
        return 1.3f;
    }
    if (attackElement == "Fire" && (targetElement == "Nature" || targetElement == "Metal"))
    {
        return 1.3f;
    }
    if (attackElement == "Earth" && (targetElement == "Storm" || targetElement == "Fire"))
    {
        return 1.25f;
    }
    if (attackElement == "Metal" && (targetElement == "Light" || targetElement == "Nature"))
    {
        return 1.2f;
    }
    if (attackElement == "Arcane" && (targetElement == "Shadow" || targetElement == "Storm"))
    {
        return 1.25f;
    }
    if (attackElement == "Fire" && targetElement == "Water")
    {
        return 0.7f;
    }
    if (attackElement == "Water" && targetElement == "Storm")
    {
        return 0.8f;
    }
    if (attackElement == "Earth" && targetElement == "Light")
    {
        return 0.8f;
    }
    if (attackElement == "Metal" && targetElement == "Fire")
    {
        return 0.75f;
    }
    if (attackElement == "Arcane" && targetElement == "Metal")
    {
        return 0.8f;
    }

    return 1.f;
}

string Game::getAttackStyleLabel(const string& attackStyleId) const
{
    if (attackStyleId == "blade")
    {
        return "BLADE";
    }
    if (attackStyleId == "pulse")
    {
        return "PULSE";
    }
    if (attackStyleId == "dust")
    {
        return "DUST";
    }
    if (attackStyleId == "ember")
    {
        return "EMBER";
    }
    if (attackStyleId == "tide")
    {
        return "TIDE";
    }

    return "STRIKE";
}

string Game::getAttackStyleDescription(const string& attackStyleId) const
{
    if (attackStyleId == "blade")
    {
        return "balanced cut";
    }
    if (attackStyleId == "pulse")
    {
        return "arcane burst";
    }
    if (attackStyleId == "dust")
    {
        return "wild unstable hit";
    }
    if (attackStyleId == "ember")
    {
        return "burning arc";
    }
    if (attackStyleId == "tide")
    {
        return "flowing wave";
    }

    return "basic hit";
}

vector<string> Game::getAvailableAttackStyleIds() const
{
    return {"blade", "pulse", "dust", "ember", "tide"};
}

float Game::getPlayerAttackMultiplier(const Monster& monster, const string& attackStyleId) const
{
    float multiplier = getElementalEffectiveness(getAttackElementType(attackStyleId), getMonsterElementType(monster));

    if (m_player.getAppearanceId() == "vanguard" && attackStyleId == "blade")
    {
        multiplier += 0.1f;
    }
    else if (m_player.getAppearanceId() == "mystic" && attackStyleId == "pulse")
    {
        multiplier += 0.12f;
    }
    else if (m_player.getAppearanceId() == "wanderer" && attackStyleId == "dust")
    {
        multiplier += 0.08f;
    }

    return multiplier;
}

float Game::getMonsterAttackMultiplier(const Monster& monster) const
{
    const string attackType = getMonsterAttackType(monster);
    const string elementType = getMonsterElementType(monster);
    float multiplier = 1.f;
    if (attackType == "Dust" && m_player.getDef() >= 5)
    {
        multiplier = 0.85f;
    }
    else if (attackType == "Metal" && m_player.getDef() <= 3)
    {
        multiplier = 1.15f;
    }
    else if (attackType == "Pulse" && m_player.getVictories() < 3)
    {
        multiplier = 1.1f;
    }

    if (m_player.getAppearanceId() == "vanguard" && (attackType == "Metal" || attackType == "Dust" || elementType == "Earth"))
    {
        multiplier -= 0.08f;
    }
    else if (m_player.getAppearanceId() == "mystic" && (attackType == "Pulse" || elementType == "Shadow" || elementType == "Storm"))
    {
        multiplier -= 0.1f;
    }

    return max(0.65f, multiplier);
}

int Game::resolvePlayerFightAction(Monster& monster,
                                   const string& attackStyleId,
                                   bool& criticalHit,
                                   string& effectivenessText,
                                   string& attackLabel)
{
    attackLabel = getAttackStyleLabel(attackStyleId);
    const float styleMultiplier = getPlayerAttackMultiplier(monster, attackStyleId);
    const string attackElement = getAttackElementType(attackStyleId);
    const string monsterElement = getMonsterElementType(monster);
    int criticalChance = attackStyleId == "dust" ? 18 : (attackStyleId == "pulse" ? 12 : 10);
    if (attackStyleId == "ember" || attackStyleId == "tide")
    {
        criticalChance = 11;
    }
    if (m_player.getAppearanceId() == "vanguard" && attackStyleId == "blade")
    {
        criticalChance += 6;
    }
    else if (m_player.getAppearanceId() == "mystic" && attackStyleId == "pulse")
    {
        criticalChance += 5;
    }

    criticalHit = randomInt(1, 100) <= criticalChance;
    const int rawBaseDamage = randomInt(m_player.getAtk() - 2, m_player.getAtk() + 3) + m_nextFightBonus - m_nextFightPenalty;
    const int dealtDamage = monster.takeDamage(static_cast<int>(max(1, rawBaseDamage) * styleMultiplier * (criticalHit ? kCriticalMultiplier : 1.f)));

    if (styleMultiplier >= 1.2f)
    {
        effectivenessText = "It is especially effective against " + monsterElement + ".";
    }
    else if (styleMultiplier <= 0.85f)
    {
        effectivenessText = monsterElement + " resists your " + attackElement + " attack.";
    }
    else
    {
        effectivenessText.clear();
    }

    if (m_nextFightBonus > 0)
    {
        m_nextFightBonus = 0;
    }
    if (m_nextFightPenalty > 0)
    {
        m_nextFightPenalty = 0;
    }

    return dealtDamage;
}

string Game::getItemTacticalEffect(const string& itemName) const
{
    if (itemName == "Bandage")
    {
        return "small heal + guard for the next enemy turn";
    }
    if (itemName == "Snack")
    {
        return "small heal + ACT boost on your next social action";
    }
    if (itemName == "Potion")
    {
        return "solid heal + small boost to your next attack";
    }
    if (itemName == "GlowCandy")
    {
        return "medium heal + strong ACT boost";
    }
    if (itemName == "CactusJuice")
    {
        return "strong heal + strong Fight boost";
    }
    if (itemName == "SuperPotion")
    {
        return "big heal + guard and a small Fight boost";
    }

    return "restores HP";
}

int Game::computeActMercyImpact(const Monster& monster,
                                const ActAction& action,
                                const map<string, int>& actUsage) const
{
    int impact = action.getMercyImpact() + m_nextActBonus - m_nextActPenalty;
    const auto usageIt = actUsage.find(action.getId());
    const int previousUses = usageIt == actUsage.end() ? 0 : usageIt->second;

    if (m_player.getAppearanceId() == "mystic" && impact > 0)
    {
        impact += 2;
    }

    if (impact <= 0)
    {
        if (monster.getName() == "QueenByte" && action.getId() == "INSULT")
        {
            return impact - 15;
        }
        if (monster.getName() == "Dustling" && action.getId() == "THREATEN")
        {
            return impact - 10;
        }
        if (monster.getName() == "BloomCobra" && action.getId() == "THREATEN")
        {
            return impact - 15;
        }
        if (monster.getName() == "Archivore" && action.getId() == "THREATEN")
        {
            return impact - 20;
        }
        return impact;
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        impact += 12;
        break;
    case MonsterCategory::MINIBOSS:
        impact += 8;
        break;
    case MonsterCategory::BOSS:
        impact += 5;
        break;
    }

    if (monster.getMercy() >= monster.getMercyGoal() / 2)
    {
        impact += 5;
    }

    if (previousUses == 0)
    {
        impact += 6;
    }
    else if (previousUses >= 2)
    {
        impact -= 10;
    }
    else
    {
        impact -= 2;
    }

    const string& name = monster.getName();
    const string& actId = action.getId();
    if (name == "Froggit")
    {
        if (actId == "COMPLIMENT")
        {
            impact += 20;
        }
        else if (actId == "DISCUSS")
        {
            impact += 15;
        }
    }
    else if (name == "MimicBox")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "PET")
        {
            impact += 12;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 15;
        }
    }
    else if (name == "QueenByte")
    {
        if (actId == "REASON")
        {
            impact += 18;
        }
        else if (actId == "DANCE")
        {
            impact += 14;
        }
        else if (actId == "JOKE")
        {
            impact += 10;
        }
    }
    else if (name == "Dustling")
    {
        if (actId == "OBSERVE")
        {
            impact += 16;
        }
        else if (actId == "JOKE")
        {
            impact += 18;
        }
    }
    else if (name == "GlimmerMoth")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "COMPLIMENT")
        {
            impact += 16;
        }
        else if (actId == "JOKE")
        {
            impact += 8;
        }
    }
    else if (name == "Pebblimp")
    {
        if (actId == "DISCUSS")
        {
            impact += 17;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 14;
        }
    }
    else if (name == "HowlScreen")
    {
        if (actId == "REASON")
        {
            impact += 20;
        }
        else if (actId == "DISCUSS")
        {
            impact += 14;
        }
    }
    else if (name == "BloomCobra")
    {
        if (actId == "PET")
        {
            impact += 10;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 20;
        }
    }
    else if (name == "Archivore")
    {
        if (actId == "OBSERVE")
        {
            impact += 14;
        }
        else if (actId == "REASON")
        {
            impact += 18;
        }
        else if (actId == "DISCUSS")
        {
            impact += 12;
        }
    }

    return max(0, impact);
}

string Game::getActReactionText(const Monster& monster, const ActAction& action, int delta) const
{
    const string& name = monster.getName();
    const string& actId = action.getId();

    if (delta <= 0)
    {
        if (name == "QueenByte")
        {
            return "QueenByte narrows her eyes. She clearly did not appreciate that.";
        }
        if (name == "Dustling")
        {
            return "Dustling becomes even more erratic and lashes out with nervous energy.";
        }
        if (name == "BloomCobra")
        {
            return "BloomCobra flares its petals wide. That was clearly the wrong move.";
        }
        if (name == "Archivore")
        {
            return "Archivore marks your words as a failure and grows colder.";
        }
        return "The atmosphere hardens instead of calming down.";
    }

    if (name == "Froggit")
    {
        if (actId == "COMPLIMENT")
        {
            return "Froggit blushes. Compliments seem surprisingly powerful here.";
        }
        if (actId == "DISCUSS")
        {
            return "Froggit slows down and listens, one croak at a time.";
        }
    }
    else if (name == "MimicBox")
    {
        if (actId == "OBSERVE")
        {
            return "MimicBox clicks open slightly. Being understood makes it less hostile.";
        }
        if (actId == "OFFER_SNACK")
        {
            return "A pleased rustle comes from inside the box. Food was an excellent idea.";
        }
        if (actId == "PET")
        {
            return "The wooden lid settles down. Somehow, that worked.";
        }
    }
    else if (name == "QueenByte")
    {
        if (actId == "REASON")
        {
            return "QueenByte pauses to consider your argument. Logic reaches her.";
        }
        if (actId == "DANCE")
        {
            return "QueenByte cannot help but admire the dramatic commitment.";
        }
        if (actId == "JOKE")
        {
            return "A tiny smile escapes QueenByte before she regains her composure.";
        }
    }
    else if (name == "Dustling")
    {
        if (actId == "JOKE")
        {
            return "Dustling loses its rhythm for a second, almost like it laughed.";
        }
        if (actId == "OBSERVE")
        {
            return "Dustling seems relieved that someone noticed its unease.";
        }
    }
    else if (name == "GlimmerMoth")
    {
        if (actId == "OBSERVE")
        {
            return "GlimmerMoth slows its wings, as if it is happy to be seen instead of hunted.";
        }
        if (actId == "COMPLIMENT")
        {
            return "The shimmering patterns brighten. GlimmerMoth clearly loves the attention.";
        }
    }
    else if (name == "Pebblimp")
    {
        if (actId == "DISCUSS")
        {
            return "Pebblimp settles down and listens, trying to act serious about it.";
        }
        if (actId == "OFFER_SNACK")
        {
            return "Pebblimp forgets to posture for a moment and happily accepts the snack.";
        }
    }
    else if (name == "HowlScreen")
    {
        if (actId == "REASON")
        {
            return "The static clears a little. Your words are getting through the noise.";
        }
        if (actId == "DISCUSS")
        {
            return "HowlScreen lowers its volume, curious about where this conversation is going.";
        }
    }
    else if (name == "BloomCobra")
    {
        if (actId == "PET")
        {
            return "BloomCobra sways in place, surprised by the calm confidence of that gesture.";
        }
        if (actId == "OFFER_SNACK")
        {
            return "A soft rustle replaces the hiss. BloomCobra responds well to thoughtful offerings.";
        }
    }
    else if (name == "Archivore")
    {
        if (actId == "OBSERVE")
        {
            return "Archivore notices your attention to detail and stops treating you like a nuisance.";
        }
        if (actId == "REASON")
        {
            return "Archivore weighs your logic carefully, like a judge reviewing an old record.";
        }
        if (actId == "DISCUSS")
        {
            return "Archivore folds its wings slightly. It seems willing to continue the exchange.";
        }
    }

    if (delta >= 35)
    {
        return "That choice lands extremely well and changes the flow of the battle.";
    }
    if (delta >= 20)
    {
        return "The monster reacts well. You have found a promising approach.";
    }
    return "The monster settles down a little.";
}

void Game::displayActOutcome(const Monster& monster, const ActAction& action, int previousMercy) const
{
    const int delta = monster.getMercy() - previousMercy;
    if (delta > 0)
    {
        cout << "Mercy increases by " << delta << ". ";
    }
    else if (delta < 0)
    {
        cout << "Mercy decreases by " << -delta << ". ";
    }
    else
    {
        cout << "Mercy stays the same. ";
    }

    if (action.getMercyImpact() < 0)
    {
        cout << getActReactionText(monster, action, delta) << "\n";
    }
    else if (monster.isSpareable())
    {
        cout << getActReactionText(monster, action, delta)
             << " The monster is now ready to be spared.\n";
    }
    else
    {
        cout << getActReactionText(monster, action, delta) << "\n";
    }
}

void Game::grantBattleReward(const Monster& monster, BattleTurnResult result)
{
    int recoveryAmount = 8;
    if (monster.getCategory() == MonsterCategory::MINIBOSS)
    {
        recoveryAmount = 12;
    }
    else if (monster.getCategory() == MonsterCategory::BOSS)
    {
        recoveryAmount = 18;
    }

    const int recoveredHp = m_player.heal(recoveryAmount);
    if (recoveredHp > 0)
    {
        cout << "After the battle, " << m_player.getName() << " recovers "
             << recoveredHp << " HP.\n";
    }
    grantRandomItemReward(monster, result);
    tryGrantRareBattleBlessing(monster);
}

void Game::grantRandomItemReward(const Monster& monster, BattleTurnResult result)
{
    vector<Item> rewardPool;

    if (monster.getCategory() == MonsterCategory::NORMAL)
    {
        rewardPool.push_back(Item("Bandage", ItemType::HEAL, 5, 1));
        rewardPool.push_back(Item("Snack", ItemType::HEAL, 8, 1));
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
    }
    else if (monster.getCategory() == MonsterCategory::MINIBOSS)
    {
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
    }
    else
    {
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
        rewardPool.push_back(Item("SuperPotion", ItemType::HEAL, 30, 1));
    }

    if (result == BattleTurnResult::PLAYER_WON_SPARE)
    {
        rewardPool.push_back(Item("Snack", ItemType::HEAL, 8, 1));
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
    }
    else if (result == BattleTurnResult::PLAYER_WON_KILL)
    {
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
    }

    const int randomIndex = randomInt(0, static_cast<int>(rewardPool.size()) - 1);
    const Item reward = rewardPool[static_cast<size_t>(randomIndex)];
    m_player.addItem(reward);
    cout << "Reward obtained: " << reward.getName() << " x1.\n";
}

void Game::tryGrantRareBattleBlessing(const Monster& monster)
{
    int blessingChance = 0;
    if (monster.getCategory() == MonsterCategory::NORMAL)
    {
        blessingChance = 10;
    }
    else if (monster.getCategory() == MonsterCategory::MINIBOSS)
    {
        blessingChance = 20;
    }
    else
    {
        blessingChance = 35;
    }

    if (randomInt(1, 100) > blessingChance)
    {
        return;
    }

    const int blessingType = randomInt(1, 3);
    if (blessingType == 1)
    {
        m_player.setMaxHp(m_player.getMaxHp() + 5);
        m_player.heal(5);
        cout << "Rare blessing: your journey hardens you. Max HP +5.\n";
    }
    else if (blessingType == 2)
    {
        m_player.setAtk(m_player.getAtk() + 1);
        cout << "Rare blessing: your strikes become sharper. ATK +1.\n";
    }
    else
    {
        m_player.setDef(m_player.getDef() + 1);
        cout << "Rare blessing: your stance becomes steadier. DEF +1.\n";
    }
}

void Game::applyItemBattleEffect(const Item& item)
{
    const string& itemName = item.getName();
    if (itemName == "Bandage")
    {
        ++m_guardCharges;
    }
    else if (itemName == "Snack")
    {
        m_nextActBonus += 5;
    }
    else if (itemName == "Potion")
    {
        m_nextFightBonus += 2;
    }
    else if (itemName == "GlowCandy")
    {
        m_nextActBonus += 10;
    }
    else if (itemName == "CactusJuice")
    {
        m_nextFightBonus += 4;
    }
    else if (itemName == "SuperPotion")
    {
        ++m_guardCharges;
        m_nextFightBonus += 3;
    }
}

void Game::resetBattleModifiers()
{
    m_nextFightBonus = 0;
    m_nextActBonus = 0;
    m_guardCharges = 0;
    m_nextFightPenalty = 0;
    m_nextActPenalty = 0;
    m_battleTurnCounter = 0;
}

void Game::checkProgressMilestones()
{
    if (!m_hpMilestoneUnlocked && m_player.getVictories() >= 3)
    {
        m_hpMilestoneUnlocked = true;
        m_player.setMaxHp(m_player.getMaxHp() + 10);
        m_player.heal(10);
        cout << "Milestone reached: your resilience grows. Max HP +10.\n";
    }

    if (!m_atkMilestoneUnlocked && m_player.getVictories() >= 6)
    {
        m_atkMilestoneUnlocked = true;
        m_player.setAtk(m_player.getAtk() + 2);
        cout << "Milestone reached: your confidence sharpens your attacks. ATK +2.\n";
    }

    if (!m_defMilestoneUnlocked && m_player.getVictories() >= 9)
    {
        m_defMilestoneUnlocked = true;
        m_player.setDef(m_player.getDef() + 1);
        cout << "Milestone reached: you learn to brace for danger. DEF +1.\n";
    }
}

const Item* Game::findFirstUsableInventoryItem() const
{
    for (const Item& item : m_player.getInventory())
    {
        if (item.getQuantity() > 0)
        {
            return &item;
        }
    }

    return nullptr;
}

const ActAction* Game::chooseBestFrontendAct(const Monster& monster) const
{
    const vector<string> actIds = monster.getAvailableActIds();
    const ActAction* bestAction = nullptr;
    int bestScore = -100000;

    for (const string& actId : actIds)
    {
        const auto actionIt = m_actCatalog.find(actId);
        if (actionIt == m_actCatalog.end())
        {
            continue;
        }

        const int score = computeActMercyImpact(monster, actionIt->second, m_frontendActUsage);
        if (!bestAction || score > bestScore)
        {
            bestAction = &actionIt->second;
            bestScore = score;
        }
    }

    return bestAction;
}

FrontendBattleViewData Game::buildFrontendBattleViewData(const Monster& monster, const string& battleLogText) const
{
    FrontendBattleViewData data;
    data.regionName = getMonsterRegionName(monster);
    data.openingText = getBattleOpeningText(monster);
    data.moodText = getMonsterMoodText(monster);
    data.hintText = getBattleHint(monster);
    data.battleLogText = battleLogText;
    data.playerActionText = m_lastPlayerTurnSummary;
    data.monsterActionText = m_lastMonsterTurnSummary;

      data.activeBonusesText = "No active bonuses";
      if (m_nextFightBonus > 0 || m_nextActBonus > 0 || m_guardCharges > 0 || m_nextFightPenalty > 0 || m_nextActPenalty > 0)
      {
          data.activeBonusesText = "Bonuses:";
        if (m_nextFightBonus > 0)
        {
            data.activeBonusesText += " Fight +" + to_string(m_nextFightBonus);
        }
        if (m_nextActBonus > 0)
        {
            data.activeBonusesText += " ACT +" + to_string(m_nextActBonus);
        }
          if (m_guardCharges > 0)
          {
              data.activeBonusesText += " Guard x" + to_string(m_guardCharges);
          }
          if (m_nextFightPenalty > 0)
          {
              data.activeBonusesText += " Fight -" + to_string(m_nextFightPenalty);
          }
          if (m_nextActPenalty > 0)
          {
              data.activeBonusesText += " ACT -" + to_string(m_nextActPenalty);
          }
      }

    data.playerName = m_player.getName();
    data.monsterName = monster.getName();
    data.monsterCategory = Monster::categoryToString(monster.getCategory());
    data.monsterElementType = getMonsterElementType(monster);
    data.monsterPhysique = getMonsterPhysicalProfile(monster);
    data.monsterDescription = getMonsterDescription(monster);
    data.playerHpBar = {"HP", m_player.getHp(), m_player.getMaxHp()};
    data.monsterHpBar = {"HP", monster.getHp(), monster.getMaxHp()};
    data.mercyBar = {"MERCY", monster.getMercy(), monster.getMercyGoal()};
    data.primaryActions = {
        {"fight", "FIGHT", true},
        {"act", "ACT", !monster.getAvailableActIds().empty()},
        {"item", "ITEM", findFirstUsableInventoryItem() != nullptr},
        {"mercy", "MERCY", monster.isSpareable()}
    };
    data.secondaryActions = {
        {"back", "BACK", true}
    };

    return data;
}

void Game::startBattle()
{
    const int selectedIndex = chooseMonsterIndex();
    map<string, int> actUsage;
    resetBattleModifiers();
    unique_ptr<Monster> monster;
    if (selectedIndex == 0)
    {
        monster = createRandomMonster();
    }
    else
    {
        monster = m_monsterCatalog[selectedIndex - 1]->clone();
    }

    cout << "\n=== Battle Start ===\n";
    cout << "A wild " << monster->getName() << " appears.\n";
    cout << getBattleOpeningText(*monster) << "\n";

    bool battleEnded = false;
    while (!battleEnded && m_player.isAlive() && monster->isAlive())
    {
        displayBattleState(*monster);

        cout << "1. Fight\n";
        cout << "2. ACT\n";
        cout << "3. Item\n";
        cout << "4. Mercy\n";
        cout << "5. Run\n";
        cout << "Choice: ";

        const int choice = readIntChoice(1, 5);
        BattleTurnResult result = BattleTurnResult::CONTINUE;

        if (choice == 1)
        {
            result = handleFightAction(*monster);
        }
        else if (choice == 2)
        {
            result = handleActAction(*monster, actUsage);
        }
        else if (choice == 3)
        {
            result = handleItemAction(*monster);
        }
        else if (choice == 4)
        {
            result = handleMercyAction(*monster);
        }
        else if (choice == 5)
        {
            result = BattleTurnResult::PLAYER_FLED;
        }

        if (result == BattleTurnResult::PLAYER_WON_KILL
            || result == BattleTurnResult::PLAYER_WON_SPARE
            || result == BattleTurnResult::PLAYER_FLED
            || result == BattleTurnResult::PLAYER_LOST)
        {
            concludeBattle(*monster, result);
            battleEnded = true;
            continue;
        }

        if (result == BattleTurnResult::NO_TURN_SPENT)
        {
            continue;
        }

        if (monster->isAlive())
        {
            result = handleMonsterTurn(*monster);
            if (result == BattleTurnResult::PLAYER_LOST)
            {
                concludeBattle(*monster, result);
                battleEnded = true;
            }
        }
    }
}

bool Game::hasReachedEnding() const
{
    return m_player.getVictories() >= 10;
}

void Game::displayEndingAndExit()
{
    cout << "\n=== Final Outcome ===\n";
    cout << "Victories: " << m_player.getVictories()
         << " | Kills: " << m_player.getKills()
         << " | Spares: " << m_player.getSpares() << "\n";

    if (m_player.getKills() == m_player.getVictories())
    {
        cout << "Genocidal ending: every won battle ended in violence.\n";
    }
    else if (m_player.getSpares() == m_player.getVictories())
    {
        cout << "Pacifist ending: every won battle ended with mercy.\n";
    }
    else
    {
        cout << "Neutral ending: your journey mixed combat and mercy.\n";
    }
}

unique_ptr<Monster> Game::createRandomMonster()
{
    const vector<size_t> availableIndices = getAvailableMonsterIndices();
    if (availableIndices.size() == 1)
    {
        return m_monsterCatalog[availableIndices.front()]->clone();
    }

    uniform_int_distribution<int> distribution(0, static_cast<int>(availableIndices.size()) - 1);
    return m_monsterCatalog[availableIndices[distribution(m_rng)]]->clone();
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
    for (BestiaryEntry& entry : m_bestiary)
    {
        if (entry.getName() == monster.getName())
        {
            entry.setResult(result);
            return;
        }
    }

    m_bestiary.emplace_back(monster.getName(),
                            monster.getCategory(),
                            monster.getMaxHp(),
                            monster.getAtk(),
                            monster.getDef(),
                            getMonsterDescription(monster),
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
