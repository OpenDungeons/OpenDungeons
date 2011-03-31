#ifndef DIRECTIONAL_TRAP_H
#define DIRECTIONAL_TRAP_H

#include <utility>
#include "Trap.h"

class DirectionalTrap : public Trap
{
    public:
        DirectionalTrap(int, int);

        bool doUpkeep();
        Tile* positionTile();
        
        virtual std::vector<AttackableObject*> aimEnemy();
        virtual void damage(std::vector<AttackableObject*>);

    protected:
        std::pair<int, int> dir;
        int reloadTime;
        int reloadTimeCounter;
        double range;
        double minDamage, maxDamage;
        std::pair<int, int> projection_on_border(int, int);
};

#endif