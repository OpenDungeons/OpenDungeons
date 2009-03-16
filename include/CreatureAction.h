#ifndef CREATUREACTION_H
#define CREATUREACTION_H

#include "Tile.h"
#include "Creature.h"

class CreatureAction
{
	public:
		CreatureAction();
		enum ActionType {walkToTile, digTile, attackCreature, idle};

		ActionType type;
		Tile *tile;
		Creature *creature;
};

#endif

