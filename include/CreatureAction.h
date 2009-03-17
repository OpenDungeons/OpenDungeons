#ifndef CREATUREACTION_H
#define CREATUREACTION_H

#include "Tile.h"
class CreatureAction;
#include "Creature.h"

class CreatureAction
{
	public:
		enum ActionType {walkToTile, digTile, attackCreature, idle};

		CreatureAction();
		CreatureAction(ActionType nType, Tile *nTile=NULL, Creature *nCreature=NULL);

		ActionType type;
		Tile *tile;
		Creature *creature;
};

#endif

