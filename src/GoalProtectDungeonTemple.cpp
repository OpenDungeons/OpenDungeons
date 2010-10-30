#include <vector>

#include "Globals.h"
#include "GoalProtectDungeonTemple.h"

GoalProtectDungeonTemple::GoalProtectDungeonTemple(string nName, string nArguments)
	: Goal(nName, nArguments)
{
	cout << "\nAdding goal " << getName();
}

bool GoalProtectDungeonTemple::isMet(Seat *s)
{
	std::vector<Room*> aliveDungeonTemples = gameMap.getRoomsByTypeAndColor(Room::dungeonTemple, s->color);
	if(aliveDungeonTemples.size() > 0)
		return true;
	else
		return false;
}

bool GoalProtectDungeonTemple::isUnmet(Seat *s)
{
	return false;
}

bool GoalProtectDungeonTemple::isFailed(Seat *s)
{
	return !isMet(s);
}

string GoalProtectDungeonTemple::getDescription()
{
	return "Protect your dungeon temple.";
}

string GoalProtectDungeonTemple::getSuccessMessage()
{
	return "Your dungeon temple is intact";
}

string GoalProtectDungeonTemple::getFailedMessage()
{
	return "Your dungeon temple has been destroyed";
}

