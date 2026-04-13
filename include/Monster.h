#ifndef MONSTER_H
#define MONSTER_H

#include "Entity.h"

#include <memory>
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
            int maxHp = 0,
            int atk = 0,
            int def = 0,
            int mercyGoal = 100,
            const std::vector<std::string>& actIds = {});
    virtual ~Monster() = default;

    std::string getEntityType() const override;
    void printStatus(std::ostream& os) const override;

    virtual MonsterCategory getCategory() const = 0;
    virtual int getMaxActChoices() const = 0;
    virtual std::unique_ptr<Monster> clone() const = 0;

    int getMercy() const;
    int getMercyGoal() const;
    const std::vector<std::string>& getActIds() const;
    std::vector<std::string> getAvailableActIds() const;

    void addMercy(int amount);
    bool isSpareable() const;

    static MonsterCategory categoryFromString(const std::string& value);
    static std::string categoryToString(MonsterCategory category);

private:
    int m_mercy;
    int m_mercyGoal;
    std::vector<std::string> m_actIds;
};

class NormalMonster : public Monster
{
public:
    NormalMonster(const std::string& name = "",
                  int maxHp = 0,
                  int atk = 0,
                  int def = 0,
                  int mercyGoal = 100,
                  const std::vector<std::string>& actIds = {});

    MonsterCategory getCategory() const override;
    int getMaxActChoices() const override;
    std::unique_ptr<Monster> clone() const override;
};

class MiniBossMonster : public Monster
{
public:
    MiniBossMonster(const std::string& name = "",
                    int maxHp = 0,
                    int atk = 0,
                    int def = 0,
                    int mercyGoal = 100,
                    const std::vector<std::string>& actIds = {});

    MonsterCategory getCategory() const override;
    int getMaxActChoices() const override;
    std::unique_ptr<Monster> clone() const override;
};

class BossMonster : public Monster
{
public:
    BossMonster(const std::string& name = "",
                int maxHp = 0,
                int atk = 0,
                int def = 0,
                int mercyGoal = 100,
                const std::vector<std::string>& actIds = {});

    MonsterCategory getCategory() const override;
    int getMaxActChoices() const override;
    std::unique_ptr<Monster> clone() const override;
};

#endif
