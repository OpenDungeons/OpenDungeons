#include "ButtonHandlers.h"

bool quitButtonPressed(const CEGUI::EventArgs &e)
{
	return true;
}

bool quartersButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::quarters;
	gameMap.me->newTrapType = Trap::nullTrapType;
	return true;
}

bool treasuryButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::treasury;
	gameMap.me->newTrapType = Trap::nullTrapType;
	return true;
}

bool forgeButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::forge;
	gameMap.me->newTrapType = Trap::nullTrapType;
	return true;
}

bool cannonButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::nullRoomType;
	gameMap.me->newTrapType = Trap::cannon;
	return true;
}

