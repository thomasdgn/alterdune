#ifndef BESTIARYENTRY_H
#define BESTIARYENTRY_H

#include "Monster.h"

#include <string>

class BestiaryEntry
{
public:
    BestiaryEntry(const std::string& name = "",
                  MonsterCategory category = MonsterCategory::NORMAL,
                  int maxHp = 0,
                  int atk = 0,
                  int def = 0,
                  const std::string& result = "");

    const std::string& getName() const;
    MonsterCategory getCategory() const;
    int getMaxHp() const;
    int getAtk() const;
    int getDef() const;
    const std::string& getResult() const;

private:
    std::string m_name;
    MonsterCategory m_category;
    int m_maxHp;
    int m_atk;
    int m_def;
    std::string m_result;
};

#endif
