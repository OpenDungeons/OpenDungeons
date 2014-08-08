/*!
 * \file   ODFrameListener.cpp
 * \date   09 April 2011
 * \author Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 *
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

#include "ODFrameListener.h"

#include "Console.h"
#include "ODServer.h"
#include "ServerNotification.h"
#include "ODClient.h"
#include "Creature.h"
#include "ChatMessage.h"
#include "Sleep.h"
#include "BattleField.h"
#include "Trap.h"
#include "GameMap.h"
#include "MiniMap.h"
#include "Player.h"
#include "SoundEffectsHelper.h"
#include "Weapon.h"
#include "MapLight.h"
#include "AllGoals.h"
#include "CreatureAction.h"
#include "CreatureSound.h"
#include "TextRenderer.h"
#include "MusicPlayer.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "ODApplication.h"
#include "LogManager.h"
#include "ModeManager.h"
#include "CameraManager.h"
#include "MapLoader.h"
#include "Seat.h"
#include "ASWrapper.h"

#include <OgreLogManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/EventArgs.h>
#include <CEGUI/Window.h>
#include <CEGUI/Size.h>
#include <CEGUI/System.h>
#include <CEGUI/MouseCursor.h>

#include <iostream>
#include <algorithm>
#include <cstdlib>

template<> ODFrameListener* Ogre::Singleton<ODFrameListener>::msSingleton = 0;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf_is_banned_in_OD_code _snprintf
#endif

/*! \brief This constructor is where the OGRE rendering system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ODFrameListener::ODFrameListener(Ogre::RenderWindow* win) :
    cm(NULL),
    mInitialized(false),
    mWindow(win),
    mShowDebugInfo(false),
    mContinue(true),
    mTerminalActive(false),
    mTerminalWordWrap(78),
    mChatMaxMessages(10),
    mChatMaxTimeDisplay(20),
    mIsChatInputMode(false),
    mExitRequested(false)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating frame listener...", Ogre::LML_NORMAL);

    RenderManager* renderManager = new RenderManager();

    mGameMap = new GameMap(false);

    mMiniMap = new MiniMap(mGameMap);

    cm = new CameraManager(renderManager->getSceneManager(), mGameMap);

    renderManager->setGameMap(mGameMap);
    renderManager->createScene(cm->getViewport());

    mRaySceneQuery = renderManager->getSceneManager()->createRayQuery(Ogre::Ray());

    mModeManager = new ModeManager();
    cm->setModeManager(mModeManager);

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

    const OIS::MouseState &ms = mModeManager->getCurrentMode()->getMouse()->getMouseState();
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
        mModeManager = NULL;
    }
}

ODFrameListener::~ODFrameListener()
{
    if (mInitialized)
        exitApplication();

    if (mModeManager)
        delete mModeManager;
    delete cm;
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
    RenderManager::getSingletonPtr()->processRenderRequests();
    mGameMap->clearAll();
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
    MusicPlayer::getSingletonPtr()->update();
    RenderManager::getSingletonPtr()->processRenderRequests();

    mGameMap->updateAnimations(timeSinceLastFrame);
}

bool ODFrameListener::frameStarted(const Ogre::FrameEvent& evt)
{
    if (mWindow->isClosed())
        return false;

    CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);

    // Sleep to limit the framerate to the max value
    // FIXME: This 2.0 should be a 1.0 but this gives the correct result.  This probably indicates a bug.
    double frameDelay = (2.0 / ODApplication::MAX_FRAMES_PER_SECOND) - evt.timeSinceLastFrame;
    if (frameDelay > 0.0)
    {
        OD_USLEEP(1e6 * frameDelay);
    }

    //Need to capture/update each device
    mModeManager->checkModeChange();

    // If game is started, we update the game
    AbstractApplicationMode* currentMode = mModeManager->getCurrentMode();

    if ((mGameMap->getTurnNumber() != -1) || (!currentMode->waitForGameStart()))
    {
        if(!mExitRequested)
        {
            // Updates animations independant from the server new turn event
            updateAnimations(evt.timeSinceLastFrame);
        }

        currentMode->getKeyboard()->capture();
        currentMode->getMouse()->capture();

        currentMode->onFrameStarted(evt);

        if (cm != NULL)
           cm->onFrameStarted();

        if((mGameMap->getGamePaused()) && (!mExitRequested))
            return true;
    }

    //If an exit has been requested, start cleaning up.
    if(mExitRequested == true || mContinue == false)
    {
        exitApplication();
        mContinue = false;
        return mContinue;
    }

    ODClient::getSingleton().processClientSocketMessages();
    ODClient::getSingleton().processClientNotifications();

    refreshChat();

    return mContinue;
}

void ODFrameListener::refreshChat()
{
    std::stringstream chatSS;

    if(mIsChatInputMode)
    {
        chatSS << "\nTo all : " + mChatString;
    }

    int nbMsg = 0;
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

    if (cm != NULL)
        cm->onFrameEnded();

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
    Ogre::Ray mouseRay = cm->getActiveCamera()->getCameraToViewportRay(mousePos.d_x / float(
            arg.state.width), mousePos.d_y / float(arg.state.height));
    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);

    // Execute query
    return mRaySceneQuery->execute();
}

bool ODFrameListener::isConnected()
{
    return (ODServer::getSingleton().isConnected() || ODClient::getSingleton().isConnected());
}

bool ODFrameListener::isServer()
{
    return (ODServer::getSingleton().isConnected());
}

bool ODFrameListener::isClient()
{
    return (ODClient::getSingleton().isConnected());
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

void ODFrameListener::notifyChatInputMode(bool isChatInputMode)
{
    mIsChatInputMode = isChatInputMode;
    mChatString.clear();
}

void ODFrameListener::notifyChatChar(const OIS::KeyEvent &arg)
{
    if(arg.key == OIS::KC_RETURN)
    {
        if(!mChatString.empty() && ODClient::getSingleton().isConnected())
        {
            // Send a message to the server telling it we want to drop the creature
            ClientNotification *clientNotification = new ClientNotification(
                ClientNotification::chat);
            clientNotification->packet << mChatString;
            ODClient::getSingleton().queueClientNotification(clientNotification);
        }

        mChatString.clear();
    }
    else if(arg.key == OIS::KC_ESCAPE)
    {
        mChatString.clear();
    }
    else if(arg.key == OIS::KC_BACK)
    {
        if(mChatString.size() > 0)
            mChatString.resize(mChatString.size() - 1);
    }
    else
    {
        mChatString.append(1, (char)arg.text);
    }
}

void ODFrameListener::addChatMessage(ChatMessage *message)
{
    mChatMessages.push_back(message);
}


