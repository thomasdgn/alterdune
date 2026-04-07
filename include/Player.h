#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"
#include "Item.h"

#include <cstddef>
#include <iosfwd>
#include <vector>

class Player : public Entity
{
public:
    Player(const std::string& name = "Traveler", int maxHp = 100, int atk = 12, int def = 3);

    std::string getEntityType() const override;
    void printStatus(std::ostream& os) const override;

    void addItem(const Item& item);
    std::vector<Item>& getInventory();
    const std::vector<Item>& getInventory() const;

    void displayStats(std::ostream& os) const;
    void displayItems(std::ostream& os) const;
    bool useItem(std::size_t index, std::ostream& os);

    int getKills() const;
    int getSpares() const;
    int getVictories() const;

    void recordKill();
    void recordSpare();
    void recordVictory();

private:
    std::vector<Item> m_inventory;
    int m_kills;
    int m_spares;
    int m_victories;
};

#endif
