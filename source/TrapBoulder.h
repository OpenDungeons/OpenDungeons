#ifndef TRAPBOULDER_H
#define TRAPBOULDER_H

#include "DirectionalTrap.h"

class TrapBoulder : public DirectionalTrap
{
    public:
        TrapBoulder(int, int);
        std::vector<AttackableEntity*> aimEnemy();
        virtual void damage(std::vector<AttackableEntity*>);
};

#endif
