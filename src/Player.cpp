#include "Player.h"

#include <iostream>

using namespace std;

Player::Player(const string& name, int maxHp, int atk, int def)
    : Entity(name, maxHp, atk, def), m_appearanceId("wanderer"), m_kills(0), m_spares(0), m_victories(0)
{
}

string Player::getEntityType() const
{
    return "Player";
}

void Player::printStatus(ostream& os) const
{
    os << getName() << " | HP: " << getHp() << "/" << getMaxHp()
       << " | ATK: " << getAtk()
       << " | DEF: " << getDef();
}

void Player::addItem(const Item& item)
{
    for (Item& inventoryItem : m_inventory)
    {
        if (inventoryItem.getName() == item.getName() && inventoryItem.getType() == item.getType())
        {
            inventoryItem.addQuantity(item.getQuantity());
            return;
        }
    }

    m_inventory.push_back(item);
}

vector<Item>& Player::getInventory()
{
    return m_inventory;
}

const vector<Item>& Player::getInventory() const
{
    return m_inventory;
}

void Player::displayStats(ostream& os) const
{
    os << "=== Player Stats ===\n";
    printStatus(os);
    os << "\nKills: " << m_kills
       << "\nSpares: " << m_spares
       << "\nVictories: " << m_victories
       << "\nAppearance: " << m_appearanceId
       << "\nInventory slots: " << m_inventory.size() << "\n";
}

void Player::displayItems(ostream& os) const
{
    os << "=== Items ===\n";
    if (m_inventory.empty())
    {
        os << "Inventory is empty.\n";
        return;
    }

    for (size_t i = 0; i < m_inventory.size(); ++i)
    {
        const Item& item = m_inventory[i];
        os << i + 1 << ". " << item.getName()
           << " | Type: " << Item::itemTypeToString(item.getType())
           << " | Value: " << item.getValue()
           << " | Quantity: " << item.getQuantity() << "\n";
    }
}

bool Player::useItem(size_t index, ostream& os)
{
    if (index >= m_inventory.size())
    {
        os << "Invalid item selection.\n";
        return false;
    }

    Item& item = m_inventory[index];
    if (item.getQuantity() <= 0)
    {
        os << item.getName() << " is out of stock.\n";
        return false;
    }

    if (item.getType() == ItemType::HEAL)
    {
        const int healedAmount = heal(item.getValue());
        item.consumeOne();
        os << getName() << " uses " << item.getName() << " and recovers "
           << healedAmount << " HP.\n";
        return true;
    }

    os << "This item type is not implemented yet.\n";
    return false;
}

const string& Player::getAppearanceId() const
{
    return m_appearanceId;
}

void Player::setAppearanceId(const string& appearanceId)
{
    m_appearanceId = appearanceId.empty() ? "wanderer" : appearanceId;
}

int Player::getKills() const
{
    return m_kills;
}

int Player::getSpares() const
{
    return m_spares;
}

int Player::getVictories() const
{
    return m_victories;
}

void Player::recordKill()
{
    ++m_kills;
}

void Player::recordSpare()
{
    ++m_spares;
}

void Player::recordVictory()
{
    ++m_victories;
}
