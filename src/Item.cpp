#include "Item.h"

#include <algorithm>

using namespace std;

Item::Item(const string& name, ItemType type, int value, int quantity)
    : m_name(name), m_type(type), m_value(max(0, value)), m_quantity(max(0, quantity))
{
}

const string& Item::getName() const
{
    return m_name;
}

ItemType Item::getType() const
{
    return m_type;
}

int Item::getValue() const
{
    return m_value;
}

int Item::getQuantity() const
{
    return m_quantity;
}

void Item::setQuantity(int quantity)
{
    m_quantity = max(0, quantity);
}

void Item::addQuantity(int amount)
{
    m_quantity = max(0, m_quantity + amount);
}

bool Item::consumeOne()
{
    if (m_quantity <= 0)
    {
        return false;
    }

    --m_quantity;
    return true;
}

ItemType Item::itemTypeFromString(const string& value)
{
    if (value == "HEAL")
    {
        return ItemType::HEAL;
    }

    return ItemType::UNKNOWN;
}

string Item::itemTypeToString(ItemType type)
{
    switch (type)
    {
    case ItemType::HEAL:
        return "HEAL";
    default:
        return "UNKNOWN";
    }
}
