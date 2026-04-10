#include "Entity.h"

#include <algorithm>
#include <iostream>

using namespace std;

Entity::Entity(const string& name, int maxHp, int atk, int def)
    : m_name(name), m_hp(maxHp), m_maxHp(maxHp), m_atk(atk), m_def(def)
{
}

string Entity::getEntityType() const
{
    return "Entity";
}

void Entity::printStatus(ostream& os) const
{
    os << getEntityType() << " - " << m_name << " | HP: " << m_hp << "/" << m_maxHp
       << " | ATK: " << m_atk << " | DEF: " << m_def;
}

void Entity::takeDamage(int rawDamage)
{
    const int effectiveDamage = max(1, rawDamage - m_def);
    m_hp = max(0, m_hp - effectiveDamage);
}

void Entity::heal(int amount)
{
    if (amount <= 0)
    {
        return;
    }

    m_hp = min(m_maxHp, m_hp + amount);
}

bool Entity::isAlive() const
{
    return m_hp > 0;
}

const string& Entity::getName() const
{
    return m_name;
}

int Entity::getHp() const
{
    return m_hp;
}

int Entity::getMaxHp() const
{
    return m_maxHp;
}

int Entity::getAtk() const
{
    return m_atk;
}

int Entity::getDef() const
{
    return m_def;
}

void Entity::setName(const string& name)
{
    m_name = name;
}

void Entity::setHp(int hp)
{
    m_hp = clamp(hp, 0, m_maxHp);
}

void Entity::setMaxHp(int maxHp)
{
    m_maxHp = max(1, maxHp);
    m_hp = min(m_hp, m_maxHp);
}

void Entity::setAtk(int atk)
{
    m_atk = max(0, atk);
}

void Entity::setDef(int def)
{
    m_def = max(0, def);
}
