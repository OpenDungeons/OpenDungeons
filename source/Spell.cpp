#include <algorithm>
#include <cmath>

#include "Spell.h"
#include "Creature.h"

double Spell::heal(int spellLevel, Creature *targetCreature, double hp,
        double maxMana)
{
    const double manaPerHPHealed = 0.1;
    double manaCost = 5;
    double maxHPHealed = (maxMana - manaCost) / manaPerHPHealed;
    maxHPHealed = std::min(maxHPHealed, 10.0 * spellLevel);
    maxHPHealed = std::min(maxHPHealed, hp);

    //TODO: Should lock the HP to prevent race conditions.
    double currentHP = targetCreature->getHP();
    double maxHP = targetCreature->getMaxHp();

    if (currentHP <= 0.0)
        return manaCost;

    double newHP = std::min(currentHP + maxHPHealed, maxHP);
    double amountHealed = newHP - currentHP;

    // Prevent lowering the HP if the creature has more than full HP due to some other effect.
    if (amountHealed > 0.0)
    {
        manaCost += 0.1 * amountHealed;
        targetCreature->setHP(newHP);
    }

    return manaCost;
}

