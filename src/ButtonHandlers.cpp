#include "Functions.h"
#include "Globals.h"
#include "ExampleFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"

#include "ButtonHandlers.h"

bool quitButtonPressed(const CEGUI::EventArgs &e)
{
    writeGameMapToFile(std::string("levels/Test.level") + std::string(".out"));
    exampleFrameListener->mContinue = false;
    return true;
}

bool quartersButtonPressed(const CEGUI::EventArgs &e)
{
    gameMap.me->newRoomType = Room::quarters;
    gameMap.me->newTrapType = Trap::nullTrapType;
    //Show text on pointer - should get the strings from somewhere.
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "quarters");
    return true;
}

bool treasuryButtonPressed(const CEGUI::EventArgs &e)
{
    gameMap.me->newRoomType = Room::treasury;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "treasury");
    return true;
}

bool forgeButtonPressed(const CEGUI::EventArgs &e)
{
    gameMap.me->newRoomType = Room::forge;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "forge");
    return true;
}

bool dojoButtonPressed(const CEGUI::EventArgs &e)
{
    gameMap.me->newRoomType = Room::dojo;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "dojo");
    return true;
}

bool cannonButtonPressed(const CEGUI::EventArgs &e)
{
    gameMap.me->newRoomType = Room::nullRoomType;
    gameMap.me->newTrapType = Trap::cannon;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "cannon");
    return true;
}

bool serverButtonPressed(const CEGUI::EventArgs &e)
{
    return startServer();
}

