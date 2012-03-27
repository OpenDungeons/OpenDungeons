#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

class TrapCannon : public ProximityTrap
{
    public:
        TrapCannon();
        std::vector<GameEntity*> aimEnemy();
		void damage(std::vector<GameEntity*>);
    private:
        double cannonHeight;
};

#endif

