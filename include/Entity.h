#ifndef ENTITY_H
#define ENTITY_H

#include <iosfwd>
#include <string>

class Entity
{
public:
    Entity(const std::string& name = "", int maxHp = 0, int atk = 0, int def = 0);
    virtual ~Entity() = default;

    virtual std::string getEntityType() const;
    virtual void printStatus(std::ostream& os) const;

    int takeDamage(int rawDamage);
    int heal(int amount);
    bool isAlive() const;

    const std::string& getName() const;
    int getHp() const;
    int getMaxHp() const;
    int getAtk() const;
    int getDef() const;

    void setName(const std::string& name);
    void setHp(int hp);
    void setMaxHp(int maxHp);
    void setAtk(int atk);
    void setDef(int def);

protected:
    std::string m_name;
    int m_hp;
    int m_maxHp;
    int m_atk;
    int m_def;
};

#endif
