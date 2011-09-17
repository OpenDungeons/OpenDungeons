#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

class TrapCannon : public ProximityTrap
{
    public:
        TrapCannon();
        std::vector<AttackableEntity*> aimEnemy();
		void damage(std::vector<AttackableEntity*>);
    private:
        double cannonHeight;
};

#endif

