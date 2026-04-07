#ifndef ACTACTION_H
#define ACTACTION_H

#include <string>

class ActAction
{
public:
    ActAction(const std::string& id = "", const std::string& text = "", int mercyImpact = 0);

    const std::string& getId() const;
    const std::string& getText() const;
    int getMercyImpact() const;

private:
    std::string m_id;
    std::string m_text;
    int m_mercyImpact;
};

#endif
