#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

class TrapCannon : public ProximityTrap
{
    public:
        TrapCannon();
        std::vector<AttackableObject*> aimEnemy();
    private:
        double cannonHeight;
};

#endif

