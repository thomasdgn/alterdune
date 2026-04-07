#ifndef MONSTER_H
#define MONSTER_H

#include "Entity.h"

#include <string>
#include <vector>

enum class MonsterCategory
{
    NORMAL,
    MINIBOSS,
    BOSS
};

class Monster : public Entity
{
public:
    Monster(const std::string& name = "",
            MonsterCategory category = MonsterCategory::NORMAL,
            int maxHp = 0,
            int atk = 0,
            int def = 0,
            int mercyGoal = 100,
            const std::vector<std::string>& actIds = {});

    std::string getEntityType() const override;
    void printStatus(std::ostream& os) const override;

    MonsterCategory getCategory() const;
    int getMercy() const;
    int getMercyGoal() const;
    const std::vector<std::string>& getActIds() const;

    void addMercy(int amount);
    bool isSpareable() const;

    static MonsterCategory categoryFromString(const std::string& value);
    static std::string categoryToString(MonsterCategory category);

private:
    MonsterCategory m_category;
    int m_mercy;
    int m_mercyGoal;
    std::vector<std::string> m_actIds;
};

#endif
