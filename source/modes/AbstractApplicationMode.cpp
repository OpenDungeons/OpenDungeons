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

#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/Gui.h"
#include "sound/SoundEffectsManager.h"

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
        Gui::convertButton(id));
}

bool AbstractApplicationMode::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::convertButton(id));
}

bool AbstractApplicationMode::keyPressed(const OIS::KeyEvent& arg)
{
    switch (arg.key)
    {
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
    mModeManager->getInputManager().mMouse->setEventCallback(this);
    mModeManager->getInputManager().mKeyboard->setEventCallback(this);
}
