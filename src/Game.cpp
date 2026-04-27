#include "Game.h"

#include <algorithm>
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
      m_languageCode("en"),
      m_nextFightBonus(0),
      m_nextActBonus(0),
      m_guardCharges(0),
      m_nextFightPenalty(0),
      m_nextActPenalty(0),
      m_battleTurnCounter(0),
      m_hpMilestoneUnlocked(false),
      m_atkMilestoneUnlocked(false),
      m_defMilestoneUnlocked(false),
      m_regionKeys(),
      m_clearedEncounters(),
      m_frontendBattleMonster(),
      m_frontendActUsage(),
      m_frontendBattleLog("A battle preview is ready."),
      m_lastPlayerTurnSummary(),
      m_lastMonsterTurnSummary()
{
}

void Game::setLanguage(const string& languageCode)
{
    m_languageCode = (languageCode == "fr") ? "fr" : "en";
}

const string& Game::getLanguage() const
{
    return m_languageCode;
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
    m_regionKeys.clear();
    m_clearedEncounters.clear();
    m_frontendBattleMonster.reset();
    m_frontendActUsage.clear();
    m_frontendBattleLog = tr("A battle preview is ready.", "Une previsualisation du combat est prete.");
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
            cout << tr("Thanks for playing ALTERDUNE.\n", "Merci d'avoir joue a ALTERDUNE.\n");
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
    data.progressText = tr("Victories: ", "Victoires : ") + to_string(m_player.getVictories())
        + "  |  " + tr("Keys: ", "Cles : ") + to_string(getRegionKeyCount()) + "/4"
        + "  |  " + tr("Kills: ", "Tues : ") + to_string(m_player.getKills())
        + "  |  " + tr("Spares: ", "Epargnes : ") + to_string(m_player.getSpares());

    data.unlockedLandsText = tr("Unlocked lands: ", "Contrees debloquees : ");
    bool first = true;
    for (const string& regionName : getCampaignRegionOrder())
    {
        if (!isRegionUnlocked(regionName))
        {
            continue;
        }

        if (!first)
        {
            data.unlockedLandsText += ", ";
        }
        data.unlockedLandsText += localizeRegionName(regionName);
        first = false;
    }

    if (first)
    {
        data.unlockedLandsText += localizeRegionName("Sunken Mire");
    }

    data.buttons = {
        {"bestiary", "B", tr("Bestiary", "Bestiaire"), tr("Review unlocked creatures", "Revoir les creatures debloquees"), true},
        {"start_battle", ">", tr("Start Battle", "Lancer l'aventure"), tr("Open the route map", "Ouvrir la carte d'aventure"), true},
        {"player_stats", "S", tr("Player Stats", "Statistiques"), tr("Growth, keys and learned styles", "Progression, cles et styles appris"), true},
        {"items", "I", tr("Items", "Objets"), tr("Check healing and tactical supplies", "Voir les soins et objets tactiques"), true},
        {"quit", "X", tr("Quit", "Quitter"), tr("Leave the game", "Quitter le jeu"), true}
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

    data.routeText = tr("Objective: ", "Objectif : ") + getNextObjectiveText();
    data.milestoneText = tr("Styles learned: ", "Styles appris : ");
    const vector<string> attackIds = getAvailableAttackStyleIds();
    for (size_t i = 0; i < attackIds.size(); ++i)
    {
        if (i > 0)
        {
            data.milestoneText += ", ";
        }
        data.milestoneText += getAttackStyleLabel(attackIds[i]);
    }
    return data;
}

FrontendWorldMapViewData Game::buildFrontendWorldMapViewData() const
{
    FrontendWorldMapViewData data;
    data.title = tr("World Route", "Carte du voyage");
    data.subtitle = tr("Move from node to node and challenge the next available encounter.",
                       "Avance de case en case et affronte la prochaine rencontre disponible.");
    data.objectiveText = getNextObjectiveText();
    data.nodes = buildFrontendMonsterSelectionViewData();
    return data;
}

vector<FrontendMonsterCardViewData> Game::buildFrontendMonsterSelectionViewData() const
{
    vector<FrontendMonsterCardViewData> cards;
    const vector<size_t> orderedIndices = getCampaignOrderedMonsterIndices();
    map<string, int> regionNodeCounts;
    for (size_t i = 0; i < orderedIndices.size(); ++i)
    {
        const Monster& monster = *m_monsterCatalog[orderedIndices[i]];
        const string regionName = getMonsterRegionName(monster);
        FrontendMonsterCardViewData card;
        card.name = monster.getName();
        card.category = localizeCategoryName(monster.getCategory());
        card.elementType = localizeElementName(getMonsterElementType(monster));
        card.elementIcon = getElementIcon(getMonsterElementType(monster));
        card.land = localizeRegionName(regionName);
        card.physique = getMonsterPhysicalProfile(monster);
        card.description = getMonsterDescription(monster);
        const int regionNodeIndex = regionNodeCounts[regionName]++;
        card.routeLabel = tr("Encounter ", "Rencontre ") + to_string(regionNodeIndex + 1);
        card.rewardHint = getMonsterRewardHint(monster);
        card.hp = monster.getMaxHp();
        card.atk = monster.getAtk();
        card.def = monster.getDef();
        card.unlocked = isMonsterUnlocked(monster);
        card.cleared = isEncounterCleared(monster.getName());
        card.availableNow = isEncounterAvailableNow(monster);
        card.keyBattle = doesEncounterGrantKey(monster);
        card.regionIndex = 0;
        card.nodeIndex = regionNodeIndex;

        int regionIndex = 0;
        for (const string& campaignRegionName : getCampaignRegionOrder())
        {
            if (regionName == campaignRegionName)
            {
                card.regionIndex = regionIndex;
                break;
            }
            ++regionIndex;
        }

        if (monster.getCategory() == MonsterCategory::NORMAL)
        {
            card.threat = localizeThreatName("Low");
        }
        else if (monster.getCategory() == MonsterCategory::MINIBOSS)
        {
            card.threat = localizeThreatName("Medium");
        }
        else
        {
            card.threat = localizeThreatName("High");
        }

        cards.push_back(card);
    }

    return cards;
}

FrontendBattleViewData Game::buildFrontendBattlePreviewViewData(size_t displayIndex) const
{
    const vector<size_t> orderedIndices = getCampaignOrderedMonsterIndices();

    if (orderedIndices.empty())
    {
        FrontendBattleViewData data;
        data.regionName = localizeRegionName("Unknown Frontier");
        data.openingText = tr("No battle data available.", "Aucune donnee de combat disponible.");
        data.hintText = tr("Load monsters first.", "Charge d'abord les monstres.");
        data.playerName = m_player.getName();
        data.monsterName = tr("Unknown", "Inconnu");
        data.monsterCategory = tr("NONE", "AUCUN");
        data.playerHpBar = {"HP", m_player.getHp(), m_player.getMaxHp()};
        data.monsterHpBar = {"HP", 0, 1};
        data.mercyBar = {"MERCY", 0, 100};
        return data;
    }

    if (displayIndex >= orderedIndices.size())
    {
        displayIndex = 0;
    }

    const Monster& monster = *m_monsterCatalog[orderedIndices[displayIndex]];
    return buildFrontendBattleViewData(monster, getMonsterDescription(monster));
}

vector<FrontendBestiaryEntryViewData> Game::buildFrontendBestiaryViewData() const
{
    vector<FrontendBestiaryEntryViewData> entries;
    for (const BestiaryEntry& entry : m_bestiary)
    {
        FrontendBestiaryEntryViewData viewEntry;
        viewEntry.name = entry.getName();
        viewEntry.category = localizeCategoryName(entry.getCategory());
        viewEntry.elementType = localizeElementName("Neutral");
        viewEntry.elementIcon = getElementIcon("Neutral");
        viewEntry.physique = tr("Unknown physique", "Physique inconnue");
        for (const unique_ptr<Monster>& monsterPtr : m_monsterCatalog)
        {
            if (monsterPtr->getName() == entry.getName())
            {
                viewEntry.elementType = localizeElementName(getMonsterElementType(*monsterPtr));
                viewEntry.elementIcon = getElementIcon(getMonsterElementType(*monsterPtr));
                viewEntry.physique = getMonsterPhysicalProfile(*monsterPtr);
                break;
            }
        }
        viewEntry.result = entry.getResult();
        viewEntry.description = entry.getDescription();
        viewEntry.hp = entry.getMaxHp();
        viewEntry.atk = entry.getAtk();
        viewEntry.def = entry.getDef();

        viewEntry.land = localizeRegionName("Unknown Frontier");
        for (const unique_ptr<Monster>& monsterPtr : m_monsterCatalog)
        {
            if (monsterPtr->getName() == entry.getName())
            {
                viewEntry.land = localizeRegionName(getMonsterRegionName(*monsterPtr));
                break;
            }
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
        viewItem.type = isFrench() && item.getType() == ItemType::HEAL ? "SOIN" : Item::itemTypeToString(item.getType());
        viewItem.tacticalEffect = getItemTacticalEffect(item.getName());
        viewItem.value = item.getValue();
        viewItem.quantity = item.getQuantity();
        items.push_back(viewItem);
    }

    return items;
}

bool Game::startFrontendBattle(size_t displayIndex)
{
    const vector<size_t> orderedIndices = getCampaignOrderedMonsterIndices();
    if (orderedIndices.empty())
    {
        return false;
    }

    if (displayIndex >= orderedIndices.size())
    {
        displayIndex = 0;
    }

    const Monster& candidate = *m_monsterCatalog[orderedIndices[displayIndex]];
    if (!isEncounterAvailableNow(candidate))
    {
        return false;
    }

    resetBattleModifiers();
    m_frontendActUsage.clear();
    m_frontendBattleMonster = m_monsterCatalog[orderedIndices[displayIndex]]->clone();
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
        return tr("No frontend battle is active.", "Aucun combat frontend n'est actif.");
    }

    Monster& monster = *m_frontendBattleMonster;
    string logText;

    if (actionId == "fight")
    {
        const vector<string> attackIds = getAvailableAttackStyleIds();
        if (attackIds.empty())
        {
            m_frontendBattleLog = tr("No fight style is available.", "Aucun style d'attaque n'est disponible.");
            return m_frontendBattleLog;
        }

        bool criticalHit = false;
        string effectivenessText;
        string attackLabel;
        const int dealtDamage = resolvePlayerFightAction(monster, attackIds.front(), criticalHit, effectivenessText, attackLabel);
        logText = m_player.getName() + tr(" uses ", " utilise ") + attackLabel + tr(" and deals ", " et inflige ") + to_string(dealtDamage) + tr(" damage.", " degats.");
        if (criticalHit)
        {
            logText += tr(" Critical hit!", " Coup critique !");
        }
        if (!effectivenessText.empty())
        {
            logText += " " + effectivenessText;
        }

        if (!monster.isAlive())
        {
            m_lastPlayerTurnSummary = logText + " " + monster.getName() + tr(" is defeated.", " est vaincu.");
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
            m_frontendBattleLog = tr("Ce monstre n'a aucune option ACT disponible.", "Ce monstre n'a aucune option ACT disponible.");
            return m_frontendBattleLog;
        }

        const int previousMercy = monster.getMercy();
        monster.addMercy(computeActMercyImpact(monster, *chosenAction, m_frontendActUsage));
        ++m_frontendActUsage[chosenAction->getId()];
        const int delta = monster.getMercy() - previousMercy;
        logText = getLocalizedActText(*chosenAction) + " " + getActReactionText(monster, *chosenAction, delta);
        if (m_nextActBonus > 0)
        {
            logText += tr(" The prepared ACT boost is consumed.", " Le bonus ACT prepare est consomme.");
            m_nextActBonus = 0;
        }
    }
    else if (actionId == "item")
    {
        const Item* item = findFirstUsableInventoryItem();
        if (!item)
        {
            m_frontendBattleLog = tr("No usable item is available.", "Aucun objet utilisable n'est disponible.");
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

        m_lastPlayerTurnSummary = itemName + tr(" used. ", " utilise. ") + getItemTacticalEffect(itemName) + ".";
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        return m_frontendBattleLog;
    }
    else if (actionId == "mercy")
    {
        if (monster.isSpareable())
        {
            m_lastPlayerTurnSummary = tr("You spare ", "Tu epargnes ") + monster.getName() + ".";
            m_frontendBattleLog = m_lastPlayerTurnSummary;
            concludeBattle(monster, BattleTurnResult::PLAYER_WON_SPARE);
            m_frontendBattleMonster.reset();
            return m_frontendBattleLog;
        }

        m_lastPlayerTurnSummary = monster.getName() + tr(" is not ready to be spared.", " n'est pas pret a etre epargne.");
        m_frontendBattleLog = m_lastPlayerTurnSummary;
        return m_frontendBattleLog;
    }
    else
    {
        m_lastPlayerTurnSummary = tr("Unknown action.", "Action inconnue.");
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
            m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " " + tr("You were overwhelmed, but the prototype restores you.", "Tu as ete submerge, mais le prototype te restaure.");
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
        button.icon = getElementIcon(getAttackElementType(attackStyleId));
        button.label = getAttackStyleLabel(attackStyleId);
        button.subtitle = getAttackStyleDescription(attackStyleId) + " | " + localizeElementName(getAttackElementType(attackStyleId));
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
        button.icon = "A";
        button.label = actionIt->second.getId() + " (" + to_string(actionIt->second.getMercyImpact()) + ")";
        button.subtitle = getLocalizedActText(actionIt->second);
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
        button.icon = "I";
        button.label = item.getName() + " x" + to_string(item.getQuantity());
        button.subtitle = getItemTacticalEffect(item.getName());
        button.enabled = item.getQuantity() > 0;
        options.push_back(button);
    }

    return options;
}

string Game::performFrontendFightByIndex(size_t optionIndex)
{
    if (!m_frontendBattleMonster)
    {
        return tr("No frontend battle is active.", "Aucun combat frontend n'est actif.");
    }

    const vector<string> attackIds = getAvailableAttackStyleIds();
    if (optionIndex >= attackIds.size())
    {
        return tr("Invalid fight style selection.", "Selection d'attaque invalide.");
    }

    Monster& monster = *m_frontendBattleMonster;
    bool criticalHit = false;
    string effectivenessText;
    string attackLabel;
    const int dealtDamage = resolvePlayerFightAction(monster, attackIds[optionIndex], criticalHit, effectivenessText, attackLabel);

    string logText = m_player.getName() + tr(" uses ", " utilise ") + attackLabel + tr(" and deals ", " et inflige ") + to_string(dealtDamage) + tr(" damage.", " degats.");
    if (criticalHit)
    {
        logText += tr(" Critical hit!", " Coup critique !");
    }
    if (!effectivenessText.empty())
    {
        logText += " " + effectivenessText;
    }

    if (!monster.isAlive())
    {
        m_lastPlayerTurnSummary = logText + " " + monster.getName() + tr(" is defeated.", " est vaincu.");
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
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " " + tr("You were overwhelmed, but the prototype restores you.", "Tu as ete submerge, mais le prototype te restaure.");
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
        return tr("No frontend battle is active.", "Aucun combat frontend n'est actif.");
    }

    const vector<string> actIds = m_frontendBattleMonster->getAvailableActIds();
    if (optionIndex >= actIds.size())
    {
        return tr("Invalid ACT selection.", "Selection ACT invalide.");
    }

    const auto actionIt = m_actCatalog.find(actIds[optionIndex]);
    if (actionIt == m_actCatalog.end())
    {
        return tr("Unknown ACT.", "ACT inconnu.");
    }

    Monster& monster = *m_frontendBattleMonster;
    const ActAction& chosenAction = actionIt->second;
    const int previousMercy = monster.getMercy();
    monster.addMercy(computeActMercyImpact(monster, chosenAction, m_frontendActUsage));
    ++m_frontendActUsage[chosenAction.getId()];
    const int delta = monster.getMercy() - previousMercy;

    string logText = getLocalizedActText(chosenAction) + " " + getActReactionText(monster, chosenAction, delta);
    if (m_nextActBonus > 0)
    {
        logText += tr(" The prepared ACT boost is consumed.", " Le bonus ACT prepare est consomme.");
        m_nextActBonus = 0;
    }

    m_lastPlayerTurnSummary = logText;
    const BattleTurnResult monsterTurnResult = handleMonsterTurn(monster);
    if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
    {
        concludeBattle(monster, monsterTurnResult);
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " " + tr("You were overwhelmed, but the prototype restores you.", "Tu as ete submerge, mais le prototype te restaure.");
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
        return tr("No frontend battle is active.", "Aucun combat frontend n'est actif.");
    }

    if (optionIndex >= m_player.getInventory().size())
    {
        return tr("Invalid item selection.", "Selection d'objet invalide.");
    }

    const Item& selectedItem = m_player.getInventory()[optionIndex];
    if (selectedItem.getQuantity() <= 0)
    {
        return selectedItem.getName() + tr(" is out of stock.", " n'est plus en stock.");
    }

    const string itemName = selectedItem.getName();
    if (m_player.useItem(optionIndex, cout))
    {
        applyItemBattleEffect(selectedItem);
    }

    m_lastPlayerTurnSummary = itemName + tr(" used. ", " utilise. ") + getItemTacticalEffect(itemName) + ".";
    m_frontendBattleLog = m_lastPlayerTurnSummary;

    Monster& monster = *m_frontendBattleMonster;
    const BattleTurnResult monsterTurnResult = handleMonsterTurn(monster);
    if (monsterTurnResult == BattleTurnResult::PLAYER_LOST)
    {
        concludeBattle(monster, monsterTurnResult);
        m_frontendBattleLog = m_lastPlayerTurnSummary + " " + m_lastMonsterTurnSummary + " " + tr("You were overwhelmed, but the prototype restores you.", "Tu as ete submerge, mais le prototype te restaure.");
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
    m_frontendBattleLog = tr("You step back from the battle preview.", "Tu prends du recul par rapport a l'aperçu du combat.");
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
    cout << "\n=== ALTERDUNE - " << tr("Startup Summary", "Resume de demarrage") << " ===\n";
    cout << tr("Player: ", "Joueur : ") << m_player.getName() << "\n";
    cout << "HP: " << m_player.getHp() << "/" << m_player.getMaxHp() << "\n";
    cout << tr("Loaded monsters: ", "Monstres charges : ") << m_monsterCatalog.size() << "\n";
    m_player.displayItems(cout);
    cout << "\n";
}

void Game::displayMainMenu() const
{
    cout << "\n=== " << tr("Main Menu", "Menu principal") << " ===\n";
    cout << tr("Progress: ", "Progression : ") << m_player.getVictories() << tr("/10 victories", "/10 victoires")
         << " | " << tr("Kills: ", "Kills : ") << m_player.getKills()
         << " | " << tr("Spares: ", "Spares : ") << m_player.getSpares() << "\n";
    cout << tr("Milestones: 3 wins (+10 HP), 6 wins (+2 ATK), 9 wins (+1 DEF)\n",
               "Paliers : 3 victoires (+10 HP), 6 victoires (+2 ATK), 9 victoires (+1 DEF)\n");
    cout << tr("Unlocked lands: ", "Contrees debloquees : ") << localizeRegionName("Sunken Mire");
    if (m_player.getVictories() >= 2)
    {
        cout << ", " << localizeRegionName("Glass Dunes");
    }
    if (m_player.getVictories() >= 4)
    {
        cout << ", " << localizeRegionName("Signal Wastes");
    }
    if (m_player.getVictories() >= 7)
    {
        cout << ", " << localizeRegionName("Ancient Vault");
    }
    cout << "\n";
    cout << "1. " << tr("Bestiary", "Bestiaire") << "\n";
    cout << "2. " << tr("Start Battle", "Lancer un combat") << "\n";
    cout << "3. " << tr("Player Stats", "Statistiques") << "\n";
    cout << "4. " << tr("Items", "Objets") << "\n";
    cout << "5. " << tr("Quit", "Quitter") << "\n";
    cout << tr("Choice: ", "Choix : ");
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
        cout << tr("Please enter a valid choice (", "Entre un choix valide (")
             << minValue << "-" << maxValue << "): ";
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
    cout << "\n=== " << tr("Bestiary", "Bestiaire") << " ===\n";
    if (m_bestiary.empty())
    {
        cout << tr("No monster has been encountered yet.\n",
                   "Aucun monstre n'a encore ete rencontre.\n");
        return;
    }

    cout << tr("Entries unlocked: ", "Entrees debloquees : ") << m_bestiary.size() << "\n";

    for (const BestiaryEntry& entry : m_bestiary)
    {
        cout << entry.getName()
             << " | " << tr("Category: ", "Categorie : ") << localizeCategoryName(entry.getCategory())
             << " | HP: " << entry.getMaxHp()
             << " | ATK: " << entry.getAtk()
             << " | DEF: " << entry.getDef()
             << " | " << tr("Result: ", "Resultat : ")
             << (entry.getResult() == "Killed" ? tr("Killed", "Tue") : (entry.getResult() == "Spared" ? tr("Spared", "Epargne") : entry.getResult()))
             << "\n";
        cout << "   " << tr("Notes: ", "Notes : ") << entry.getDescription() << "\n";
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
    cout << tr("Healing reserves: ", "Reserve de soin : ") << totalHealingValue
         << tr(" total HP across ", " HP de soin repartis sur ")
         << totalConsumables
         << tr(" consumables.\n", " consommables.\n");
    cout << tr("Victories needed for an ending: ", "Victoires restantes avant une fin : ")
         << max(0, 10 - m_player.getVictories()) << "\n";
}

void Game::showItems()
{
    cout << "\n";
    m_player.displayItems(cout);

    if (m_player.getInventory().empty())
    {
        return;
    }

    cout << tr("Tactical effects in battle:\n", "Effets tactiques en combat :\n");
    for (const Item& item : m_player.getInventory())
    {
        cout << "- " << item.getName() << ": " << getItemTacticalEffect(item.getName()) << "\n";
    }

    cout << tr("Use an item now? (0 = no, item number = yes): ",
               "Utiliser un objet maintenant ? (0 = non, numero = oui) : ");
    const int choice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (choice > 0)
    {
        m_player.useItem(static_cast<size_t>(choice - 1), cout);
    }
}

int Game::chooseMonsterIndex() const
{
    const vector<size_t> availableIndices = getAvailableMonsterIndices();

    cout << "\n=== " << tr("Choose Opponent", "Choisir un adversaire") << " ===\n";
    cout << "0. " << tr("Random monster", "Monstre aleatoire") << "\n";

    for (size_t i = 0; i < availableIndices.size(); ++i)
    {
        const Monster& monster = *m_monsterCatalog[availableIndices[i]];
        cout << i + 1 << ". " << monster.getName()
             << " [" << Monster::categoryToString(monster.getCategory()) << "]"
             << " | HP: " << monster.getMaxHp()
             << " | ATK: " << monster.getAtk()
             << " | DEF: " << monster.getDef()
             << " | " << tr("Land: ", "Region : ") << getMonsterRegionName(monster);

        if (monster.getCategory() == MonsterCategory::NORMAL)
        {
            cout << " | " << tr("Threat: Low", "Menace : faible");
        }
        else if (monster.getCategory() == MonsterCategory::MINIBOSS)
        {
            cout << " | " << tr("Threat: Medium", "Menace : moyenne");
        }
        else
        {
            cout << " | " << tr("Threat: High", "Menace : elevee");
        }

        cout << "\n";
    }

    const int lockedMonsters = static_cast<int>(m_monsterCatalog.size() - availableIndices.size());
    if (lockedMonsters > 0)
    {
        cout << lockedMonsters << tr(" opponents remain hidden in farther lands.\n",
                                     " adversaires restent caches dans des contrees plus lointaines.\n");
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
    for (size_t i : getCampaignOrderedMonsterIndices())
    {
        if (isMonsterUnlocked(*m_monsterCatalog[i]))
        {
            availableIndices.push_back(i);
        }
    }

    return availableIndices;
}

vector<size_t> Game::getCampaignOrderedMonsterIndices() const
{
    vector<size_t> orderedIndices;
    for (const string& regionName : getCampaignRegionOrder())
    {
        for (const string& monsterName : getRegionEncounterOrder(regionName))
        {
            for (size_t i = 0; i < m_monsterCatalog.size(); ++i)
            {
                if (m_monsterCatalog[i]->getName() == monsterName)
                {
                    orderedIndices.push_back(i);
                    break;
                }
            }
        }
    }

    return orderedIndices;
}

bool Game::isMonsterUnlocked(const Monster& monster) const
{
    if (!isRegionUnlocked(getMonsterRegionName(monster)))
    {
        return false;
    }

    const vector<string> encounterOrder = getRegionEncounterOrder(getMonsterRegionName(monster));
    auto encounterIt = find(encounterOrder.begin(), encounterOrder.end(), monster.getName());
    if (encounterIt == encounterOrder.end())
    {
        return false;
    }

    if (encounterIt == encounterOrder.begin())
    {
        return true;
    }

    for (auto it = encounterOrder.begin(); it != encounterIt; ++it)
    {
        if (!isEncounterCleared(*it))
        {
            return false;
        }
    }

    return true;
}

bool Game::isEncounterCleared(const string& monsterName) const
{
    return m_clearedEncounters.find(monsterName) != m_clearedEncounters.end();
}

bool Game::isEncounterAvailableNow(const Monster& monster) const
{
    return isMonsterUnlocked(monster) && !isEncounterCleared(monster.getName());
}

bool Game::isRegionUnlocked(const string& regionName) const
{
    const string requiredKey = getRequiredKeyForRegion(regionName);
    return requiredKey.empty() || m_regionKeys.find(requiredKey) != m_regionKeys.end();
}

bool Game::doesEncounterGrantKey(const Monster& monster) const
{
    const string regionName = getMonsterRegionName(monster);
    const vector<string> encounterOrder = getRegionEncounterOrder(regionName);
    return !encounterOrder.empty() && encounterOrder.back() == monster.getName() && !getKeyNameForRegion(regionName).empty();
}

bool Game::hasRegionKey(const string& regionName) const
{
    const string keyName = getKeyNameForRegion(regionName);
    return !keyName.empty() && m_regionKeys.find(keyName) != m_regionKeys.end();
}

int Game::getRegionKeyCount() const
{
    return static_cast<int>(m_regionKeys.size());
}

vector<string> Game::getCampaignRegionOrder() const
{
    return {"Sunken Mire", "Glass Dunes", "Signal Wastes", "Ancient Vault", "Unknown Frontier"};
}

vector<string> Game::getRegionEncounterOrder(const string& regionName) const
{
    if (regionName == "Sunken Mire")
    {
        return {"BogSlime", "Froggit", "Dustling", "Miresting", "Mudscale"};
    }
    if (regionName == "Glass Dunes")
    {
        return {"GlimmerMoth", "Pebblimp", "Shardback", "MimicBox", "QueenByte"};
    }
    if (regionName == "Signal Wastes")
    {
        return {"HowlScreen", "BloomCobra", "AshDrake", "CinderHex", "Solaraith"};
    }
    if (regionName == "Ancient Vault")
    {
        return {"AegisSlime", "TideWarden", "RuneDemon", "MedusaPrime", "Archivore"};
    }
    if (regionName == "Unknown Frontier")
    {
        return {"NullSaint"};
    }

    return {};
}

string Game::getRequiredKeyForRegion(const string& regionName) const
{
    if (regionName == "Glass Dunes") return "mire_key";
    if (regionName == "Signal Wastes") return "dune_key";
    if (regionName == "Ancient Vault") return "signal_key";
    if (regionName == "Unknown Frontier") return "vault_key";
    return "";
}

string Game::getKeyNameForRegion(const string& regionName) const
{
    if (regionName == "Sunken Mire") return "mire_key";
    if (regionName == "Glass Dunes") return "dune_key";
    if (regionName == "Signal Wastes") return "signal_key";
    if (regionName == "Ancient Vault") return "vault_key";
    return "";
}

string Game::getNextObjectiveText() const
{
    for (const string& regionName : getCampaignRegionOrder())
    {
        if (!isRegionUnlocked(regionName))
        {
            return tr("Recover the key that opens ", "Recupere la cle qui ouvre ")
                + localizeRegionName(regionName) + ".";
        }

        for (const string& encounterName : getRegionEncounterOrder(regionName))
        {
            if (!isEncounterCleared(encounterName))
            {
                const string objective = tr("Reach ", "Atteins ") + encounterName;
                const auto monsterIt = find_if(m_monsterCatalog.begin(), m_monsterCatalog.end(),
                    [&](const unique_ptr<Monster>& monsterPtr) { return monsterPtr->getName() == encounterName; });
                if (monsterIt != m_monsterCatalog.end() && doesEncounterGrantKey(**monsterIt))
                {
                    return objective + tr(" to claim the regional key.", " pour obtenir la cle regionale.");
                }
                return objective + ".";
            }
        }
    }

    return tr("Face the ultimate shadow and finish the journey.", "Affronte l'ombre ultime et termine le voyage.");
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
    cout << tr("Mood: ", "Humeur : ") << getMonsterMoodText(monster) << "\n";
    cout << tr("Hint: ", "Indice : ") << getBattleHint(monster) << "\n";
    cout << tr("Physique: ", "Physique : ") << getMonsterPhysicalProfile(monster) << "\n";
    cout << tr("Hero style: ", "Style du heros : ") << getHeroAppearanceLabel(m_player.getAppearanceId())
         << " | " << getHeroAppearanceBonusText() << "\n";

    if (m_nextFightBonus > 0 || m_nextActBonus > 0 || m_guardCharges > 0 || m_nextFightPenalty > 0 || m_nextActPenalty > 0)
    {
        cout << tr("Active bonuses:", "Bonus actifs :");
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
    cout << tr("Choose a fight style:\n", "Choisis un style d'attaque :\n");
    for (size_t i = 0; i < attackIds.size(); ++i)
    {
        cout << i + 1 << ". " << getAttackStyleLabel(attackIds[i])
             << " - " << getAttackStyleDescription(attackIds[i]) << "\n";
    }
    cout << "0. " << tr("Cancel", "Annuler") << "\n";
    cout << tr("Choice: ", "Choix : ");
    const int attackChoice = readIntChoice(0, static_cast<int>(attackIds.size()));
    if (attackChoice == 0)
    {
        cout << tr("You hold your stance and wait.\n", "Tu gardes ta position et tu attends.\n");
        return BattleTurnResult::NO_TURN_SPENT;
    }

    bool criticalHit = false;
    string effectivenessText;
    string attackLabel;
    const int dealtDamage = resolvePlayerFightAction(monster, attackIds[attackChoice - 1], criticalHit, effectivenessText, attackLabel);
    cout << m_player.getName() << tr(" uses ", " utilise ") << attackLabel
         << tr(" and deals ", " et inflige ") << dealtDamage << tr(" damage.\n", " degats.\n");
    if (criticalHit)
    {
        cout << tr("Critical hit!\n", "Coup critique !\n");
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
        cout << tr("This monster has no ACT options configured yet.\n",
                   "Ce monstre n'a encore aucune option ACT configuree.\n");
        return BattleTurnResult::NO_TURN_SPENT;
    }

    cout << tr("Available ACT actions:\n", "Actions ACT disponibles :\n");
    for (size_t i = 0; i < actIds.size(); ++i)
    {
        const ActAction& action = m_actCatalog.at(actIds[i]);
        cout << i + 1 << ". " << action.getId()
             << " (" << action.getMercyImpact() << " " << tr("mercy", "mercy") << ") - "
             << getLocalizedActText(action) << "\n";
    }
    cout << "0. " << tr("Cancel", "Annuler") << "\n";
    cout << tr("Choose an ACT: ", "Choisis un ACT : ");
    const int actChoice = readIntChoice(0, static_cast<int>(actIds.size()));
    if (actChoice == 0)
    {
        cout << tr("You keep thinking.\n", "Tu continues de reflechir.\n");
        return BattleTurnResult::NO_TURN_SPENT;
    }

    const ActAction& selectedAction = m_actCatalog.at(actIds[actChoice - 1]);
    const int previousMercy = monster.getMercy();
    monster.addMercy(computeActMercyImpact(monster, selectedAction, actUsage));
    ++actUsage[selectedAction.getId()];
    cout << getLocalizedActText(selectedAction) << "\n";
    displayActOutcome(monster, selectedAction, previousMercy);
    if (m_nextActBonus > 0)
    {
        cout << tr("Your prepared social item boost has been consumed.\n",
                   "Le bonus social prepare par ton objet a ete consomme.\n");
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

    cout << "0. " << tr("Cancel", "Annuler") << "\n";
    cout << tr("Choose an item: ", "Choisis un objet : ");
    const int itemChoice = readIntChoice(0, static_cast<int>(m_player.getInventory().size()));
    if (itemChoice == 0)
    {
        cout << tr("You put the item back in your bag.\n",
                   "Tu ranges l'objet dans ton sac.\n");
        return BattleTurnResult::NO_TURN_SPENT;
    }

    const Item& item = m_player.getInventory()[static_cast<size_t>(itemChoice - 1)];
    const string itemName = item.getName();
    if (m_player.useItem(static_cast<size_t>(itemChoice - 1), cout))
    {
        applyItemBattleEffect(item);
        cout << tr("Tactical effect: ", "Effet tactique : ") << getItemTacticalEffect(itemName) << "\n";
    }
    return BattleTurnResult::CONTINUE;
}

Game::BattleTurnResult Game::handleMercyAction(Monster& monster)
{
    if (monster.isSpareable())
    {
        return BattleTurnResult::PLAYER_WON_SPARE;
    }

    cout << monster.getName() << tr(" is not ready to be spared yet.\n",
                                     " n'est pas encore pret a etre epargne.\n");
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
    else if (monster.getName() == "Miresting")
    {
        const bool bindTurn = randomInt(1, 100) <= 28;
        if (!bindTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 1) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        m_nextFightPenalty = max(m_nextFightPenalty, bindTurn ? 3 : 2);
        cout << "Wet reeds slow your stance. Your next FIGHT will lose some edge.\n";
        m_lastMonsterTurnSummary += bindTurn ? " Tangling reeds heavily weaken your next FIGHT." : " Reeds weaken your next FIGHT.";
    }
    else if (monster.getName() == "BogSlime")
    {
        const bool oozeTurn = randomInt(1, 100) <= 34;
        if (!oozeTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 3, monster.getAtk() + 1) * attackMultiplier)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        monster.heal(oozeTurn ? 5 : 2);
        if (oozeTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 2);
            cout << "BogSlime regathers itself from the mire and leaves your next FIGHT slightly bogged down.\n";
            m_lastMonsterTurnSummary += " It reforms and slightly slows your next FIGHT.";
        }
    }
    else if (monster.getName() == "Mudscale")
    {
        const bool mireCrashTurn = randomInt(1, 100) <= 38;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 1, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? 1.25f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (mireCrashTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 4);
            cout << "Mudscale churns the ground into mire. Your next FIGHT will be much slower.\n";
            m_lastMonsterTurnSummary += " Thick mire heavily weakens your next FIGHT.";
        }
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
    else if (monster.getName() == "Shardback")
    {
        const bool braceTurn = randomInt(1, 100) <= 32;
        if (!braceTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        monster.heal(braceTurn ? 6 : 3);
        cout << "Shardback locks its plates in place and restores a little structure.\n";
        m_lastMonsterTurnSummary += braceTurn ? " reinforces its crystal shell and restores HP." : " Its shell recovers a little HP.";
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
    else if (monster.getName() == "CinderHex")
    {
        const bool hexTurn = randomInt(1, 100) <= 35;
        if (!hexTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? 1.4f : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        m_nextActPenalty = max(m_nextActPenalty, hexTurn ? 6 : 4);
        cout << "The ember curse disturbs your rhythm. Your next ACT will be weaker.\n";
        m_lastMonsterTurnSummary += hexTurn ? " brands your focus with a heavy curse." : " leaves an ember curse on your next ACT.";
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
    else if (monster.getName() == "AshDrake")
    {
        const bool flameDiveTurn = randomInt(1, 100) <= 36;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? 1.35f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (flameDiveTurn)
        {
            m_nextActPenalty = max(m_nextActPenalty, 3);
            cout << "AshDrake sweeps overhead in a burst of embers. Your next ACT loses clarity.\n";
            m_lastMonsterTurnSummary += " Ember wake weakens your next ACT.";
        }
    }
    else if (monster.getName() == "TideWarden")
    {
        const bool surgeTurn = randomInt(1, 100) <= 34;
        if (!surgeTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? kCriticalMultiplier : 1.f)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        monster.heal(surgeTurn ? 8 : 4);
        cout << "TideWarden draws water around its armor and restores HP.\n";
        m_lastMonsterTurnSummary += surgeTurn ? " calls a strong restorative tide." : " restores a little HP with flowing pressure.";
    }
    else if (monster.getName() == "AegisSlime")
    {
        const bool fortifyTurn = randomInt(1, 100) <= 34;
        if (!fortifyTurn)
        {
            const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 2, monster.getAtk() + 1) * attackMultiplier)) - mercySoftening - guardReduction;
            totalDamage = m_player.takeDamage(rawMonsterDamage);
            cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
            m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        }
        monster.heal(fortifyTurn ? 7 : 3);
        if (fortifyTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 2);
            cout << "AegisSlime hardens its runes and braces for impact.\n";
            m_lastMonsterTurnSummary += " Its runes fortify and restore HP.";
        }
    }
    else if (monster.getName() == "RuneDemon")
    {
        const bool curseTurn = randomInt(1, 100) <= 36;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 1, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? 1.3f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (curseTurn)
        {
            m_nextActPenalty = max(m_nextActPenalty, 4);
            cout << "RuneDemon brands the air with a curse. Your next ACT is weakened.\n";
            m_lastMonsterTurnSummary += " A curse weakens your next ACT.";
        }
    }
    else if (monster.getName() == "MedusaPrime")
    {
        const bool petrifyTurn = randomInt(1, 100) <= 35;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 1, monster.getAtk() + 2) * attackMultiplier) * (criticalHit ? 1.25f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (petrifyTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 3);
            cout << "MedusaPrime locks onto your stance. Your next FIGHT stiffens under the pressure.\n";
            m_lastMonsterTurnSummary += " Petrifying pressure weakens your next FIGHT.";
        }
    }
    else if (monster.getName() == "Solaraith")
    {
        const bool flareTurn = randomInt(1, 100) <= 45;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 1, monster.getAtk() + 3) * attackMultiplier) * (criticalHit ? 1.45f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (flareTurn)
        {
            m_nextFightPenalty = max(m_nextFightPenalty, 4);
            m_nextActPenalty = max(m_nextActPenalty, 4);
            cout << "A solar flare overwhelms the arena. Your next FIGHT and ACT are both weaker.\n";
            m_lastMonsterTurnSummary += " A solar flare weakens both your next FIGHT and ACT.";
        }
    }
    else if (monster.getName() == "NullSaint")
    {
        const bool collapseTurn = randomInt(1, 100) <= 42;
        const int rawMonsterDamage = static_cast<int>((randomInt(monster.getAtk() - 1, monster.getAtk() + 3) * attackMultiplier) * (criticalHit ? 1.45f : 1.f)) - mercySoftening - guardReduction;
        totalDamage = m_player.takeDamage(rawMonsterDamage);
        cout << attackText << " Its " << attackType << " attack deals " << totalDamage << " damage.\n";
        m_lastMonsterTurnSummary += " deals " + to_string(totalDamage) + " damage.";
        if (collapseTurn && monster.getMercy() < monster.getMercyGoal())
        {
            monster.addMercy(-8);
            cout << "The void contracts around the battlefield. Mercy falls.\n";
            m_lastMonsterTurnSummary += " A void collapse lowers mercy.";
        }
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
        cout << monster.getName() << tr(" was defeated.\n", " a ete vaincu.\n");
        m_player.recordKill();
        m_player.recordVictory();
        m_clearedEncounters.insert(monster.getName());
        recordBattleResult(monster, "Killed");
        grantBattleReward(monster, result);
        if (doesEncounterGrantKey(monster) && !hasRegionKey(getMonsterRegionName(monster)))
        {
            m_regionKeys.insert(getKeyNameForRegion(getMonsterRegionName(monster)));
            cout << tr("You recovered a regional key. A new land opens ahead.\n",
                       "Tu as recupere une cle regionale. Une nouvelle contree s'ouvre devant toi.\n");
        }
        checkProgressMilestones();
        cout << tr("Campaign progress: ", "Progression de campagne : ")
             << getRegionKeyCount() << tr("/4 keys secured.\n", "/4 cles securisees.\n");
        break;
    case BattleTurnResult::PLAYER_WON_SPARE:
        cout << tr("You spare ", "Tu epargnes ") << monster.getName() << ".\n";
        m_player.recordSpare();
        m_player.recordVictory();
        m_clearedEncounters.insert(monster.getName());
        recordBattleResult(monster, "Spared");
        grantBattleReward(monster, result);
        if (doesEncounterGrantKey(monster) && !hasRegionKey(getMonsterRegionName(monster)))
        {
            m_regionKeys.insert(getKeyNameForRegion(getMonsterRegionName(monster)));
            cout << tr("Mercy still earns the key. The next land is now unlocked.\n",
                       "La compassion permet aussi d'obtenir la cle. La prochaine contree est maintenant debloquee.\n");
        }
        checkProgressMilestones();
        cout << tr("Campaign progress: ", "Progression de campagne : ")
             << getRegionKeyCount() << tr("/4 keys secured.\n", "/4 cles securisees.\n");
        break;
    case BattleTurnResult::PLAYER_FLED:
        cout << tr("You retreat for now. No victory is counted.\n",
                   "Tu bats en retraite pour l'instant. Aucune victoire n'est comptee.\n");
        break;
    case BattleTurnResult::PLAYER_LOST:
        cout << m_player.getName()
             << tr(" has fallen. Restoring HP for the prototype build.\n",
                   " est tombe. Les HP sont restaures pour le prototype.\n");
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
        return tr("Mercy is high enough: you can spare this monster now.",
                  "La mercy est assez haute : tu peux epargner ce monstre maintenant.");
    }

    const int mercy = monster.getMercy();
    const int goal = monster.getMercyGoal();
    if (mercy == 0)
    {
        return tr("The monster is still hostile.", "Le monstre reste hostile.");
    }
    if (mercy * 2 < goal)
    {
        return tr("Your ACT choices are starting to have an effect.",
                  "Tes choix ACT commencent a produire un effet.");
    }
    return tr("The monster is hesitating. Mercy is getting close to the goal.",
              "Le monstre hesite. La mercy approche du seuil.");
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
    if (name == "BogSlime")
    {
        return tr("BogSlime quivers inside a shallow pool, uncertain whether to hide or engulf you.",
                  "BogSlime tremble dans une flaque peu profonde, sans savoir s'il doit se cacher ou t'engloutir.");
    }
    if (name == "Froggit")
    {
        return tr("Froggit hops forward, unsure whether this is a duel or a conversation.",
                  "Froggit s'avance en sautillant, sans savoir s'il s'agit d'un duel ou d'une conversation.");
    }
    if (name == "MimicBox")
    {
        return tr("MimicBox rattles nervously. Something inside it wants to be understood.",
                  "MimicBox tremble nerveusement. Quelque chose en lui aimerait etre compris.");
    }
    if (name == "QueenByte")
    {
        return tr("QueenByte descends with theatrical confidence and expects a worthy performance.",
                  "QueenByte descend avec une assurance theatrale et attend une prestation digne d'elle.");
    }
    if (name == "Dustling")
    {
        return tr("Dustling spins in jittery circles, as if it cannot decide whether to flee or fight.",
                  "Dustling tourne en cercles nerveux, incapable de choisir entre fuite et combat.");
    }
    if (name == "Miresting")
    {
        return tr("Miresting emerges from the reeds, watching every step with patient suspicion.",
                  "Miresting sort des roseaux et observe chacun de tes pas avec une suspicion patiente.");
    }
    if (name == "Mudscale")
    {
        return tr("Mudscale drags a heavy tail through the mire and claims the whole marsh as its throne.",
                  "Mudscale traine une lourde queue dans la vase et revendique tout le marais comme son trone.");
    }
    if (name == "GlimmerMoth")
    {
        return tr("GlimmerMoth scatters shimmering dust and watches you with curious caution.",
                  "GlimmerMoth disperse une poussiere brillante et t'observe avec une curiosite prudente.");
    }
    if (name == "Pebblimp")
    {
        return tr("Pebblimp bounces in place, trying very hard to look tougher than it feels.",
                  "Pebblimp rebondit sur place en essayant tres fort d'avoir l'air plus coriace qu'il ne l'est.");
    }
    if (name == "Shardback")
    {
        return tr("Shardback digs into the dune glass, bracing like a creature that trusts its armor more than anyone else.",
                  "Shardback s'ancre dans le verre des dunes, comme une creature qui fait davantage confiance a son armure qu'au reste du monde.");
    }
    if (name == "HowlScreen")
    {
        return tr("HowlScreen crackles to life, projecting static like a warning siren.",
                  "HowlScreen grésille brusquement et projette une statique semblable a une sirene d'alerte.");
    }
    if (name == "CinderHex")
    {
        return tr("CinderHex burns through the haze in flickering bursts, as if every motion is a ritual.",
                  "CinderHex fend la brume par salves vacillantes, comme si chacun de ses gestes relevait d'un rituel.");
    }
    if (name == "BloomCobra")
    {
        return tr("BloomCobra rises from the sand, beautiful and dangerous in equal measure.",
                  "BloomCobra surgit du sable, aussi belle que dangereuse.");
    }
    if (name == "AshDrake")
    {
        return tr("AshDrake darts through broken antennas and leaves ember trails in the storm-lit air.",
                  "AshDrake fend les antennes brisees et laisse des traces de braises dans l'air electrique.");
    }
    if (name == "AshDrake")
    {
        return tr("A young drake born from hot circuitry and ash gusts. Fast, bright and temperamental.",
                  "Un jeune drake ne des circuits brulants et des rafales de cendre. Rapide, brillant et capricieux.");
    }
    if (name == "AshDrake")
    {
        return tr("A young wasteland drake that tests intruders with rapid dives and hot, unstable bursts.",
                  "Un jeune drake des friches qui teste les intrus avec des plongeons rapides et des salves ardentes instables.");
    }
    if (name == "AegisSlime")
    {
        return tr("A vault-bred guardian slime covered in old runes. It is slow, defensive and difficult to unsettle.",
                  "Un slime gardien de crypte couvert de runes anciennes. Il est lent, defensif et difficile a destabiliser.");
    }
    if (name == "RuneDemon")
    {
        return tr("A demon bound in archive sigils that mixes brute force with curses and pressure.",
                  "Un demon lie a des sceaux d'archives qui melange force brute, maledictions et pression constante.");
    }
    if (name == "MedusaPrime")
    {
        return tr("A prime serpentine sentinel whose calm gaze hides petrifying control and ruthless precision.",
                  "Une sentinelle serpentine majeure dont le regard calme cache une maitrise petrifiante et une precision implacable.");
    }
    if (name == "TideWarden")
    {
        return tr("TideWarden steps forward with the patience of a floodgate that has waited centuries to open.",
                  "TideWarden avance avec la patience d'une ecluse qui attend depuis des siecles de s'ouvrir.");
    }
    if (name == "AegisSlime")
    {
        return tr("AegisSlime ripples around an ancient seal and turns every impact into patient resistance.",
                  "AegisSlime ondule autour d'un sceau ancien et transforme chaque impact en resistance patiente.");
    }
    if (name == "RuneDemon")
    {
        return tr("RuneDemon rises from a cracked ward circle, trailing sparks of forbidden script.",
                  "RuneDemon s'eleve d'un cercle de garde fissure, suivi d'etincelles d'ecriture interdite.");
    }
    if (name == "MedusaPrime")
    {
        return tr("MedusaPrime coils across the archive floor, waiting for one careless look to end the duel.",
                  "MedusaPrime se love sur le sol des archives, attendant qu'un seul regard imprudent mette fin au duel.");
    }
    if (name == "AegisSlime")
    {
        return tr("A vault-bred slime that hardens around runic seals and buys time for the archive defenses.",
                  "Un slime de crypte qui se durcit autour des sceaux runiques et gagne du temps pour les defenses des archives.");
    }
    if (name == "RuneDemon")
    {
        return tr("A disciplined fiend that weaponizes curses and pressure instead of reckless rage.",
                  "Un demon discipline qui transforme maledictions et pression en armes plutot qu'en rage aveugle.");
    }
    if (name == "MedusaPrime")
    {
        return tr("An archive predator whose gaze and venom can lock a battle in place if you lose focus.",
                  "Une predatrice des archives dont le regard et le venin peuvent figer tout un combat si tu perds ta concentration.");
    }
    if (name == "Solaraith")
    {
        return tr("Solaraith descends in a crown of fire, turning the wasteland into a stage of living embers.",
                  "Solaraith descend dans une couronne de feu, transformant la friche en scene de braises vivantes.");
    }
    if (name == "Archivore")
    {
        return tr("Archivore opens ancient metal wings, as if testing whether you deserve to be remembered.",
                  "Archivore deploie d'anciennes ailes de metal, comme pour juger si tu merites d'etre retenu.");
    }
    if (name == "NullSaint")
    {
        return tr("NullSaint arrives in total silence, and the air itself seems to retreat from the altar of its shadow.",
                  "NullSaint arrive dans un silence total, et l'air lui-meme semble reculer devant l'autel de son ombre.");
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return tr("A cautious foe stands in your way.", "Un ennemi prudent te barre la route.");
    case MonsterCategory::MINIBOSS:
        return tr("A tougher opponent studies your every move.", "Un adversaire plus coriace etudie chacun de tes mouvements.");
    case MonsterCategory::BOSS:
        return tr("The air grows heavy. This battle will demand more than raw force.",
                  "L'air devient lourd. Ce combat demandera plus que de la force brute.");
    }

    return tr("A monster challenges you.", "Un monstre te defie.");
}

string Game::getBattleHint(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime")
    {
        return tr("Small kindness and careful observation work better than force here.", "De petits gestes bienveillants et l'observation marchent mieux que la force.");
    }
    if (name == "Froggit")
    {
        return tr("Kindness and calm discussion seem promising.", "La gentillesse et une discussion calme semblent prometteuses.");
    }
    if (name == "MimicBox")
    {
        return tr("Observation and a gentle approach may open it up.", "L'observation et une approche douce pourraient l'ouvrir.");
    }
    if (name == "QueenByte")
    {
        return tr("She seems to appreciate wit, reason and spectacle.", "Elle semble apprecier l'esprit, la raison et le spectacle.");
    }
    if (name == "Dustling")
    {
        return tr("It looks restless. Humor might work better than pressure.", "Il semble agite. L'humour marchera sans doute mieux que la pression.");
    }
    if (name == "Miresting")
    {
        return tr("It reacts better to patience than to sudden aggression.", "Il reagit mieux a la patience qu'a l'agression soudaine.");
    }
    if (name == "Mudscale")
    {
        return tr("Respect its territory first. Calm study opens it up faster than bravado.", "Respecte d'abord son territoire. Une observation calme l'ouvre plus vite que la bravade.");
    }
    if (name == "GlimmerMoth")
    {
        return tr("Careful observation and gentle praise might keep it from panicking.",
                  "Une observation attentive et quelques eloges pourraient l'empecher de paniquer.");
    }
    if (name == "Pebblimp")
    {
        return tr("It seems easier to calm than to frighten.", "Il semble plus facile a apaiser qu'a effrayer.");
    }
    if (name == "Shardback")
    {
        return tr("Observation and practical kindness may work better than trying to overpower the shell.",
                  "L'observation et une gentillesse concrete marcheront mieux que de tenter de briser sa carapace.");
    }
    if (name == "HowlScreen")
    {
        return tr("Reason and calm dialogue may cut through the noise.", "La raison et un dialogue calme peuvent traverser le bruit.");
    }
    if (name == "CinderHex")
    {
        return tr("It seems drawn to confidence, rhythm and composed words.", "Il semble attire par l'assurance, le rythme et les paroles posees.");
    }
    if (name == "BloomCobra")
    {
        return tr("A respectful approach should work better than threats.", "Une approche respectueuse devrait mieux fonctionner que les menaces.");
    }
    if (name == "AshDrake")
    {
        return tr("It likes boldness, but not mockery. Rhythm and warmth may steady it.", "Il aime l'audace, mais pas la moquerie. Le rythme et la chaleur peuvent l'apaiser.");
    }
    if (name == "TideWarden")
    {
        return tr("It values restraint, insight and offerings made without panic.",
                  "Il valorise la retenue, la lucidité et les offrandes faites sans panique.");
    }
    if (name == "Solaraith")
    {
        return tr("Spectacle alone is not enough. It wants conviction and control.",
                  "Le spectacle seul ne suffit pas. Il veut de la conviction et du controle.");
    }
    if (name == "Archivore")
    {
        return tr("It values understanding and thoughtful words over brute force.",
                  "Il valorise la comprehension et les mots bien choisis plutot que la force brute.");
    }
    if (name == "AegisSlime")
    {
        return tr("It yields when you stop forcing the issue. Gentle offerings help.", "Il cede quand on arrete de forcer. Les offrandes douces aident.");
    }
    if (name == "RuneDemon")
    {
        return tr("Clear reasoning keeps its curses from snowballing.", "Un raisonnement net evite que ses maledictions ne s'emballent.");
    }
    if (name == "MedusaPrime")
    {
        return tr("Do not provoke it. Careful observation and composed dialogue are safest.", "Ne la provoque pas. L'observation et le dialogue pose sont les plus surs.");
    }
    if (name == "NullSaint")
    {
        return tr("Careful observation and disciplined reasoning seem safer than any show of force.",
                  "Une observation precise et un raisonnement discipline semblent plus surs que toute demonstration de force.");
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return tr("Try positive ACT choices before fighting.", "Essaie d'abord des ACT positifs avant d'attaquer.");
    case MonsterCategory::MINIBOSS:
        return tr("You may need a stronger sequence of ACT choices.", "Tu auras sans doute besoin d'une meilleure sequence d'ACT.");
    case MonsterCategory::BOSS:
        return tr("Bosses resist mercy, but they usually have favorite interactions.",
                  "Les boss resistent a la mercy, mais ils ont souvent des interactions favorites.");
    }

    return tr("Watch the mercy bar and adapt your strategy.", "Observe la barre de mercy et adapte ta strategie.");
}

string Game::getMonsterDescription(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime")
    {
        return tr("A marsh slime that mirrors the mood around it. Kindness makes it settle quickly.",
                  "Un slime des marais qui reflÃ¨te l'humeur autour de lui. La douceur le calme vite.");
    }
    if (name == "Froggit")
    {
        return tr("A nervous marsh wanderer that responds best to kindness and patient conversation.",
                  "Un voyageur des marais nerveux qui repond surtout a la gentillesse et a une conversation patiente.");
    }
    if (name == "MimicBox")
    {
        return tr("A suspicious living chest whose hostility fades once it feels understood or fed.",
                  "Un coffre vivant suspicieux dont l'hostilite baisse lorsqu'il se sent compris ou nourri.");
    }
    if (name == "QueenByte")
    {
        return tr("A theatrical ruler of code who respects wit, logic and dramatic flair.",
                  "Une souveraine theatrale du code qui respecte l'esprit, la logique et le sens de la mise en scene.");
    }
    if (name == "Dustling")
    {
        return tr("A restless sand spirit. Humor and empathy calm it far better than intimidation.",
                  "Un esprit de sable agite. L'humour et l'empathie l'apaisent bien mieux que l'intimidation.");
    }
    if (name == "Miresting")
    {
        return tr("A reed-dwelling hunter that softens when treated with patience and steady attention.",
                  "Un chasseur vivant dans les roseaux qui s'adoucit face a la patience et a une attention stable.");
    }
    if (name == "Mudscale")
    {
        return tr("A lizard boss of the mire. Tough scales, heavy strikes and a surprisingly watchful mind.",
                  "Un boss lezard du marais. Ecailles solides, coups lourds et esprit etonnamment attentif.");
    }
    if (name == "GlimmerMoth")
    {
        return tr("A fragile luminous creature that reacts well to observation and gentle praise.",
                  "Une creature lumineuse fragile qui repond bien a l'observation et aux eloges delicats.");
    }
    if (name == "Pebblimp")
    {
        return tr("A bluffing stone puffball that is easier to reassure than to scare.",
                  "Une petite boule de pierre fanfaronne qu'il est plus simple de rassurer que d'effrayer.");
    }
    if (name == "Shardback")
    {
        return tr("A dune-burrowing crystal beast that trusts careful study more than loud bravado.",
                  "Une bete de cristal fouisseuse qui fait davantage confiance a l'observation attentive qu'aux grandes bravades.");
    }
    if (name == "HowlScreen")
    {
        return tr("A noisy signal-being whose static weakens when someone speaks with clarity.",
                  "Un etre de signal bruyant dont la statique faiblit lorsque quelqu'un parle avec clarte.");
    }
    if (name == "CinderHex")
    {
        return tr("A ritual ember spirit that respects presence, rhythm and words delivered without fear.",
                  "Un esprit de braise rituel qui respecte la presence, le rythme et les paroles prononcees sans peur.");
    }
    if (name == "BloomCobra")
    {
        return tr("A proud desert serpent-flower that dislikes threats but respects thoughtful gestures.",
                  "Un fier serpent-fleur du desert qui deteste les menaces mais respecte les gestes bien pensés.");
    }
    if (name == "TideWarden")
    {
        return tr("A vault sentinel of flowing armor that answers best to measured insight and calm offerings.",
                  "Une sentinelle de crypte a l'armure fluide qui repond surtout a la lucidité et aux offrandes calmes.");
    }
    if (name == "Solaraith")
    {
        return tr("A high sovereign of living flame that tests whether your resolve can survive its radiance.",
                  "Une haute souveraine de flamme vivante qui teste si ta determination peut survivre a son eclat.");
    }
    if (name == "Archivore")
    {
        return tr("An ancient keeper of records that values insight, reason and measured dialogue.",
                  "Un ancien gardien des archives qui valorise la lucidité, la raison et le dialogue mesure.");
    }
    if (name == "NullSaint")
    {
        return tr("A void sanctifier that judges weakness harshly but responds to lucid, disciplined intent.",
                  "Un sanctificateur du vide qui juge durement la faiblesse mais repond a une intention lucide et disciplinee.");
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return tr("A common foe with a simple emotional pattern.", "Un ennemi courant au schema emotionnel assez simple.");
    case MonsterCategory::MINIBOSS:
        return tr("A sturdier opponent that requires a more deliberate approach.",
                  "Un adversaire plus robuste qui demande une approche plus deliberate.");
    case MonsterCategory::BOSS:
        return tr("A major enemy with high pressure and stronger resistance to mercy.",
                  "Un ennemi majeur, plus oppressant et plus resistant a la mercy.");
    }

    return tr("An unusual creature from ALTERDUNE.", "Une creature inhabituelle d'ALTERDUNE.");
}

string Game::getMonsterRegionName(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime" || name == "Froggit" || name == "Dustling" || name == "Miresting" || name == "Mudscale")
    {
        return "Sunken Mire";
    }
    if (name == "MimicBox" || name == "GlimmerMoth" || name == "Pebblimp" || name == "Shardback" || name == "QueenByte")
    {
        return "Glass Dunes";
    }
    if (name == "HowlScreen" || name == "BloomCobra" || name == "AshDrake" || name == "CinderHex" || name == "Solaraith")
    {
        return "Signal Wastes";
    }
    if (name == "AegisSlime" || name == "TideWarden" || name == "RuneDemon" || name == "MedusaPrime" || name == "Archivore")
    {
        return "Ancient Vault";
    }
    if (name == "NullSaint")
    {
        return "Unknown Frontier";
    }

    return "Unknown Frontier";
}

string Game::getMonsterPhysicalProfile(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime")
    {
        return tr("Soft marsh body - elastic and messy", "Corps marecageux souple - elastique et salissant");
    }
    if (name == "Froggit")
    {
        return tr("Light amphibian - slippery and reactive", "Amphibien leger - glissant et reactif");
    }
    if (name == "MimicBox")
    {
        return tr("Heavy armored shell - strong resistance to frontal pressure", "Coque lourde blindee - forte resistance a la pression frontale");
    }
    if (name == "QueenByte")
    {
        return tr("Regal pulse construct - elegant but dangerous", "Construct regal d'impulsion - elegant mais dangereux");
    }
    if (name == "Dustling")
    {
        return tr("Fast drifting spirit - unstable and evasive", "Esprit derive rapide - instable et evasif");
    }
    if (name == "Mudscale")
    {
        return tr("Dense scaled predator - grounded, heavy and hard to stagger", "Predateur ecaille dense - ancre, lourd et difficile a ebranler");
    }
    if (name == "GlimmerMoth")
    {
        return tr("Fragile aerial body - luminous and elusive", "Corps aerien fragile - lumineux et insaisissable");
    }
    if (name == "Pebblimp")
    {
        return tr("Compact rocky body - bouncy and stubborn", "Corps rocheux compact - rebondissant et tenace");
    }
    if (name == "Shardback")
    {
        return tr("Layered crystal shell - sturdy, heavy and difficult to stagger", "Coque de cristal stratifiee - solide, lourde et difficile a ebranler");
    }
    if (name == "HowlScreen")
    {
        return tr("Angular signal frame - volatile static emitter", "Structure de signal angulaire - emetteur statique instable");
    }
    if (name == "CinderHex")
    {
        return tr("Smoldering wraith frame - unstable core wrapped in sparks", "Silhouette spectrale incandescente - noyau instable entoure d'etincelles");
    }
    if (name == "BloomCobra")
    {
        return tr("Flexible serpent vine - graceful and venomous", "Liane serpentine souple - gracieuse et venimeuse");
    }
    if (name == "AshDrake")
    {
        return tr("Young flying drake - quick bursts and hot breath", "Jeune drake volant - salves rapides et souffle ardent");
    }
    if (name == "TideWarden")
    {
        return tr("Massive tidal armor - slow, steady and fortified by flowing pressure", "Armure de maree massive - lente, stable et renforcee par la pression fluide");
    }
    if (name == "AegisSlime")
    {
        return tr("Runic gel mass - slow, compressive and stubborn", "Masse gelatineuse runique - lente, compressive et obstinee");
    }
    if (name == "RuneDemon")
    {
        return tr("Infernal rune frame - muscular, cursed and relentless", "Silhouette infernale runique - puissante, maudite et tenace");
    }
    if (name == "MedusaPrime")
    {
        return tr("Serpentine guardian body - elegant, petrifying and lethal at mid-range", "Corps de gardienne serpentine - elegant, petrifiant et mortel a moyenne portee");
    }
    if (name == "Solaraith")
    {
        return tr("Radiant flame sovereign - blazing wings and relentless presence", "Souverainete de flamme radieuse - ailes ardentes et presence implacable");
    }
    if (name == "Archivore")
    {
        return tr("Ancient metal avian - dense and disciplined", "Oiseau metallique ancien - dense et discipline");
    }
    if (name == "NullSaint")
    {
        return tr("Void-forged sentinel - cold symmetry with oppressive gravity", "Sentinelle forgee dans le vide - symetrie froide et gravite oppressante");
    }

    return tr("Unknown physique", "Physique inconnu");
}

string Game::getHeroAppearanceBonusText() const
{
    const string& appearanceId = m_player.getAppearanceId();
    if (appearanceId == "vanguard")
    {
        return tr("Vanguard bonus: better BLADE pressure, slightly steadier against heavy attacks.",
                  "Bonus Avant-garde : meilleure pression BLADE et meilleure tenue face aux attaques lourdes.");
    }
    if (appearanceId == "mystic")
    {
        return tr("Mystic bonus: stronger PULSE affinity and calmer ACT rhythm.",
                  "Bonus Mystique : meilleure affinite PULSE et rythme ACT plus stable.");
    }

    return tr("Wanderer bonus: balanced style with slightly safer DUST handling.",
              "Bonus Voyageur : style equilibre avec une meilleure maitrise de DUST.");
}

string Game::getMonsterMoodText(const Monster& monster) const
{
    if (monster.isSpareable())
    {
        return tr("The monster is ready to give up the fight.", "Le monstre est pret a abandonner le combat.");
    }

    const int mercy = monster.getMercy();
    const int mercyGoal = monster.getMercyGoal();
    const int hp = monster.getHp();
    const int maxHp = monster.getMaxHp();

    if (hp * 3 <= maxHp)
    {
        return tr("It looks shaken and much less confident than before.", "Il semble ebranle et beaucoup moins sur de lui.");
    }
    if (mercy * 3 >= mercyGoal * 2)
    {
        return tr("Its guard is dropping. A final kind gesture might be enough.", "Sa garde tombe. Un dernier geste bienveillant pourrait suffire.");
    }
    if (mercy > 0)
    {
        return tr("It is still tense, but your actions are getting through.", "Il reste tendu, mais tes actions commencent a l'atteindre.");
    }
    return tr("It is focused entirely on the fight.", "Il est entierement concentre sur le combat.");
}

string Game::getMonsterAttackText(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime")
    {
        return tr("BogSlime splashes corrosive mire in quick bursts.",
                  "BogSlime projette de la vase corrosive par salves rapides.");
    }
    if (name == "Froggit")
    {
        return tr("Froggit splashes muddy water around the arena.",
                  "Froggit projette de l'eau boueuse dans toute l'arene.");
    }
    if (name == "MimicBox")
    {
        return tr("MimicBox snaps open and throws sharp debris.",
                  "MimicBox s'ouvre brusquement et projette des eclats coupants.");
    }
    if (name == "QueenByte")
    {
        return tr("QueenByte floods the battlefield with royal code strikes.",
                  "QueenByte inonde le champ de bataille de frappes de code royal.");
    }
    if (name == "Dustling")
    {
        return tr("Dustling whirls into a stinging sand spiral.",
                  "Dustling tourbillonne en une spirale de sable mordante.");
    }
    if (name == "Mudscale")
    {
        return tr("Mudscale crashes forward with a swamp-heavy tail slam.",
                  "Mudscale charge avec un violent balayage de queue charge de boue.");
    }
    if (name == "GlimmerMoth")
    {
        return tr("GlimmerMoth sends a haze of sparkling scales drifting toward you.",
                  "GlimmerMoth envoie vers toi un nuage d'ecailles scintillantes.");
    }
    if (name == "Pebblimp")
    {
        return tr("Pebblimp hurls itself forward in a flurry of clumsy stone bumps.",
                  "Pebblimp se jette en avant dans une rafale de chocs pierreux maladroits.");
    }
    if (name == "Shardback")
    {
        return tr("Shardback launches a grinding rush of glass plates and crystal sparks.",
                  "Shardback lance une charge de plaques de verre et d'etincelles de cristal.");
    }
    if (name == "HowlScreen")
    {
        return tr("HowlScreen blasts the arena with waves of buzzing static.",
                  "HowlScreen bombarde l'arene d'ondes de statique bourdonnante.");
    }
    if (name == "CinderHex")
    {
        return tr("CinderHex traces a burning sigil that erupts into searing arcs.",
                  "CinderHex trace un sceau ardent qui explose en arcs brulants.");
    }
    if (name == "BloomCobra")
    {
        return tr("BloomCobra lashes out with thorned vines and a sudden strike.",
                  "BloomCobra attaque avec des lianes epineuses et une frappe soudaine.");
    }
    if (name == "AshDrake")
    {
        return tr("AshDrake dives and spits a ribbon of ember fire.",
                  "AshDrake plonge et crache un ruban de feu cendreux.");
    }
    if (name == "TideWarden")
    {
        return tr("TideWarden drives a crushing wave of pressure straight through the arena.",
                  "TideWarden propulse une vague de pression ecrasante a travers l'arene.");
    }
    if (name == "AegisSlime")
    {
        return tr("AegisSlime rebounds with a dense runic slam.",
                  "AegisSlime rebondit avec un choc runique tres dense.");
    }
    if (name == "RuneDemon")
    {
        return tr("RuneDemon tears the air with a cursed infernal swipe.",
                  "RuneDemon dechire l'air d'un revers infernal maudit.");
    }
    if (name == "MedusaPrime")
    {
        return tr("MedusaPrime lashes with venomous coils and a petrifying glare.",
                  "MedusaPrime frappe avec ses anneaux venimeux et un regard petrifiant.");
    }
    if (name == "Solaraith")
    {
        return tr("Solaraith calls down a radiant flare that devours the air around you.",
                  "Solaraith invoque une flare radieuse qui devore l'air autour de toi.");
    }
    if (name == "Archivore")
    {
        return tr("Archivore summons razor-sharp pages of light around the battlefield.",
                  "Archivore invoque autour du champ de bataille des pages de lumiere tranchees.");
    }
    if (name == "NullSaint")
    {
        return tr("NullSaint folds the arena into a cold eclipse and strikes from the dark seam.",
                  "NullSaint replie l'arene en eclipse glaciale et frappe depuis la faille obscure.");
    }

    switch (monster.getCategory())
    {
    case MonsterCategory::NORMAL:
        return monster.getName() + tr(" lashes out quickly.", " frappe rapidement.");
    case MonsterCategory::MINIBOSS:
        return monster.getName() + tr(" launches a heavier attack.", " lance une attaque plus lourde.");
    case MonsterCategory::BOSS:
        return monster.getName() + tr(" unleashes a brutal combo.", " dechaine un combo brutal.");
    }

    return monster.getName() + tr(" attacks.", " attaque.");
}

string Game::getMonsterAttackType(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "Froggit" || name == "BloomCobra" || name == "BogSlime" || name == "MedusaPrime")
    {
        return "Venom";
    }
    if (name == "Dustling" || name == "Pebblimp" || name == "Mudscale")
    {
        return "Dust";
    }
    if (name == "MimicBox" || name == "Archivore" || name == "Shardback" || name == "AegisSlime")
    {
        return "Metal";
    }
    if (name == "QueenByte" || name == "HowlScreen" || name == "CinderHex" || name == "AshDrake")
    {
        return "Pulse";
    }
    if (name == "GlimmerMoth" || name == "Solaraith")
    {
        return "Light";
    }
    if (name == "Miresting")
    {
        return "Nature";
    }
    if (name == "TideWarden")
    {
        return "Tide";
    }
    if (name == "RuneDemon")
    {
        return "Void";
    }
    if (name == "NullSaint")
    {
        return "Void";
    }

    return "Strike";
}

string Game::getMonsterElementType(const Monster& monster) const
{
    const string& name = monster.getName();
    if (name == "BogSlime")
    {
        return "Nature";
    }
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
    if (name == "Miresting")
    {
        return "Nature";
    }
    if (name == "GlimmerMoth")
    {
        return "Light";
    }
    if (name == "Pebblimp")
    {
        return "Earth";
    }
    if (name == "Shardback")
    {
        return "Metal";
    }
    if (name == "HowlScreen")
    {
        return "Storm";
    }
    if (name == "AshDrake")
    {
        return "Fire";
    }
    if (name == "CinderHex")
    {
        return "Fire";
    }
    if (name == "BloomCobra")
    {
        return "Nature";
    }
    if (name == "AegisSlime")
    {
        return "Metal";
    }
    if (name == "TideWarden")
    {
        return "Water";
    }
    if (name == "RuneDemon")
    {
        return "Shadow";
    }
    if (name == "MedusaPrime")
    {
        return "Nature";
    }
    if (name == "Solaraith")
    {
        return "Light";
    }
    if (name == "Archivore")
    {
        return "Shadow";
    }
    if (name == "NullSaint")
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

string Game::getLocalizedActText(const ActAction& action) const
{
    const string& id = action.getId();
    if (id == "JOKE")
    {
        return tr("You crack a weird desert joke.", "Tu lances une blague etrange du desert.");
    }
    if (id == "COMPLIMENT")
    {
        return tr("You compliment the monster's style.", "Tu complimentes le style du monstre.");
    }
    if (id == "DISCUSS")
    {
        return tr("You try to calmly discuss the situation.", "Tu essaies de discuter calmement de la situation.");
    }
    if (id == "OBSERVE")
    {
        return tr("You observe carefully and learn a weakness.", "Tu observes attentivement et reperes une faiblesse.");
    }
    if (id == "PET")
    {
        return tr("You offer a friendly gesture.", "Tu proposes un geste amical.");
    }
    if (id == "OFFER_SNACK")
    {
        return tr("You offer a snack from your bag.", "Tu offres un snack de ton sac.");
    }
    if (id == "REASON")
    {
        return tr("You appeal to reason and empathy.", "Tu fais appel a la raison et a l'empathie.");
    }
    if (id == "DANCE")
    {
        return tr("You start an awkward but sincere dance.", "Tu commences une danse maladroite mais sincere.");
    }
    if (id == "INSULT")
    {
        return tr("You provoke the monster.", "Tu provoques le monstre.");
    }
    if (id == "THREATEN")
    {
        return tr("You make the tension worse.", "Tu rends la tension encore pire.");
    }

    return action.getText();
}

string Game::localizeRegionName(const string& regionName) const
{
    if (regionName == "Sunken Mire")
    {
        return tr("Sunken Mire", "Mare Engloutie");
    }
    if (regionName == "Glass Dunes")
    {
        return tr("Glass Dunes", "Dunes de Verre");
    }
    if (regionName == "Signal Wastes")
    {
        return tr("Signal Wastes", "Friches de Signal");
    }
    if (regionName == "Ancient Vault")
    {
        return tr("Ancient Vault", "Crypte Ancienne");
    }
    if (regionName == "Unknown Frontier")
    {
        return tr("Final Gate", "Porte Finale");
    }

    return regionName;
}

string Game::localizeElementName(const string& elementName) const
{
    if (elementName == "Water") return tr("Water", "Eau");
    if (elementName == "Metal") return tr("Metal", "Metal");
    if (elementName == "Fire") return tr("Fire", "Feu");
    if (elementName == "Shadow") return tr("Shadow", "Ombre");
    if (elementName == "Nature") return tr("Nature", "Nature");
    if (elementName == "Light") return tr("Light", "Lumiere");
    if (elementName == "Earth") return tr("Earth", "Terre");
    if (elementName == "Storm") return tr("Storm", "Tempete");
    if (elementName == "Arcane") return tr("Arcane", "Arcane");
    if (elementName == "Neutral") return tr("Neutral", "Neutre");
    return elementName;
}

string Game::localizeAttackTypeName(const string& attackType) const
{
    if (attackType == "Venom") return tr("Venom", "Venin");
    if (attackType == "Dust") return tr("Dust", "Poussiere");
    if (attackType == "Metal") return tr("Metal", "Metal");
    if (attackType == "Pulse") return tr("Pulse", "Impulsion");
    if (attackType == "Light") return tr("Light", "Lumiere");
    if (attackType == "Nature") return tr("Nature", "Nature");
    if (attackType == "Tide") return tr("Tide", "Maree");
    if (attackType == "Void") return tr("Void", "Neant");
    if (attackType == "Strike") return tr("Strike", "Impact");
    return attackType;
}

string Game::getElementIcon(const string& elementName) const
{
    if (elementName == "Fire") return "F";
    if (elementName == "Water") return "W";
    if (elementName == "Shadow") return "S";
    if (elementName == "Light") return "L";
    if (elementName == "Nature") return "N";
    if (elementName == "Metal") return "M";
    if (elementName == "Storm") return "T";
    if (elementName == "Earth") return "E";
    if (elementName == "Arcane") return "A";
    if (elementName == "Void") return "V";
    return "*";
}

string Game::getMonsterRewardHint(const Monster& monster) const
{
    if (monster.getName() == "NullSaint")
    {
        return tr("Final battle of the run", "Combat final de la partie");
    }
    if (doesEncounterGrantKey(monster))
    {
        return tr("Regional key + stronger rewards", "Cle regionale + meilleures recompenses");
    }
    if (monster.getCategory() == MonsterCategory::BOSS)
    {
        return tr("Major loot + rare blessing chance", "Gros butin + chance de benediction rare");
    }
    if (monster.getCategory() == MonsterCategory::MINIBOSS)
    {
        return tr("Good loot + strong item chance", "Bon butin + chance d'objet puissant");
    }

    return tr("Basic loot + healing supplies", "Butin simple + soin");
}

string Game::localizeCategoryName(MonsterCategory category) const
{
    switch (category)
    {
    case MonsterCategory::NORMAL:
        return tr("NORMAL", "NORMAL");
    case MonsterCategory::MINIBOSS:
        return tr("MINIBOSS", "MINIBOSS");
    case MonsterCategory::BOSS:
        return tr("BOSS", "BOSS");
    }

    return "UNKNOWN";
}

string Game::localizeThreatName(const string& threat) const
{
    if (threat == "Low") return tr("Low", "Faible");
    if (threat == "Medium") return tr("Medium", "Moyenne");
    if (threat == "High") return tr("High", "Elevee");
    return threat;
}

bool Game::isFrench() const
{
    return m_languageCode == "fr";
}

string Game::tr(const string& englishText, const string& frenchText) const
{
    return isFrench() ? frenchText : englishText;
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
        return tr("BLADE", "LAME");
    }
    if (attackStyleId == "pulse")
    {
        return "PULSE";
    }
    if (attackStyleId == "dust")
    {
        return tr("DUST", "POUSSIERE");
    }
    if (attackStyleId == "ember")
    {
        return tr("EMBER", "BRAISE");
    }
    if (attackStyleId == "tide")
    {
        return tr("TIDE", "MAREE");
    }

    return tr("STRIKE", "FRAPPE");
}

string Game::getAttackStyleDescription(const string& attackStyleId) const
{
    if (attackStyleId == "blade")
    {
        return tr("balanced cut", "coupe equilibree");
    }
    if (attackStyleId == "pulse")
    {
        return tr("arcane burst", "rafale arcane");
    }
    if (attackStyleId == "dust")
    {
        return tr("wild unstable hit", "impact instable");
    }
    if (attackStyleId == "ember")
    {
        return tr("burning arc", "arc brulant");
    }
    if (attackStyleId == "tide")
    {
        return tr("flowing wave", "vague fluide");
    }

    return tr("basic hit", "frappe simple");
}

vector<string> Game::getAvailableAttackStyleIds() const
{
    vector<string> attacks = {"blade", "pulse"};
    if (getRegionKeyCount() >= 1 || m_player.getVictories() >= 2)
    {
        attacks.push_back("dust");
    }
    if (getRegionKeyCount() >= 2 || m_player.getVictories() >= 4)
    {
        attacks.push_back("ember");
    }
    if (getRegionKeyCount() >= 3 || m_player.getVictories() >= 7)
    {
        attacks.push_back("tide");
    }
    return attacks;
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
        effectivenessText = tr("It is especially effective against ", "C'est tres efficace contre ")
            + localizeElementName(monsterElement) + ".";
    }
    else if (styleMultiplier <= 0.85f)
    {
        effectivenessText = localizeElementName(monsterElement) + tr(" resists your ", " resiste a ton attaque ")
            + localizeElementName(attackElement) + ".";
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
        return tr("small heal + guard for the next enemy turn", "petit soin + garde pour le prochain tour ennemi");
    }
    if (itemName == "Snack")
    {
        return tr("small heal + ACT boost on your next social action", "petit soin + bonus ACT sur ta prochaine action sociale");
    }
    if (itemName == "Potion")
    {
        return tr("solid heal + small boost to your next attack", "bon soin + petit bonus sur ta prochaine attaque");
    }
    if (itemName == "GlowCandy")
    {
        return tr("medium heal + strong ACT boost", "soin moyen + fort bonus ACT");
    }
    if (itemName == "CactusJuice")
    {
        return tr("strong heal + strong Fight boost", "gros soin + fort bonus Attaque");
    }
    if (itemName == "SuperPotion")
    {
        return tr("big heal + guard and a small Fight boost", "gros soin + garde + petit bonus Attaque");
    }
    if (itemName == "Elixir")
    {
        return tr("strong heal + clears combat penalties + balanced boost", "gros soin + annule les malus de combat + bonus equilibre");
    }
    if (itemName == "AegisTonic")
    {
        return tr("medium heal + double guard for the next enemy turns", "soin moyen + double garde pour les prochains tours ennemis");
    }
    if (itemName == "PhoenixDust")
    {
        return tr("strong heal + major Fight burst on your next attack", "gros soin + gros burst Attaque sur ta prochaine frappe");
    }

    return tr("restores HP", "rend des PV");
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
    else if (name == "Miresting")
    {
        if (actId == "DISCUSS")
        {
            impact += 18;
        }
        else if (actId == "PET")
        {
            impact += 14;
        }
        else if (actId == "OBSERVE")
        {
            impact += 10;
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
    else if (name == "Shardback")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 15;
        }
        else if (actId == "DISCUSS")
        {
            impact += 10;
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
    else if (name == "CinderHex")
    {
        if (actId == "REASON")
        {
            impact += 14;
        }
        else if (actId == "DANCE")
        {
            impact += 19;
        }
        else if (actId == "COMPLIMENT")
        {
            impact += 12;
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
    else if (name == "BogSlime")
    {
        if (actId == "OBSERVE")
        {
            impact += 14;
        }
        else if (actId == "COMPLIMENT")
        {
            impact += 18;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 10;
        }
    }
    else if (name == "Mudscale")
    {
        if (actId == "OBSERVE")
        {
            impact += 16;
        }
        else if (actId == "DISCUSS")
        {
            impact += 13;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 18;
        }
    }
    else if (name == "AshDrake")
    {
        if (actId == "JOKE")
        {
            impact += 11;
        }
        else if (actId == "DISCUSS")
        {
            impact += 16;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 20;
        }
    }
    else if (name == "AegisSlime")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "DISCUSS")
        {
            impact += 12;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 15;
        }
    }
    else if (name == "RuneDemon")
    {
        if (actId == "OBSERVE")
        {
            impact += 15;
        }
        else if (actId == "REASON")
        {
            impact += 18;
        }
    }
    else if (name == "MedusaPrime")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "DISCUSS")
        {
            impact += 14;
        }
        else if (actId == "PET")
        {
            impact += 9;
        }
    }
    else if (name == "TideWarden")
    {
        if (actId == "OBSERVE")
        {
            impact += 12;
        }
        else if (actId == "REASON")
        {
            impact += 16;
        }
        else if (actId == "OFFER_SNACK")
        {
            impact += 18;
        }
    }
    else if (name == "Solaraith")
    {
        if (actId == "REASON")
        {
            impact += 17;
        }
        else if (actId == "DANCE")
        {
            impact += 14;
        }
        else if (actId == "COMPLIMENT")
        {
            impact += 19;
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
    else if (name == "NullSaint")
    {
        if (actId == "OBSERVE")
        {
            impact += 18;
        }
        else if (actId == "DISCUSS")
        {
            impact += 14;
        }
        else if (actId == "REASON")
        {
            impact += 17;
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
            return tr("QueenByte narrows her eyes. She clearly did not appreciate that.",
                      "QueenByte plisse les yeux. Elle n'a clairement pas apprecie.");
        }
        if (name == "Dustling")
        {
            return tr("Dustling becomes even more erratic and lashes out with nervous energy.",
                      "Dustling devient encore plus erratique et reagit avec une energie nerveuse.");
        }
        if (name == "Solaraith")
        {
            return tr("Solaraith brightens into a punishing glare. Vanity was the wrong angle.",
                      "Solaraith s'embrase d'un regard punitif. La vanite etait un tres mauvais angle.");
        }
        if (name == "BloomCobra")
        {
            return tr("BloomCobra flares its petals wide. That was clearly the wrong move.",
                      "BloomCobra deploye ses petales. C'etait clairement le mauvais choix.");
        }
        if (name == "Archivore")
        {
            return tr("Archivore marks your words as a failure and grows colder.",
                      "Archivore classe tes paroles comme un echec et devient plus froid.");
        }
        if (name == "NullSaint")
        {
            return tr("NullSaint absorbs the failure in silence and the whole arena grows colder.",
                      "NullSaint absorbe l'echec en silence et toute l'arene se refroidit.");
        }
        return tr("The atmosphere hardens instead of calming down.",
                  "L'atmosphere se durcit au lieu de s'apaiser.");
    }

    if (name == "Froggit")
    {
        if (actId == "COMPLIMENT")
        {
            return tr("Froggit blushes. Compliments seem surprisingly powerful here.",
                      "Froggit rougit. Les compliments semblent etrangement efficaces ici.");
        }
        if (actId == "DISCUSS")
        {
            return tr("Froggit slows down and listens, one croak at a time.",
                      "Froggit ralentit et ecoute, croassement apres croassement.");
        }
    }
    else if (name == "MimicBox")
    {
        if (actId == "OBSERVE")
        {
            return tr("MimicBox clicks open slightly. Being understood makes it less hostile.",
                      "MimicBox s'entrouvre legerement. Se sentir compris le rend moins hostile.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("A pleased rustle comes from inside the box. Food was an excellent idea.",
                      "Un bruissement satisfait vient de l'interieur. Offrir de la nourriture etait une excellente idee.");
        }
        if (actId == "PET")
        {
            return tr("The wooden lid settles down. Somehow, that worked.",
                      "Le couvercle en bois se calme. Contre toute attente, ca a marche.");
        }
    }
    else if (name == "QueenByte")
    {
        if (actId == "REASON")
        {
            return tr("QueenByte pauses to consider your argument. Logic reaches her.",
                      "QueenByte s'arrete pour considerer ton argument. La logique l'atteint.");
        }
        if (actId == "DANCE")
        {
            return tr("QueenByte cannot help but admire the dramatic commitment.",
                      "QueenByte ne peut s'empecher d'admirer cet engagement dramatique.");
        }
        if (actId == "JOKE")
        {
            return tr("A tiny smile escapes QueenByte before she regains her composure.",
                      "Un minuscule sourire echappe a QueenByte avant qu'elle ne se reprenne.");
        }
    }
    else if (name == "Dustling")
    {
        if (actId == "JOKE")
        {
            return tr("Dustling loses its rhythm for a second, almost like it laughed.",
                      "Dustling perd son rythme une seconde, presque comme s'il avait ri.");
        }
        if (actId == "OBSERVE")
        {
            return tr("Dustling seems relieved that someone noticed its unease.",
                      "Dustling semble soulage que quelqu'un ait remarque son malaise.");
        }
    }
    else if (name == "Miresting")
    {
        if (actId == "DISCUSS")
        {
            return tr("Miresting settles into the reeds and listens instead of lunging.",
                      "Miresting se pose dans les roseaux et ecoute au lieu d'attaquer.");
        }
        if (actId == "PET")
        {
            return tr("A cautious trill escapes Miresting. It did not expect gentleness here.",
                      "Un petit trille prudent echappe a Miresting. Il ne s'attendait pas a tant de douceur ici.");
        }
    }
    else if (name == "GlimmerMoth")
    {
        if (actId == "OBSERVE")
        {
            return tr("GlimmerMoth slows its wings, as if it is happy to be seen instead of hunted.",
                      "GlimmerMoth ralentit ses ailes, comme s'il etait heureux d'etre observe plutot que chasse.");
        }
        if (actId == "COMPLIMENT")
        {
            return tr("The shimmering patterns brighten. GlimmerMoth clearly loves the attention.",
                      "Les motifs scintillants s'illuminent. GlimmerMoth adore clairement l'attention.");
        }
    }
    else if (name == "Pebblimp")
    {
        if (actId == "DISCUSS")
        {
            return tr("Pebblimp settles down and listens, trying to act serious about it.",
                      "Pebblimp se calme et ecoute, en essayant d'avoir l'air tres serieux.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("Pebblimp forgets to posture for a moment and happily accepts the snack.",
                      "Pebblimp oublie sa posture un instant et accepte le snack avec plaisir.");
        }
    }
    else if (name == "Shardback")
    {
        if (actId == "OBSERVE")
        {
            return tr("Shardback notices that you respected its armor instead of mocking it.",
                      "Shardback remarque que tu respectes son armure au lieu de t'en moquer.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("The crystal beast calms down, crunching the gift with surprising care.",
                      "La bete de cristal se calme et croque le present avec un soin surprenant.");
        }
    }
    else if (name == "HowlScreen")
    {
        if (actId == "REASON")
        {
            return tr("The static clears a little. Your words are getting through the noise.",
                      "La statique se dissipe un peu. Tes paroles traversent enfin le bruit.");
        }
        if (actId == "DISCUSS")
        {
            return tr("HowlScreen lowers its volume, curious about where this conversation is going.",
                      "HowlScreen baisse le volume, curieux de voir ou mene cette conversation.");
        }
    }
    else if (name == "CinderHex")
    {
        if (actId == "REASON")
        {
            return tr("The flames tighten into a cleaner shape. CinderHex respects your composure.",
                      "Les flammes se resserrent en une forme plus nette. CinderHex respecte ton sang-froid.");
        }
        if (actId == "DANCE")
        {
            return tr("CinderHex mirrors the rhythm in a ring of embers. That clearly reached it.",
                      "CinderHex imite le rythme dans un cercle de braises. Cela l'a clairement touche.");
        }
    }
    else if (name == "BloomCobra")
    {
        if (actId == "PET")
        {
            return tr("BloomCobra sways in place, surprised by the calm confidence of that gesture.",
                      "BloomCobra se balance sur place, surpris par l'assurance calme de ce geste.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("A soft rustle replaces the hiss. BloomCobra responds well to thoughtful offerings.",
                      "Un doux bruissement remplace le sifflement. BloomCobra reagit bien aux attentions delicates.");
        }
    }
    else if (name == "BogSlime")
    {
        if (actId == "COMPLIMENT")
        {
            return tr("BogSlime jiggles happily. It clearly likes being treated as more than swamp sludge.",
                      "BogSlime tremblote de joie. Il aime visiblement qu'on le traite comme autre chose qu'une simple flaque.");
        }
        if (actId == "OBSERVE")
        {
            return tr("BogSlime slows down and mirrors your breathing. Observation helps it settle.",
                      "BogSlime ralentit et calque sa respiration sur la tienne. L'observation l'apaise.");
        }
    }
    else if (name == "Mudscale")
    {
        if (actId == "OBSERVE")
        {
            return tr("Mudscale notices that you studied its stance instead of charging blindly.",
                      "Mudscale remarque que tu as etudie sa posture au lieu de charger sans reflechir.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("Mudscale lowers its massive head and accepts the offering with wary respect.",
                      "Mudscale baisse sa lourde tete et accepte l'offrande avec un respect mefiant.");
        }
    }
    else if (name == "AshDrake")
    {
        if (actId == "DISCUSS")
        {
            return tr("AshDrake circles once, then listens. It wanted a challenge, not chaos.",
                      "AshDrake effectue un cercle puis ecoute. Il cherchait un defi, pas le chaos.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("The young drake forgets to posture for a moment and lands to inspect the snack.",
                      "Le jeune drake oublie sa posture un instant et se pose pour examiner le snack.");
        }
    }
    else if (name == "AegisSlime")
    {
        if (actId == "OBSERVE")
        {
            return tr("The runes dim for a moment. AegisSlime responds to careful pattern reading.",
                      "Les runes s'attenuent un instant. AegisSlime reagit a une lecture attentive de ses motifs.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("AegisSlime absorbs the gift and relaxes, as if ritual protocol has been respected.",
                      "AegisSlime absorbe l'offrande et se detend, comme si un vieux protocole venait d'etre respecte.");
        }
    }
    else if (name == "RuneDemon")
    {
        if (actId == "REASON")
        {
            return tr("RuneDemon hesitates. It was not expecting anyone to answer its violence with conviction.",
                      "RuneDemon hesite. Il ne s'attendait pas a voir quelqu'un repondre a sa violence avec autant de conviction.");
        }
        if (actId == "OBSERVE")
        {
            return tr("The sigils flicker. RuneDemon realizes you noticed the curse beneath the rage.",
                      "Les sceaux vacillent. RuneDemon comprend que tu as vu la malediction sous sa rage.");
        }
    }
    else if (name == "MedusaPrime")
    {
        if (actId == "OBSERVE")
        {
            return tr("MedusaPrime stills her coils. Precision recognizes precision.",
                      "MedusaPrime immobilise ses anneaux. La precision reconnait la precision.");
        }
        if (actId == "DISCUSS")
        {
            return tr("Her gaze softens by a fraction. MedusaPrime respects calm control.",
                      "Son regard s'adoucit d'une fraction. MedusaPrime respecte la maitrise calme.");
        }
    }
    else if (name == "TideWarden")
    {
        if (actId == "REASON")
        {
            return tr("TideWarden lowers its crest and seems willing to weigh your judgment.",
                      "TideWarden baisse sa crete et semble pret a peser ton jugement.");
        }
        if (actId == "OFFER_SNACK")
        {
            return tr("The waters around TideWarden grow calmer. It accepts the gesture without pride.",
                      "Les eaux autour de TideWarden s'apaisent. Il accepte le geste sans orgueil.");
        }
    }
    else if (name == "Solaraith")
    {
        if (actId == "COMPLIMENT")
        {
            return tr("Solaraith holds the blaze steady, clearly pleased that you saw the discipline behind the spectacle.",
                      "Solaraith maintient sa flamme stable, visiblement satisfait que tu aies percu la discipline derriere le spectacle.");
        }
        if (actId == "REASON")
        {
            return tr("For a moment, Solaraith burns with focus instead of rage. Your words reached the core.",
                      "Un instant, Solaraith brule de concentration plutot que de rage. Tes mots ont touche son coeur.");
        }
    }
    else if (name == "Archivore")
    {
        if (actId == "OBSERVE")
        {
            return tr("Archivore notices your attention to detail and stops treating you like a nuisance.",
                      "Archivore remarque ton sens du detail et cesse de te traiter comme une nuisance.");
        }
        if (actId == "REASON")
        {
            return tr("Archivore weighs your logic carefully, like a judge reviewing an old record.",
                      "Archivore pese ta logique avec soin, comme un juge qui etudie un ancien dossier.");
        }
        if (actId == "DISCUSS")
        {
            return tr("Archivore folds its wings slightly. It seems willing to continue the exchange.",
                      "Archivore replie legerement ses ailes. Il semble pret a poursuivre l'echange.");
        }
    }
    else if (name == "NullSaint")
    {
        if (actId == "OBSERVE")
        {
            return tr("NullSaint recognizes the discipline of your attention and eases its crushing pressure.",
                      "NullSaint reconnait la discipline de ton regard et relache un peu sa pression ecrasante.");
        }
        if (actId == "REASON")
        {
            return tr("A faint fracture appears in the void around NullSaint. Logic still matters here.",
                      "Une legere fissure apparait dans le vide autour de NullSaint. La logique compte encore ici.");
        }
    }

    if (delta >= 35)
    {
        return tr("That choice lands extremely well and changes the flow of the battle.",
                  "Ce choix fonctionne extremement bien et change le cours du combat.");
    }
    if (delta >= 20)
    {
        return tr("The monster reacts well. You have found a promising approach.",
                  "Le monstre reagit bien. Tu as trouve une approche prometteuse.");
    }
    return tr("The monster settles down a little.",
              "Le monstre se calme un peu.");
}

void Game::displayActOutcome(const Monster& monster, const ActAction& action, int previousMercy) const
{
    const int delta = monster.getMercy() - previousMercy;
    if (delta > 0)
    {
        cout << tr("Mercy increases by ", "La mercy augmente de ") << delta << ". ";
    }
    else if (delta < 0)
    {
        cout << tr("Mercy decreases by ", "La mercy baisse de ") << -delta << ". ";
    }
    else
    {
        cout << tr("Mercy stays the same. ", "La mercy reste stable. ");
    }

    if (action.getMercyImpact() < 0)
    {
        cout << getActReactionText(monster, action, delta) << "\n";
    }
    else if (monster.isSpareable())
    {
        cout << getActReactionText(monster, action, delta)
             << tr(" The monster is now ready to be spared.\n",
                   " Le monstre peut maintenant etre epargne.\n");
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
        cout << tr("After the battle, ", "Apres le combat, ") << m_player.getName()
             << tr(" recovers ", " recupere ")
             << recoveredHp << " HP.\n";
    }
    grantRandomItemReward(monster, result);
    tryGrantRareBattleBlessing(monster);
}

void Game::grantRandomItemReward(const Monster& monster, BattleTurnResult result)
{
    vector<Item> rewardPool;
    const string regionName = getMonsterRegionName(monster);

    if (regionName == "Sunken Mire")
    {
        rewardPool.push_back(Item("Bandage", ItemType::HEAL, 5, 1));
        rewardPool.push_back(Item("Snack", ItemType::HEAL, 8, 1));
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
        if (monster.getCategory() != MonsterCategory::NORMAL)
        {
            rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        }
    }
    else if (regionName == "Glass Dunes")
    {
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
        if (monster.getCategory() != MonsterCategory::NORMAL)
        {
            rewardPool.push_back(Item("Elixir", ItemType::HEAL, 24, 1));
        }
    }
    else if (regionName == "Signal Wastes")
    {
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
        rewardPool.push_back(Item("SuperPotion", ItemType::HEAL, 30, 1));
        rewardPool.push_back(Item("Elixir", ItemType::HEAL, 24, 1));
        if (monster.getCategory() != MonsterCategory::NORMAL)
        {
            rewardPool.push_back(Item("PhoenixDust", ItemType::HEAL, 28, 1));
        }
    }
    else
    {
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
        rewardPool.push_back(Item("SuperPotion", ItemType::HEAL, 30, 1));
        rewardPool.push_back(Item("Elixir", ItemType::HEAL, 24, 1));
        rewardPool.push_back(Item("AegisTonic", ItemType::HEAL, 18, 1));
        rewardPool.push_back(Item("PhoenixDust", ItemType::HEAL, 28, 1));
    }

    if (result == BattleTurnResult::PLAYER_WON_SPARE)
    {
        rewardPool.push_back(Item("Snack", ItemType::HEAL, 8, 1));
        rewardPool.push_back(Item("GlowCandy", ItemType::HEAL, 12, 1));
        if (monster.getCategory() != MonsterCategory::NORMAL)
        {
            rewardPool.push_back(Item("AegisTonic", ItemType::HEAL, 18, 1));
        }
    }
    else if (result == BattleTurnResult::PLAYER_WON_KILL)
    {
        rewardPool.push_back(Item("Potion", ItemType::HEAL, 15, 1));
        rewardPool.push_back(Item("CactusJuice", ItemType::HEAL, 20, 1));
        if (monster.getCategory() != MonsterCategory::NORMAL)
        {
            rewardPool.push_back(Item("PhoenixDust", ItemType::HEAL, 28, 1));
        }
    }

    const int randomIndex = randomInt(0, static_cast<int>(rewardPool.size()) - 1);
    const Item reward = rewardPool[static_cast<size_t>(randomIndex)];
    m_player.addItem(reward);
    cout << tr("Reward obtained: ", "Recompense obtenue : ") << reward.getName() << " x1.\n";
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
        cout << tr("Rare blessing: your journey hardens you. Max HP +5.\n",
                   "Benediction rare : ton voyage t'endurcit. Max HP +5.\n");
    }
    else if (blessingType == 2)
    {
        m_player.setAtk(m_player.getAtk() + 1);
        cout << tr("Rare blessing: your strikes become sharper. ATK +1.\n",
                   "Benediction rare : tes coups deviennent plus precis. ATK +1.\n");
    }
    else
    {
        m_player.setDef(m_player.getDef() + 1);
        cout << tr("Rare blessing: your stance becomes steadier. DEF +1.\n",
                   "Benediction rare : ta posture devient plus stable. DEF +1.\n");
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
    else if (itemName == "Elixir")
    {
        m_nextFightPenalty = 0;
        m_nextActPenalty = 0;
        m_nextFightBonus += 2;
        m_nextActBonus += 4;
    }
    else if (itemName == "AegisTonic")
    {
        m_guardCharges += 2;
    }
    else if (itemName == "PhoenixDust")
    {
        m_nextFightBonus += 6;
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
    if (!m_hpMilestoneUnlocked && (m_player.getVictories() >= 3 || getRegionKeyCount() >= 1))
    {
        m_hpMilestoneUnlocked = true;
        m_player.setMaxHp(m_player.getMaxHp() + 10);
        m_player.heal(10);
        cout << tr("Milestone reached: your resilience grows. Max HP +10.\n",
                   "Palier atteint : ton endurance augmente. Max HP +10.\n");
    }

    if (!m_atkMilestoneUnlocked && (m_player.getVictories() >= 6 || getRegionKeyCount() >= 2))
    {
        m_atkMilestoneUnlocked = true;
        m_player.setAtk(m_player.getAtk() + 2);
        cout << tr("Milestone reached: your confidence sharpens your attacks. ATK +2.\n",
                   "Palier atteint : ta confiance renforce tes attaques. ATK +2.\n");
    }

    if (!m_defMilestoneUnlocked && (m_player.getVictories() >= 9 || getRegionKeyCount() >= 3))
    {
        m_defMilestoneUnlocked = true;
        m_player.setDef(m_player.getDef() + 1);
        cout << tr("Milestone reached: you learn to brace for danger. DEF +1.\n",
                   "Palier atteint : tu apprends a encaisser le danger. DEF +1.\n");
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
    data.regionName = localizeRegionName(getMonsterRegionName(monster));
    data.openingText = getBattleOpeningText(monster);
    data.moodText = getMonsterMoodText(monster);
    data.hintText = getBattleHint(monster);
    data.battleLogText = battleLogText;
    data.playerActionText = m_lastPlayerTurnSummary;
    data.monsterActionText = m_lastMonsterTurnSummary;

      data.activeBonusesText = tr("No active bonuses", "Aucun bonus actif");
      if (m_nextFightBonus > 0 || m_nextActBonus > 0 || m_guardCharges > 0 || m_nextFightPenalty > 0 || m_nextActPenalty > 0)
      {
          data.activeBonusesText = tr("Bonuses:", "Bonus :");
        if (m_nextFightBonus > 0)
        {
            data.activeBonusesText += tr(" Fight +", " Attaque +") + to_string(m_nextFightBonus);
        }
        if (m_nextActBonus > 0)
        {
            data.activeBonusesText += " ACT +" + to_string(m_nextActBonus);
        }
          if (m_guardCharges > 0)
          {
              data.activeBonusesText += tr(" Guard x", " Garde x") + to_string(m_guardCharges);
          }
          if (m_nextFightPenalty > 0)
          {
              data.activeBonusesText += tr(" Fight -", " Attaque -") + to_string(m_nextFightPenalty);
          }
          if (m_nextActPenalty > 0)
          {
              data.activeBonusesText += " ACT -" + to_string(m_nextActPenalty);
          }
    }

    data.playerName = m_player.getName();
    data.monsterName = monster.getName();
    data.monsterCategory = localizeCategoryName(monster.getCategory());
    data.monsterElementType = localizeElementName(getMonsterElementType(monster));
    data.monsterElementIcon = getElementIcon(getMonsterElementType(monster));
    data.monsterPhysique = getMonsterPhysicalProfile(monster);
    data.monsterDescription = getMonsterDescription(monster);
    data.playerHpBar = {"HP", m_player.getHp(), m_player.getMaxHp()};
    data.monsterHpBar = {"HP", monster.getHp(), monster.getMaxHp()};
    data.mercyBar = {tr("MERCY", "PITIE"), monster.getMercy(), monster.getMercyGoal()};
    data.primaryActions = {
        {"fight", "!", tr("FIGHT", "ATTAQUE"), tr("Choose a learned style", "Choisir un style appris"), true},
        {"act", "A", "ACT", tr("Raise mercy with context", "Faire monter la pitie"), !monster.getAvailableActIds().empty()},
        {"item", "I", tr("ITEM", "OBJET"), tr("Use tactical consumables", "Utiliser un objet tactique"), findFirstUsableInventoryItem() != nullptr},
        {"mercy", "M", tr("MERCY", "PITIE"), tr("Spare when the bar is full", "Epargner quand la barre est pleine"), monster.isSpareable()}
    };
    data.secondaryActions = {
        {"back", "<", tr("BACK", "RETOUR"), tr("Leave this battle", "Quitter ce combat"), true}
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

    cout << "\n=== " << tr("Battle Start", "Debut du combat") << " ===\n";
    cout << tr("A wild ", "Un ") << monster->getName() << tr(" appears.\n", " apparait.\n");
    cout << getBattleOpeningText(*monster) << "\n";

    bool battleEnded = false;
    while (!battleEnded && m_player.isAlive() && monster->isAlive())
    {
        displayBattleState(*monster);

        cout << "1. " << tr("Fight", "Attaque") << "\n";
        cout << "2. ACT\n";
        cout << "3. " << tr("Item", "Objet") << "\n";
        cout << "4. " << tr("Mercy", "Pitie") << "\n";
        cout << "5. " << tr("Run", "Fuir") << "\n";
        cout << tr("Choice: ", "Choix : ");

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
    return isEncounterCleared("NullSaint");
}

void Game::displayEndingAndExit()
{
    cout << "\n=== " << tr("Final Outcome", "Issue finale") << " ===\n";
    cout << tr("All regional keys have been secured and the final boss has fallen.\n",
               "Toutes les cles regionales ont ete obtenues et le boss final est tombe.\n");
    cout << tr("Victories: ", "Victoires : ") << m_player.getVictories()
         << " | " << tr("Kills: ", "Kills : ") << m_player.getKills()
         << " | " << tr("Spares: ", "Spares : ") << m_player.getSpares() << "\n";

    if (m_player.getKills() == m_player.getVictories())
    {
        cout << tr("Genocidal ending: every won battle ended in violence.\n",
                   "Fin genocidaire : chaque combat gagne s'est termine dans la violence.\n");
    }
    else if (m_player.getSpares() == m_player.getVictories())
    {
        cout << tr("Pacifist ending: every won battle ended with mercy.\n",
                   "Fin pacifiste : chaque combat gagne s'est conclu par la mercy.\n");
    }
    else
    {
        cout << tr("Neutral ending: your journey mixed combat and mercy.\n",
                   "Fin neutre : ton voyage a mele combat et mercy.\n");
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
