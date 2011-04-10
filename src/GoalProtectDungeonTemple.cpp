#include <vector>
#include <iostream>

#include "Globals.h"
#include "GoalProtectDungeonTemple.h"
#include "GameMap.h"
#include "Room.h"

GoalProtectDungeonTemple::GoalProtectDungeonTemple(const std::string& nName,
        const std::string& nArguments) :
    Goal(nName, nArguments)
{
    std::cout << "\nAdding goal " << getName();
}

bool GoalProtectDungeonTemple::isMet(Seat *s)
{
    return (gameMap.getRoomsByTypeAndColor(Room::dungeonTemple, s->color).size() > 0);
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

