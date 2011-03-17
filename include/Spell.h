#ifndef SPELL_H
#define SPELL_H

#include "Creature.h"

class Spell
{
    public:
        static double heal(int spellLevel, Creature *targetCreature, double hp,
                double maxMana);

};

#endif

