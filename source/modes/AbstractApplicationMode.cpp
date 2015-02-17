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
#include "entities/MapLight.h"
#include "gamemap/GameMap.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "spell/Spell.h"

#include <CEGUI/System.h>
#include <CEGUI/GUIContext.h>
#include <CEGUI/widgets/PushButton.h>

AbstractApplicationMode::~AbstractApplicationMode()
{
    //Disconnect all event connections.
    for(CEGUI::Event::Connection& c : mEventConnections)
    {
        c->disconnect();
    }
}

bool AbstractApplicationMode::isConnected()
{
    //TODO: isConnected is used in some places to know if the game is started. We should use something better
    return (ODServer::getSingleton().isConnected() || ODClient::getSingleton().isConnected());
}

bool AbstractApplicationMode::mouseMoved(const OIS::MouseEvent& arg)
{
    if(arg.state.Z.rel != 0)
    {
        CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseWheelChange(
                  static_cast<float>(arg.state.Z.rel) / 100.0f);
    }
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(
              static_cast<float>(arg.state.X.abs), static_cast<float>(arg.state.Y.abs));
}

bool AbstractApplicationMode::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool AbstractApplicationMode::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool AbstractApplicationMode::keyPressed(const OIS::KeyEvent& arg)
{
    switch (arg.key)
    {

    case OIS::KC_ESCAPE:
        regressMode();
        break;
    default:
        CEGUI::System::getSingleton().getDefaultGUIContext().injectChar(
            static_cast<CEGUI::String::value_type>(arg.text));
        CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyDown(
            static_cast<CEGUI::Key::Scan>(arg.key));
        break;
    }
    return true;
}

bool AbstractApplicationMode::keyReleased(const OIS::KeyEvent& arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectKeyUp(
      static_cast<CEGUI::Key::Scan>(arg.key));
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
    // We check the prefix to know the kind of object the user clicked. Then, we call the corresponding
    // GameMap function to retrieve the entity
    GameMap* gameMap = ODFrameListener::getSingletonPtr()->getClientGameMap();
    if (entityName.compare(0, Creature::CREATURE_PREFIX.length(), Creature::CREATURE_PREFIX) == 0)
    {
        // It is a creature
        std::string name = entityName.substr(Creature::CREATURE_PREFIX.length());
        return gameMap->getCreature(name);
    }
    else if (entityName.compare(0, RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX.length(), RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX) == 0)
    {
        std::string name = entityName.substr(RenderedMovableEntity::RENDEREDMOVABLEENTITY_OGRE_PREFIX.length());
        return gameMap->getRenderedMovableEntity(name);
    }
    else if (entityName.compare(0, Spell::SPELL_OGRE_PREFIX.length(), Spell::SPELL_OGRE_PREFIX) == 0)
    {
        std::string name = entityName.substr(Spell::SPELL_OGRE_PREFIX.length());
        return gameMap->getSpell(name);
    }
    else if (entityName.compare(0, MapLight::MAPLIGHT_INDICATOR_PREFIX.length(), MapLight::MAPLIGHT_INDICATOR_PREFIX) == 0)
    {
        std::string name = entityName.substr(MapLight::MAPLIGHT_INDICATOR_PREFIX.length());
        return gameMap->getMapLight(name);
    }

    return nullptr;
}

void AbstractApplicationMode::subscribeCloseButton(CEGUI::Window& rootWindow)
{
    CEGUI::Window* closeButton = rootWindow.getChild("__auto_closebutton__");
    addEventConnection(
        closeButton->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(&AbstractApplicationMode::regressModeEvent,
              this)));
}
