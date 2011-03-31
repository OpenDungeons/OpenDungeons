#ifndef SPELL_H
#define SPELL_H

class Creature;

class Spell
{
    public:
        static double heal(int spellLevel, Creature *targetCreature, double hp,
                double maxMana);

};

#endif

