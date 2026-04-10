#include "ActAction.h"

using namespace std;

ActAction::ActAction(const string& id, const string& text, int mercyImpact)
    : m_id(id), m_text(text), m_mercyImpact(mercyImpact)
{
}

const string& ActAction::getId() const
{
    return m_id;
}

const string& ActAction::getText() const
{
    return m_text;
}

int ActAction::getMercyImpact() const
{
    return m_mercyImpact;
}
