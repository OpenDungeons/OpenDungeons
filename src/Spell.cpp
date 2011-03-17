#include <algorithm>

#include "Spell.h"

double Spell::heal(int spellLevel, Creature *targetCreature, double hp,
        double maxMana)
{
    const double manaPerHPHealed = 0.1;
    double manaCost = 5;
    double maxHPHealed = (maxMana - manaCost) / manaPerHPHealed;
    maxHPHealed = min(maxHPHealed, 10.0 * spellLevel);
    maxHPHealed = min(maxHPHealed, hp);

    //TODO: Should lock the HP to prevent race conditions.
    double currentHP = targetCreature->getHP(NULL);
    double maxHP = targetCreature->maxHP;

    if (currentHP <= 0.0)
        return manaCost;

    double newHP = min(currentHP + maxHPHealed, maxHP);
    double amountHealed = newHP - currentHP;

    // Prevent lowering the HP if the creature has more than full HP due to some other effect.
    if (amountHealed > 0.0)
    {
        manaCost += 0.1 * amountHealed;
        targetCreature->setHP(newHP);
    }

    return manaCost;
}

