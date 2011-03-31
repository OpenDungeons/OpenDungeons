#ifndef TRAPBOULDER_H
#define TRAPBOULDER_H

#include "DirectionalTrap.h"

class TrapBoulder : public DirectionalTrap
{
    public:
        TrapBoulder(int, int);
        std::vector<AttackableObject*> aimEnemy();
};

#endif