/*
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

#include "FppMode.h"

#include "render/Gui.h"

FppMode::FppMode(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::FPP)
{
}

FppMode::~FppMode()
{
}

void FppMode::activate()
{
    // Loads the corresponding Gui sheet.
    Gui::getSingleton().loadGuiSheet(Gui::hideGui);

    giveFocus();
}

bool FppMode::mouseMoved(const OIS::MouseEvent &arg)
{
    // CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    // if (arg.state.X.abs == 0)
    // 	mMc->frameListener->cm->move(CameraManager::moveLeft);
    // else
    // 	mMc->frameListener->cm->move(CameraManager::stopLeft);

    // if (arg.state.X.abs ==  arg.state.width)
    // 	mMc->frameListener->cm->move(CameraManager::moveRight);
    // else
    // 	mMc->frameListener->cm->move(CameraManager::stopRight);

    // if (arg.state.Y.abs == 0)
    // 	mMc->frameListener->cm->move(CameraManager::moveForward);
    // else
    // 	mMc->frameListener->cm->move(CameraManager::stopForward);

    // if (arg.state.Y.abs ==  arg.state.height)
    // 	mMc->frameListener->cm->move(CameraManager::moveBackward);
    // else
    // 	mMc->frameListener->cm->move(CameraManager::stopBackward);
    return true;
}

bool FppMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
                Gui::getSingletonPtr()->convertButton(id));
}

bool FppMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
                Gui::getSingletonPtr()->convertButton(id));
}

bool FppMode::keyPressed(const OIS::KeyEvent &arg)
{
    switch (arg.key)
    {
    case OIS::KC_ESCAPE:
        regressMode();
        break;
    default:
        break;
    }
    return true;
}

bool FppMode::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void FppMode::handleHotkeys(OIS::KeyCode keycode)
{
}
