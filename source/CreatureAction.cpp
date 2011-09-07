#include "Creature.h"

#include "CreatureAction.h"

CreatureAction::CreatureAction() :
        type    (idle),
        tile    (0),
        creature(0)
{
}

CreatureAction::CreatureAction(const ActionType nType, Tile *nTile, Creature *nCreature) :
        type    (nType),
        tile    (nTile),
        creature(nCreature)
{
}

std::string CreatureAction::toString() const
{
    switch (type)
    {
        case walkToTile:
            return "walkToTile";

        case maneuver:
            return "maneuver";

        case digTile:
            return "digTile";

        case claimTile:
            return "claimTile";

        case depositGold:
            return "depositGold";

        case attackObject:
            return "attackObject";

        case findHome:
            return "findHome";

        case sleep:
            return "sleep";

        case train:
            return "train";

        case idle:
            return "idle";

        default:
            return "unhandledAct";
    }
}

