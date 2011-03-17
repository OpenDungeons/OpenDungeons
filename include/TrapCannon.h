#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include <vector>
#include <string>

#include "Trap.h"
#include "Tile.h"
#include "AttackableObject.h"

class TrapCannon: public Trap
{
    public:
        TrapCannon();

        bool doUpkeep();
        Tile* positionTile();

        int x, y;

    private:
        int reloadTime;
        int reloadTimeCounter;
        double range;
        double minDamage, maxDamage;
        double cannonHeight;

};

#endif

