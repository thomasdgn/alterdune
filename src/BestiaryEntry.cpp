#include "BestiaryEntry.h"

using namespace std;

BestiaryEntry::BestiaryEntry(const string& name,
                             MonsterCategory category,
                             int maxHp,
                             int atk,
                             int def,
                             const string& description,
                             const string& result)
    : m_name(name),
      m_category(category),
      m_maxHp(maxHp),
      m_atk(atk),
      m_def(def),
      m_description(description),
      m_result(result)
{
}

const string& BestiaryEntry::getName() const
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

const string& BestiaryEntry::getDescription() const
{
    return m_description;
}

const string& BestiaryEntry::getResult() const
{
    return m_result;
}

void BestiaryEntry::setResult(const string& result)
{
    m_result = result;
}
