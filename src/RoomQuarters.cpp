#include "RoomQuarters.h"

RoomQuarters::RoomQuarters()
	: Room()
{
	return;
}

void RoomQuarters::doUpkeep()
{
	// Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
	Room::doUpkeep();

	cout << "\n\nDoing upkeep for room " << name << "\n";
}

