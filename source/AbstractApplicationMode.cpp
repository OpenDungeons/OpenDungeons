/*!
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "AbstractApplicationMode.h"

#include "ODServer.h"
#include "ODClient.h"

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
