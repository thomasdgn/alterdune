#include "BestiaryEntry.h"

BestiaryEntry::BestiaryEntry(const std::string& name,
                             MonsterCategory category,
                             int maxHp,
                             int atk,
                             int def,
                             const std::string& result)
    : m_name(name), m_category(category), m_maxHp(maxHp), m_atk(atk), m_def(def), m_result(result)
{
}

const std::string& BestiaryEntry::getName() const
{
    return m_name;
}

MonsterCategory BestiaryEntry::getCategory() const
{
    return m_category;
}

int BestiaryEntry::getMaxHp() const
{
    return m_maxHp;
}

int BestiaryEntry::getAtk() const
{
    return m_atk;
}

int BestiaryEntry::getDef() const
{
    return m_def;
}

const std::string& BestiaryEntry::getResult() const
{
    return m_result;
}
