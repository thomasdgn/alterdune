#include "Item.h"

#include <algorithm>

Item::Item(const std::string& name, ItemType type, int value, int quantity)
    : m_name(name), m_type(type), m_value(std::max(0, value)), m_quantity(std::max(0, quantity))
{
}

const std::string& Item::getName() const
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
    m_quantity = std::max(0, quantity);
}

void Item::addQuantity(int amount)
{
    m_quantity = std::max(0, m_quantity + amount);
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

ItemType Item::itemTypeFromString(const std::string& value)
{
    if (value == "HEAL")
    {
        return ItemType::HEAL;
    }

    return ItemType::UNKNOWN;
}

std::string Item::itemTypeToString(ItemType type)
{
    switch (type)
    {
    case ItemType::HEAL:
        return "HEAL";
    default:
        return "UNKNOWN";
    }
}
