#include "Monster.h"

#include <algorithm>
#include <iostream>

Monster::Monster(const std::string& name,
                 MonsterCategory category,
                 int maxHp,
                 int atk,
                 int def,
                 int mercyGoal,
                 const std::vector<std::string>& actIds)
    : Entity(name, maxHp, atk, def),
      m_category(category),
      m_mercy(0),
      m_mercyGoal(std::max(1, mercyGoal)),
      m_actIds(actIds)
{
}

std::string Monster::getEntityType() const
{
    return "Monster";
}

void Monster::printStatus(std::ostream& os) const
{
    os << getName() << " [" << categoryToString(m_category) << "]"
       << " | HP: " << getHp() << "/" << getMaxHp()
       << " | ATK: " << getAtk()
       << " | DEF: " << getDef()
       << " | Mercy: " << m_mercy << "/" << m_mercyGoal;
}

MonsterCategory Monster::getCategory() const
{
    return m_category;
}

int Monster::getMercy() const
{
    return m_mercy;
}

int Monster::getMercyGoal() const
{
    return m_mercyGoal;
}

const std::vector<std::string>& Monster::getActIds() const
{
    return m_actIds;
}

void Monster::addMercy(int amount)
{
    m_mercy = std::clamp(m_mercy + amount, 0, m_mercyGoal);
}

bool Monster::isSpareable() const
{
    return m_mercy >= m_mercyGoal;
}

MonsterCategory Monster::categoryFromString(const std::string& value)
{
    if (value == "NORMAL")
    {
        return MonsterCategory::NORMAL;
    }
    if (value == "MINIBOSS")
    {
        return MonsterCategory::MINIBOSS;
    }
    return MonsterCategory::BOSS;
}

std::string Monster::categoryToString(MonsterCategory category)
{
    switch (category)
    {
    case MonsterCategory::NORMAL:
        return "NORMAL";
    case MonsterCategory::MINIBOSS:
        return "MINIBOSS";
    default:
        return "BOSS";
    }
}
