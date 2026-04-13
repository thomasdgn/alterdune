#include "Monster.h"

#include <algorithm>
#include <iostream>
#include <memory>

using namespace std;

Monster::Monster(const string& name,
                 int maxHp,
                 int atk,
                 int def,
                 int mercyGoal,
                 const vector<string>& actIds)
    : Entity(name, maxHp, atk, def),
      m_mercy(0),
      m_mercyGoal(max(1, mercyGoal)),
      m_actIds(actIds)
{
}

string Monster::getEntityType() const
{
    return "Monster";
}

void Monster::printStatus(ostream& os) const
{
    os << getName() << " [" << categoryToString(getCategory()) << "]"
       << " | HP: " << getHp() << "/" << getMaxHp()
       << " | ATK: " << getAtk()
       << " | DEF: " << getDef()
       << " | Mercy: " << m_mercy << "/" << m_mercyGoal;
}

int Monster::getMercy() const
{
    return m_mercy;
}

int Monster::getMercyGoal() const
{
    return m_mercyGoal;
}

const vector<string>& Monster::getActIds() const
{
    return m_actIds;
}

vector<string> Monster::getAvailableActIds() const
{
    const size_t actCount = min(static_cast<size_t>(getMaxActChoices()), m_actIds.size());
    return vector<string>(m_actIds.begin(), m_actIds.begin() + actCount);
}

void Monster::addMercy(int amount)
{
    m_mercy = clamp(m_mercy + amount, 0, m_mercyGoal);
}

bool Monster::isSpareable() const
{
    return m_mercy >= m_mercyGoal;
}

MonsterCategory Monster::categoryFromString(const string& value)
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

string Monster::categoryToString(MonsterCategory category)
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

NormalMonster::NormalMonster(const string& name,
                             int maxHp,
                             int atk,
                             int def,
                             int mercyGoal,
                             const vector<string>& actIds)
    : Monster(name, maxHp, atk, def, mercyGoal, actIds)
{
}

MonsterCategory NormalMonster::getCategory() const
{
    return MonsterCategory::NORMAL;
}

int NormalMonster::getMaxActChoices() const
{
    return 2;
}

unique_ptr<Monster> NormalMonster::clone() const
{
    return make_unique<NormalMonster>(*this);
}

MiniBossMonster::MiniBossMonster(const string& name,
                                 int maxHp,
                                 int atk,
                                 int def,
                                 int mercyGoal,
                                 const vector<string>& actIds)
    : Monster(name, maxHp, atk, def, mercyGoal, actIds)
{
}

MonsterCategory MiniBossMonster::getCategory() const
{
    return MonsterCategory::MINIBOSS;
}

int MiniBossMonster::getMaxActChoices() const
{
    return 3;
}

unique_ptr<Monster> MiniBossMonster::clone() const
{
    return make_unique<MiniBossMonster>(*this);
}

BossMonster::BossMonster(const string& name,
                         int maxHp,
                         int atk,
                         int def,
                         int mercyGoal,
                         const vector<string>& actIds)
    : Monster(name, maxHp, atk, def, mercyGoal, actIds)
{
}

MonsterCategory BossMonster::getCategory() const
{
    return MonsterCategory::BOSS;
}

int BossMonster::getMaxActChoices() const
{
    return 4;
}

unique_ptr<Monster> BossMonster::clone() const
{
    return make_unique<BossMonster>(*this);
}
