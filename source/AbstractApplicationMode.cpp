#include "AbstractApplicationMode.h"

AbstractApplicationMode::AbstractApplicationMode(GameStateManager* gameStateManager, AbstractApplicationMode* parentState)
    : gameStateManager(gameStateManager), parentState(parentState)
{

}

AbstractApplicationMode::~AbstractApplicationMode()
{

}

