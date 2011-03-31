//~ #include "Globals.h"
//~ #include "Functions.h"
#include "TrapCannon.h"
//~ #include "MissileObject.h"

TrapCannon::TrapCannon()
	: ProximityTrap()
{
	reloadTime = 5;
	reloadTimeCounter = reloadTime;
	range = 12;
	minDamage = 4;
	maxDamage = 20;
	cannonHeight = 1.5;
}

