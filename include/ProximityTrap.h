#ifndef PROXIMITY_TRAP_H
#define PROXIMITY_TRAP_H

#include "Trap.h"

class ProximityTrap : public Trap
{
	public:
		ProximityTrap();

		bool doUpkeep();
		Tile* positionTile();
        
        virtual std::vector<AttackableObject*> aimEnemy();
        virtual void damage(std::vector<AttackableObject*>);

	protected:
		int reloadTime;
		int reloadTimeCounter;
		double range;
		double minDamage, maxDamage;
};

#endif