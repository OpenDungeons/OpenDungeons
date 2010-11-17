#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include <vector>
#include <string>

#include "Trap.h"
#include "Tile.h"
#include "AttackableObject.h"

class TrapCannon : public Trap
{
	public:
		TrapCannon();

		void doUpkeep();
		Tile* positionTile();

		int x, y;

	private:
		double range;
		double minDamage, maxDamage;

};

#endif

