#include "CreatureAction.h"

CreatureAction::CreatureAction()
{
	type = idle;
	tile = NULL;
	creature = NULL;
}

CreatureAction::CreatureAction(ActionType nType, Tile *nTile, Creature *nCreature)
{
	type = nType;
	tile = nTile;
	creature = nCreature;
}

