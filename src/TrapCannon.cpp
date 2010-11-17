#include "Globals.h"
#include "TrapCannon.h"

TrapCannon::TrapCannon()
	: Trap()
{
}

void TrapCannon::doUpkeep()
{
	std::vector<Tile*> visibleTiles = gameMap.visibleTiles(coveredTiles[0], range);
	for(unsigned int i = 0; i < visibleTiles.size(); i++)
	{
	}
}

