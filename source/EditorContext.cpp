/*
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

#include "EditorContext.h"

#include "ASWrapper.h"
#include "ODApplication.h"
#include "CameraManager.h"
#include "LogManager.h"
#include "RenderManager.h"
#include "GameMap.h"

EditorContext::EditorContext(Ogre::RenderWindow* renderWindow, ModeManager* modeManager, GameMap *gm):
    mGameMap(gm),
    mRenderManager(RenderManager::getSingletonPtr()),
    mCameraManager(NULL),
    mModeManager(modeManager),
    mLogManager(LogManager::getSingletonPtr()),
    mPreviousTurn(0),
    mCreatureSceneNode(NULL),
    mRoomSceneNode(NULL),
    mFieldSceneNode(NULL),
    mLightSceneNode(NULL)
{
    mRenderManager->setGameMap(mGameMap);

    //NOTE This is moved here temporarily.
    try
    {
        // renderManager->createCamera();

        // renderManager->createViewports();
        mLogManager->logMessage("Creating scene...", Ogre::LML_NORMAL);
        mRenderManager->createScene();
        mLogManager->logMessage("Creating compositors...", Ogre::LML_NORMAL);
        mRenderManager->createCompositors();
    }
    catch(Ogre::Exception& e)
    {
        ODApplication::displayErrorMessage("Ogre exception when initialising the render manager:\n"
            + e.getFullDescription(), false);
        // TODO: Cleanly exit instead
        exit(0);
        //cleanUp();
        //return;
    }
    catch (std::exception& e)
    {
        ODApplication::displayErrorMessage("Exception when initialising the render manager:\n"
            + std::string(e.what()), false);
        // TODO: Cleanly exit instead
        exit(0);
        //cleanUp();
        //return;
    }

    mLogManager->logMessage("Created camera manager");

    mGameMap->createTilesMeshes();
}

EditorContext::~EditorContext()
{
}

void EditorContext::onFrameStarted(const Ogre::FrameEvent& evt)
{
    mCameraManager->moveCamera(evt.timeSinceLastFrame);

    mGameMap->getMiniMap()->draw();
    mGameMap->getMiniMap()->swap();
}

void EditorContext::onFrameEnded(const Ogre::FrameEvent& evt)
{
}

void EditorContext::setCameraManager(CameraManager* tmpCm)
{
    mCameraManager = tmpCm;
}
