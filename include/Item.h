#ifndef ITEM_H
#define ITEM_H

#include <string>

enum class ItemType
{
    HEAL,
    UNKNOWN
};

class Item
{
public:
    Item(const std::string& name = "", ItemType type = ItemType::UNKNOWN, int value = 0, int quantity = 0);

    const std::string& getName() const;
    ItemType getType() const;
    int getValue() const;
    int getQuantity() const;

    void setQuantity(int quantity);
    void addQuantity(int amount);
    bool consumeOne();

    static ItemType itemTypeFromString(const std::string& value);
    static std::string itemTypeToString(ItemType type);

private:
    std::string m_name;
    ItemType m_type;
    int m_value;
    int m_quantity;
};

#endif
