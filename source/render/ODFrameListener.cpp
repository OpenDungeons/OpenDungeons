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

#include "modes/InputManager.h"
#include "modes/AbstractApplicationMode.h"
#include "network/ODServer.h"
#include "network/ServerNotification.h"
#include "network/ODClient.h"
#include "network/ChatMessage.h"
#include "gamemap/GameMap.h"
#include "gamemap/MiniMap.h"
#include "game/Player.h"
#include "render/TextRenderer.h"
#include "sound/MusicPlayer.h"
#include "render/RenderManager.h"
#include "ODApplication.h"
#include "utils/LogManager.h"
#include "modes/ModeManager.h"
#include "camera/CameraManager.h"
#include "game/Seat.h"

#include <OgreLogManager.h>
#include <OgreRenderWindow.h>
#include <OgreSceneManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/EventArgs.h>
#include <CEGUI/Window.h>
#include <CEGUI/Size.h>
#include <CEGUI/System.h>
#include <CEGUI/MouseCursor.h>

#include <boost/locale.hpp>
#include <boost/thread.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>


template<> ODFrameListener* Ogre::Singleton<ODFrameListener>::msSingleton = 0;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

/*! \brief This constructor is where the OGRE rendering system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ODFrameListener::ODFrameListener(Ogre::RenderWindow* renderWindow, Ogre::OverlaySystem* overLaySystem) :
    mInitialized(false),
    mWindow(renderWindow),
    mModeManager(nullptr),
    mShowDebugInfo(false),
    mContinue(true),
    mTerminalWordWrap(78),
    mChatMaxMessages(10),
    mChatMaxTimeDisplay(20.0f),
    mRaySceneQuery(nullptr),
    mGameMap(nullptr),
    mMiniMap(nullptr),
    mExitRequested(false),
    mCameraManager(nullptr),
    mIsChatInputMode(false)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating frame listener...", Ogre::LML_NORMAL);

    RenderManager* renderManager = new RenderManager(overLaySystem);

    mGameMap = new GameMap(false);

    mMiniMap = new MiniMap(mGameMap);

    mCameraManager = new CameraManager(renderManager->getSceneManager(), mGameMap, renderWindow);

    renderManager->createScene(mCameraManager->getViewport());

    mRaySceneQuery = renderManager->getSceneManager()->createRayQuery(Ogre::Ray());

    mModeManager = new ModeManager(renderWindow);

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

void ODFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
    if(rw == mWindow && mModeManager)
    {
        delete mModeManager;
        mModeManager = nullptr;
    }
}

ODFrameListener::~ODFrameListener()
{
    if (mInitialized)
        exitApplication();

    if (mModeManager)
        delete mModeManager;
    delete mCameraManager;
    mGameMap->clearAll();
    mGameMap->processDeletionQueues();
    delete mGameMap;
    delete mMiniMap;
    // We deinit it here since it was also created in this class.
    delete RenderManager::getSingletonPtr();

    std::deque<ChatMessage*>::iterator it = mChatMessages.end();
    while(mChatMessages.size() > 0)
    {
        ChatMessage* msg = *mChatMessages.begin();
        mChatMessages.erase(mChatMessages.begin());
        delete msg;
    }
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
    RenderManager::getSingletonPtr()->getSceneManager()->destroyQuery(mRaySceneQuery);

    Ogre::LogManager::getSingleton().logMessage("Remove listener registration", Ogre::LML_NORMAL);
    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    Ogre::LogManager::getSingleton().logMessage("Window closed", Ogre::LML_NORMAL);
    windowClosed(mWindow);

    Ogre::LogManager::getSingleton().logMessage("Frame listener uninitialization done.", Ogre::LML_NORMAL);
    mInitialized = false;
}

void ODFrameListener::updateAnimations(Ogre::Real timeSinceLastFrame)
{
    MusicPlayer::getSingleton().update(static_cast<float>(timeSinceLastFrame));
    RenderManager::getSingleton().updateRenderAnimations(timeSinceLastFrame);
    mGameMap->processDeletionQueues();

    mGameMap->updateAnimations(timeSinceLastFrame);
}

bool ODFrameListener::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if (mWindow->isClosed())
        return false;

    CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);
    CEGUI::System::getSingleton().getDefaultGUIContext().injectTimePulse(evt.timeSinceLastFrame);

    // Sleep to limit the framerate to the max value
    // Note: This 2.0 should be a 1.0 but this gives the correct result.
    // As sleep functions can't be too accurate themselves (as they do take time to compute after all)
    // Thus, giving some free time to them helps getting the correct behaviour...
    double frameDelay = (1.0 / ODApplication::MAX_FRAMES_PER_SECOND) - evt.timeSinceLastFrame;
    if (frameDelay > 0.0)
    {
        boost::this_thread::sleep_for(boost::chrono::duration<double>(frameDelay));
    }

    mModeManager->update(evt);

    int64_t currentTurn = mGameMap->getTurnNumber();

    if(!mExitRequested)
    {
        // Updates animations independant from the server new turn event
        updateAnimations(evt.timeSinceLastFrame);
    }

    mCameraManager->updateCameraFrameTime(evt.timeSinceLastFrame);
    mCameraManager->onFrameStarted();

    if((currentTurn != -1) && (mGameMap->getGamePaused()) && (!mExitRequested))
        return true;

    //If an exit has been requested, start cleaning up.
    if(mExitRequested == true || mContinue == false)
    {
        exitApplication();
        mContinue = false;
        return mContinue;
    }


    ODClient::getSingleton().processClientSocketMessages(*mGameMap);
    ODClient::getSingleton().processClientNotifications();

    refreshChat();

    return mContinue;
}

void ODFrameListener::updateMinimap()
{
    if (mMiniMap == nullptr)
        return;

    mMiniMap->updateCameraInfos(getCameraViewTarget(),
                                mCameraManager->getActiveCameraNode()->getOrientation().getRoll().valueRadians());
}

void ODFrameListener::refreshChat()
{
    std::stringstream chatSS;

    if(mIsChatInputMode)
    {
        chatSS << "\nTo all : " + mChatString;
    }

    uint32_t nbMsg = 0;
    std::deque<ChatMessage*>::iterator it = mChatMessages.end();
    while (it != mChatMessages.begin())
    {
        --it;
        ChatMessage* msg = *it;
        if ((msg->isMessageTooOld(mChatMaxTimeDisplay)) ||
            (nbMsg > mChatMaxMessages))
        {
            it = mChatMessages.erase(it);
            delete msg;
        }
        else
        {
            ++nbMsg;
        }
    }

    for (it = mChatMessages.begin(); it != mChatMessages.end(); ++it)
    {
        ChatMessage* msg = *it;
        chatSS << "\n" + msg->getClientNick() << ": " << msg->getMessage();
    }

    bool isEditor = (mModeManager->getCurrentModeType() == ModeManager::EDITOR);
    if (!isEditor && mGameMap->getTurnNumber() == -1)
    {
        // Tells the user the game is loading.
        chatSS << "\nWaiting for players...";
    }

    if (mShowDebugInfo)
    {
        chatSS << "\nFPS: " << mWindow->getStatistics().lastFPS;
        chatSS << "\ntriangleCount: " << mWindow->getStatistics().triangleCount;
        chatSS << "\nBatches: " << mWindow->getStatistics().batchCount;
        chatSS << "\nTurn number:  " <<  static_cast<int32_t>(mGameMap->getTurnNumber());
        if(ODClient::getSingleton().isConnected())
        {
            int32_t gameTime = ODClient::getSingleton().getGameTimeMillis() / 1000;
            int32_t seconds = gameTime % 60;
            gameTime /= 60;
            int32_t minutes = gameTime % 60;
            gameTime /= 60;
            chatSS << "\nElapsed time:  " << gameTime << ":" << minutes << ":" << seconds;
        }
    }

    std::string strDisplay = chatSS.str();
    if(strDisplay.size() > 0)
        printText("---------- Chat ----------" + strDisplay);
    else
        printText("");
}

void ODFrameListener::refreshPlayerDisplay(const std::string& goalsDisplayString)
{
    Seat* mySeat = mGameMap->getLocalPlayer()->getSeat();

    //! \brief Updates common info on screen.
    CEGUI::Window *tempWindow = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_TERRITORY);
    std::stringstream tempSS("");
    tempSS << mySeat->getNumClaimedTiles();
    tempWindow->setText(tempSS.str());

    tempWindow = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_GOLD);
    tempSS.str("");
    tempSS << mySeat->getGold();
    tempWindow->setText(tempSS.str());

    tempWindow = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_MANA);
    tempSS.str("");
    tempSS << mySeat->getMana() << " " << (mySeat->getManaDelta() >= 0 ? "+" : "-")
            << mySeat->getManaDelta();
    tempWindow->setText(tempSS.str());

    tempWindow = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(Gui::MESSAGE_WINDOW);

    tempWindow->setText(goalsDisplayString);
}

bool ODFrameListener::frameEnded(const Ogre::FrameEvent& evt)
{
    AbstractApplicationMode* currentMode = mModeManager->getCurrentMode();
    currentMode->onFrameEnded(evt);

    mCameraManager->onFrameEnded();

    return true;
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
    Ogre::Ray mouseRay = mCameraManager->getActiveCamera()->getCameraToViewportRay(mousePos.d_x / float(
            arg.state.width), mousePos.d_y / float(arg.state.height));
    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);

    // Execute query
    return mRaySceneQuery->execute();
}

void ODFrameListener::printText(const std::string& text)
{
    std::string tempString;
    tempString.reserve(text.size());
    int lineLength = 0;
    for (unsigned int i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            lineLength = 0;
        }

        if (lineLength < mTerminalWordWrap)
        {
            ++lineLength;
        }
        else
        {
            lineLength = 0;
            tempString += "\n";
        }

        tempString += text[i];
    }

    TextRenderer::getSingleton().setText("DebugMessages", tempString);
}

void ODFrameListener::notifyChatInputMode(bool isChatInputMode, bool sendChatMsg)
{
    if(mIsChatInputMode && sendChatMsg && !mChatString.empty() && ODClient::getSingleton().isConnected())
    {
        ClientNotification *clientNotification = new ClientNotification(
            ClientNotification::chat);
        clientNotification->mPacket << mChatString;
        ODClient::getSingleton().queueClientNotification(clientNotification);
    }
    mIsChatInputMode = isChatInputMode;
    mChatString.clear();
}

void ODFrameListener::notifyChatChar(int text)
{
    // Ogre::Overlay do not work with special characters. We have to convert
    // the String to make sure no such characters are used
    std::string str;
    str.append(1, (char)text);
    mChatString = mChatString + boost::locale::conv::to_utf<char>(
        str, "Ascii");
}

void ODFrameListener::notifyChatCharDel()
{
    if(mChatString.size() > 0)
        mChatString.resize(mChatString.size() - 1);
}

void ODFrameListener::addChatMessage(ChatMessage *message)
{
    mChatMessages.push_back(message);
}

void ODFrameListener::setCameraPosition(const Ogre::Vector3& position)
{
    mCameraManager->setCameraPosition(position);
}

void ODFrameListener::moveCamera(CameraManager::Direction direction)
{
    mCameraManager->move(direction);
}

void ODFrameListener::setActiveCameraNearClipDistance(Ogre::Real value)
{
    mCameraManager->getActiveCamera()->setNearClipDistance(value);
}

Ogre::Real ODFrameListener::getActiveCameraNearClipDistance()
{
    return mCameraManager->getActiveCamera()->getNearClipDistance();
}

void ODFrameListener::setActiveCameraFarClipDistance(Ogre::Real value)
{
    mCameraManager->getActiveCamera()->setFarClipDistance(value);
}

Ogre::Real ODFrameListener::getActiveCameraFarClipDistance()
{
    return mCameraManager->getActiveCamera()->getFarClipDistance();
}

const Ogre::Vector3 ODFrameListener::getCameraViewTarget()
{
    return mCameraManager->getCameraViewTarget();
}

void ODFrameListener::cameraFlyTo(const Ogre::Vector3& destination)
{
    mCameraManager->flyTo(destination);
}

void ODFrameListener::onMiniMapClick(int xPos, int yPos)
{
    Ogre::Vector2 cc = mMiniMap->camera_2dPositionFromClick(xPos, yPos);
    mCameraManager->onMiniMapClick(cc);
}
