/*!
 * \file   ODFrameListener.cpp
 * \date   09 April 2011
 * \author Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 *
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

#include "render/ODFrameListener.h"

#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "modes/AbstractApplicationMode.h"
#include "modes/ModeManager.h"
#include "modes/GameMode.h"
#include "network/ODServer.h"
#include "network/ODClient.h"
#include "network/ChatMessage.h"
#include "render/Gui.h"
#include "render/RenderManager.h"
#include "render/TextRenderer.h"
#include "sound/MusicPlayer.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <Overlay/OgreOverlaySystem.h>
#include <CEGUI/EventArgs.h>
#include <CEGUI/Window.h>
#include <CEGUI/Size.h>
#include <CEGUI/System.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>


template<> ODFrameListener* Ogre::Singleton<ODFrameListener>::msSingleton = nullptr;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

namespace
{
    constexpr const unsigned int DEFAULT_FRAME_RATE = 60;
}

/*! \brief This constructor is where the OGRE rendering system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ODFrameListener::ODFrameListener(Ogre::RenderWindow* renderWindow, Ogre::OverlaySystem* overLaySystem, Gui* gui) :
    mInitialized(false),
    mWindow(renderWindow),
    mGui(gui),
    mRenderManager(Utils::make_unique<RenderManager>(overLaySystem)),
    mGameMap(Utils::make_unique<GameMap>(false)),
    mModeManager(new ModeManager(renderWindow, gui)),
    mShowDebugInfo(false),
    mContinue(true),
    mChatMaxTimeDisplay(20.0f),
    mRaySceneQuery(nullptr),
    mExitRequested(false),
    mCameraManager(mRenderManager->getSceneManager(), mGameMap.get(), renderWindow),
    mFpsLimiter(DEFAULT_FRAME_RATE)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating frame listener...");

    mRenderManager->createScene(mCameraManager.getViewport());

    mRaySceneQuery = mRenderManager->getSceneManager()->createRayQuery(Ogre::Ray());
    mRaySceneQuery->setQueryTypeMask(Ogre::SceneManager::ENTITY_TYPE_MASK);

    mRenderManager->getSceneManager()->addRenderQueueListener(this);
    //Set initial mouse clipping size
    windowResized(mWindow);

    //Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    mInitialized = true;
}

void ODFrameListener::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);

    const OIS::MouseState &ms = mModeManager->getInputManager()->mMouse->getMouseState();
    ms.width = width;
    ms.height = height;

    //Notify CEGUI that the display size has changed.
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size<float>(
            static_cast<float> (width), static_cast<float> (height)));
}

ODFrameListener::~ODFrameListener()
{
    if (mInitialized)
        exitApplication();

    mGameMap->clearAll();
    mGameMap->processDeletionQueues();
}

void ODFrameListener::requestExit()
{
    mExitRequested = true;
}

void ODFrameListener::exitApplication()
{
    LogManager::getSingleton().logMessage("Closing down.");

    ODClient::getSingleton().notifyExit();
    ODServer::getSingleton().notifyExit();
    mGameMap->clearAll();
    mGameMap->processDeletionQueues();
    mRenderManager->getSceneManager()->destroyQuery(mRaySceneQuery);

    Ogre::LogManager::getSingleton().logMessage("Remove listener registration");
    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);

    Ogre::LogManager::getSingleton().logMessage("Frame listener uninitialization done.");
    mInitialized = false;
}

void ODFrameListener::updateAnimations(Ogre::Real timeSinceLastFrame)
{
    MusicPlayer::getSingleton().update(static_cast<float>(timeSinceLastFrame));
    mRenderManager->updateRenderAnimations(timeSinceLastFrame);
    mGameMap->processDeletionQueues();

    mGameMap->updateAnimations(timeSinceLastFrame);
}

bool ODFrameListener::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    CEGUI::MouseCursor& mouseCursor = CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor();
    CEGUI::Vector2<float> mousePos = mouseCursor.getDisplayIndependantPosition();
    RenderManager::getSingleton().moveCursor(mousePos.d_x, mousePos.d_y);

    if (mWindow->isClosed())
        return false;

    // Sleep to limit the framerate to the max value
    mFpsLimiter.sleepIfEarly();

    CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(evt.timeSinceLastFrame);

    mModeManager->update(evt);

    int64_t currentTurn = mGameMap->getTurnNumber();

    if(!mExitRequested)
    {
        // Updates animations independant from the server new turn event
        updateAnimations(evt.timeSinceLastFrame);
    }

    mCameraManager.updateCameraFrameTime(evt.timeSinceLastFrame);
    mCameraManager.onFrameStarted();

    if((currentTurn != -1) && (mGameMap->getGamePaused()) && (!mExitRequested))
        return true;

    //If an exit has been requested, start cleaning up.
    if(mExitRequested == true || mContinue == false)
    {
        exitApplication();
        mContinue = false;
        return mContinue;
    }

    printDebugInfo();

    ODClient::getSingleton().processClientSocketMessages(*mGameMap.get());
    ODClient::getSingleton().processClientNotifications();

    return mContinue;
}

void ODFrameListener::refreshPlayerDisplay(const std::string& goalsDisplayString)
{
    Seat* mySeat = mGameMap->getLocalPlayer()->getSeat();

    //! \brief Updates common info on screen.
    CEGUI::Window *tempWindow = mGui->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_TERRITORY);
    std::stringstream tempSS("");
    tempSS << mySeat->getNumClaimedTiles();
    tempWindow->setText(tempSS.str());

    tempWindow = mGui->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_CREATURES);
    tempSS.str("");
    tempSS << mySeat->getNumCreaturesFighters() << "/" << mySeat->getNumCreaturesFightersMax();
    tempWindow->setText(tempSS.str());

    tempWindow = mGui->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_GOLD);
    tempSS.str("");
    tempSS << mySeat->getGold();
    tempWindow->setText(tempSS.str());

    tempWindow = mGui->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_MANA);
    tempSS.str("");
    tempSS << mySeat->getMana() << " " << (mySeat->getManaDelta() >= 0 ? "+" : "-")
            << mySeat->getManaDelta();
    tempWindow->setText(tempSS.str());

    tempWindow = mGui->getGuiSheet(Gui::inGameMenu)->getChild(Gui::OBJECTIVE_TEXT);
    tempWindow->setText(goalsDisplayString);
}

bool ODFrameListener::frameEnded(const Ogre::FrameEvent& evt)
{
    AbstractApplicationMode* currentMode = mModeManager->getCurrentMode();
    currentMode->onFrameEnded(evt);

    mCameraManager.onFrameEnded();

    return true;
}

void ODFrameListener::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation,
    bool& skipThisInvocation)
{
    if(queueGroupId == Ogre::RENDER_QUEUE_OVERLAY && invocation == "")
    {
        CEGUI::System::getSingleton().renderAllGUIContexts();
    }
}

bool ODFrameListener::quit(const CEGUI::EventArgs &e)
{
    requestExit();
    return true;
}

Ogre::RaySceneQueryResult& ODFrameListener::doRaySceneQuery(const OIS::MouseEvent &arg)
{
    // Setup the ray scene query, use CEGUI's mouse position
    CEGUI::Vector2<float> mousePos = CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().getPosition();// * mMouseScale;
    Ogre::Ray mouseRay = mCameraManager.getActiveCamera()->getCameraToViewportRay(mousePos.d_x / float(
            arg.state.width), mousePos.d_y / float(arg.state.height));

    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);

    // Execute query
    return mRaySceneQuery->execute();
}

Ogre::RaySceneQueryResult& ODFrameListener::doRaySceneQuery(const OIS::MouseEvent &arg, Ogre::Vector3& keeperHand3DPos)
{
    // Setup the ray scene query, use CEGUI's mouse position
    CEGUI::Vector2<float> mousePos = CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().getPosition();// * mMouseScale;
    Ogre::Ray mouseRay = mCameraManager.getActiveCamera()->getCameraToViewportRay(mousePos.d_x / float(
            arg.state.width), mousePos.d_y / float(arg.state.height));

    Ogre::Plane groundPlane(Ogre::Vector3::UNIT_Z, RenderManager::KEEPER_HAND_WORLD_Z);
    std::pair<bool, Ogre::Real> p = mouseRay.intersects(groundPlane);
    if(p.first)
        keeperHand3DPos = mouseRay.getPoint(p.second);

    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);

    // Execute query
    return mRaySceneQuery->execute();

}

void ODFrameListener::printDebugInfo()
{
    std::stringstream infoSS;
    if (!mGameMap->isInEditorMode() && mGameMap->getTurnNumber() == -1)
    {
        infoSS << "Waiting for players...\n";
    }
    if (mShowDebugInfo)
    {
        infoSS << "FPS: " << mWindow->getStatistics().lastFPS;
        infoSS << "\ntriangleCount: " << mWindow->getStatistics().triangleCount;
        infoSS << "\nBatches: " << mWindow->getStatistics().batchCount;
        infoSS << "\nTurn number:  " << mGameMap->getTurnNumber();
        if(ODClient::getSingleton().isConnected())
        {
            int32_t gameTime = ODClient::getSingleton().getGameTimeMillis() / 1000;
            int32_t seconds = gameTime % 60;
            gameTime /= 60;
            int32_t minutes = gameTime % 60;
            gameTime /= 60;
            infoSS << "\nElapsed time:  " << gameTime << ":" << minutes << ":" << seconds;
        }
    }

    TextRenderer::getSingleton().setText("DebugMessages", infoSS.str());
}

void ODFrameListener::setCameraPosition(const Ogre::Vector3& position)
{
    mCameraManager.setCameraPosition(position);
}

void ODFrameListener::moveCamera(CameraManager::Direction direction)
{
    mCameraManager.move(direction);
}

void ODFrameListener::setActiveCameraNearClipDistance(Ogre::Real value)
{
    mCameraManager.getActiveCamera()->setNearClipDistance(value);
}

Ogre::Real ODFrameListener::getActiveCameraNearClipDistance()
{
    return mCameraManager.getActiveCamera()->getNearClipDistance();
}

void ODFrameListener::setActiveCameraFarClipDistance(Ogre::Real value)
{
    mCameraManager.getActiveCamera()->setFarClipDistance(value);
}

Ogre::Real ODFrameListener::getActiveCameraFarClipDistance()
{
    return mCameraManager.getActiveCamera()->getFarClipDistance();
}

Ogre::Vector3 ODFrameListener::getCameraViewTarget()
{
    return mCameraManager.getCameraViewTarget();
}

void ODFrameListener::cameraFlyTo(const Ogre::Vector3& destination)
{
    mCameraManager.flyTo(destination);
}
