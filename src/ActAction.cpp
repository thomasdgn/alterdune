#include "ActAction.h"

ActAction::ActAction(const std::string& id, const std::string& text, int mercyImpact)
    : m_id(id), m_text(text), m_mercyImpact(mercyImpact)
{
}

const std::string& ActAction::getId() const
{
    return m_id;
}

const std::string& ActAction::getText() const
{
    return m_text;
}

int ActAction::getMercyImpact() const
{
    return m_mercyImpact;
}
