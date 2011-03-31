#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

<<<<<<< HEAD
class TrapCannon : public ProximityTrap
{
	public:
		TrapCannon();
        //~ bool doUpkeep();
	private:
		double cannonHeight;
=======
#include "Trap.h"
class Tile;

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
>>>>>>> aa695ceddcac90dceda12a70d009c0d9fcffacff

};

#endif

