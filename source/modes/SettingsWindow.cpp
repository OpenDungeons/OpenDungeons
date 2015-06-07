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

#include "modes/SettingsWindow.h"

#include "utils/LogManager.h"

#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/WindowManager.h>

SettingsWindow::SettingsWindow(CEGUI::Window* rootWindow):
    mSettingsWindow(nullptr),
    mRootWindow(rootWindow)
{
    if (rootWindow == nullptr)
    {
        OD_LOG_ERR("Settings Window loaded without any main CEGUI window!!");
        return;
    }
    // Create the window child.
    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

     mSettingsWindow = wmgr->loadLayoutFromFile("WindowSettings.layout");
    if (mSettingsWindow == nullptr)
    {
        OD_LOG_ERR("Couldn't load the Settings Window layout!!");
        return;
    }
    rootWindow->addChild(mSettingsWindow);
    mSettingsWindow->hide();

    // Events connections
    addEventConnection(
        mSettingsWindow->getChild("CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::hideSettingsWindow, this)
        )
    );
    addEventConnection(
        mSettingsWindow->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::hideSettingsWindow, this)
        )
    );
}

SettingsWindow::~SettingsWindow()
{
    // Disconnects all event connections.
    for(CEGUI::Event::Connection& c : mEventConnections)
    {
        c->disconnect();
    }

    if (mSettingsWindow)
    {
        mRootWindow->removeChild(mSettingsWindow->getID());
        CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
        wmgr->destroyWindow(mSettingsWindow);
    }
}

bool SettingsWindow::hideSettingsWindow(const CEGUI::EventArgs&)
{
    hide();
    return true;
}
