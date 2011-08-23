#include <vector>
#include <iostream>

#include "Globals.h"
#include "GoalProtectDungeonTemple.h"
#include "GameMap.h"
#include "Room.h"
#include "Seat.h"

GoalProtectDungeonTemple::GoalProtectDungeonTemple(const std::string& nName,
        const std::string& nArguments, const GameMap& gameMap) :
    Goal(nName, nArguments, gameMap)
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalProtectDungeonTemple::isMet(Seat *s)
{
    return (gameMap.numRoomsByTypeAndColor(Room::dungeonTemple, s->color) > 0);
}

bool GoalProtectDungeonTemple::isUnmet(Seat *s)
{
    return false;
}

bool GoalProtectDungeonTemple::isFailed(Seat *s)
{
    return !isMet(s);
}

std::string GoalProtectDungeonTemple::getDescription()
{
    return "Protect your dungeon temple.";
}

std::string GoalProtectDungeonTemple::getSuccessMessage()
{
    return "Your dungeon temple is intact";
}

std::string GoalProtectDungeonTemple::getFailedMessage()
{
    return "Your dungeon temple has been destroyed";
}

