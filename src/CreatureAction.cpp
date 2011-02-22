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

string CreatureAction::toString()
{
	switch(type)
	{
		case walkToTile:    return "walkToTile";    break;
		case maneuver:      return "maneuver";      break;
		case digTile:       return "digTile";       break;
		case claimTile:     return "claimTile";     break;
		case depositGold:   return "depositGold";   break;
		case attackObject:  return "attackObject";  break;
		case findHome:      return "findHome";      break;
		case sleep:         return "sleep";         break;
		case train:         return "train";         break;
		case idle:          return "idle";          break;
	}
}

