/*!
 *  Copyright (C) 2011-2015  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "modes/AbstractApplicationMode.h"

#include "entities/Creature.h"
#include "entities/RenderedMovableEntity.h"
#include "gamemap/GameMap.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/ODFrameListener.h"

bool AbstractApplicationMode::isConnected()
{
    //TODO: isConnected is used in some places to know if the game is started. We should use something better
    return (ODServer::getSingleton().isConnected() || ODClient::getSingleton().isConnected());
}

void AbstractApplicationMode::giveFocus()
{
    mModeManager->getInputManager()->mMouse->setEventCallback(this);
    mModeManager->getInputManager()->mKeyboard->setEventCallback(this);
}

bool AbstractApplicationMode::isChatKey(const OIS::KeyEvent &arg)
{
    if(arg.key == OIS::KeyCode::KC_RETURN || arg.key == OIS::KeyCode::KC_ESCAPE ||
       arg.key == OIS::KeyCode::KC_BACK || arg.text != 0)
        return true;

    return false;
}

int AbstractApplicationMode::getChatChar(const OIS::KeyEvent &arg)
{
    return arg.text;
}

GameEntity* AbstractApplicationMode::getEntityFromOgreName(const std::string& entityName)
{
    GameMap* gameMap = ODFrameListener::getSingletonPtr()->getClientGameMap();
    if (entityName.find(Creature::CREATURE_PREFIX) != std::string::npos)
    {
        // It is a creature
        std::string name = entityName.substr(Creature::CREATURE_PREFIX.length());
        return gameMap->getCreature(name);
    }
    else if (entityName.find(RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX) != std::string::npos)
    {
        std::string name = entityName.substr(RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX.length());
        return gameMap->getRenderedMovableEntity(name);
    }

    return nullptr;
}
