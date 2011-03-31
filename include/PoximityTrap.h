#ifndef PROXIMITY_TRAP_H
#define PROXIMITY_TRAP_H

class ProximityTrap : public Trap
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
	
