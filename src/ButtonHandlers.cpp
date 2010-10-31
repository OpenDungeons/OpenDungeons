#include "ButtonHandlers.h"

bool quitButtonPressed(const CEGUI::EventArgs &e)
{
	return true;
}

bool quartersButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::quarters;
	return true;
}

bool treasuryButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::treasury;
	return true;
}

bool forgeButtonPressed(const CEGUI::EventArgs &e)
{
	gameMap.me->newRoomType = Room::forge;
	return true;
}

