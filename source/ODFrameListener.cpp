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
#include "Socket.h"
#include "ODServer.h"
#include "Creature.h"
#include "ChatMessage.h"
#include "Network.h"
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
#include "ClientNotification.h"
#include "CreatureAction.h"
#include "CreatureSound.h"
#include "ServerNotification.h"
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
ODFrameListener::ODFrameListener(Ogre::RenderWindow* win, Ogre::OverlaySystem* overlaySystem) :
    cm(NULL),
    mInitialized(false),
    mWindow(win),
    mShowDebugInfo(false),
    mContinue(true),
    mTerminalActive(false),
    mTerminalWordWrap(78),
    mChatMaxMessages(10),
    mChatMaxTimeDisplay(20),
    mFrameDelay(0.0),
    mPreviousTurn(-1)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating frame listener...", Ogre::LML_NORMAL);

    // Use Ogre::SceneType enum instead of string to identify the scene manager type; this is more robust!
    Ogre::SceneManager* sceneManager = ODApplication::getSingletonPtr()->getRoot()->createSceneManager(Ogre::ST_GENERIC,
                                                                                                       "SceneManager");
    sceneManager->addRenderQueueListener(overlaySystem);

    mGameMap = new GameMap;
    cm = new CameraManager(sceneManager, mGameMap);

    RenderManager* renderManager = new RenderManager();
    renderManager->setGameMap(mGameMap);
    renderManager->setCameraManager(cm);
    renderManager->setViewport(cm->getViewport());

    mRockSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode("Rock_scene_node");
    mCreatureSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode("Creature_scene_node");
    mRoomSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode("Room_scene_node");
    mFieldSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode("Field_scene_node");
    mLightSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode("Light_scene_node");
    mRaySceneQuery = sceneManager->createRayQuery(Ogre::Ray());

    mModeManager = new ModeManager();
    cm->setModeManager(mModeManager);

    //Set initial mouse clipping size
    windowResized(mWindow);

    //Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    TextRenderer::getSingleton().addTextBox(ODApplication::POINTER_INFO_STRING, "",
            0, 0, 200, 50, Ogre::ColourValue::White);

    renderManager->setSceneNodes(mRockSceneNode, mRoomSceneNode, mCreatureSceneNode,
                                 mLightSceneNode, mFieldSceneNode);

    // TODO: Move this into the common definitions files.
    //Available team colours
    //red
    mPlayerColourValues.push_back(Ogre::ColourValue(1.0, 0.0, 0.0, 1.0));
    //yellow
    mPlayerColourValues.push_back(Ogre::ColourValue(1.0, 1.0, 0.0, 1.0));
    //green
    mPlayerColourValues.push_back(Ogre::ColourValue(0.0, 1.0, 0.0, 1.0));
    //cyan
    mPlayerColourValues.push_back(Ogre::ColourValue(0.0, 1.0, 1.0, 1.0));
    //blue
    mPlayerColourValues.push_back(Ogre::ColourValue(0.0, 0.0, 1.0, 1.0));
    //violet
    mPlayerColourValues.push_back(Ogre::ColourValue(1.0, 0.0, 1.0, 1.0));
    //white
    mPlayerColourValues.push_back(Ogre::ColourValue(1.0, 1.0, 1.0, 1.0));
    //black
    mPlayerColourValues.push_back(Ogre::ColourValue(0.5, 0.5, 0.5, 1.0));

    mThreadStopRequested.set(false);
    mExitRequested.set(false);

    mInitialized = true;

    // Init the render manager
    try
    {
        logManager->logMessage("Creating scene...", Ogre::LML_NORMAL);
        renderManager->createScene();
        logManager->logMessage("Creating compositors...", Ogre::LML_NORMAL);
        renderManager->createCompositors();
        logManager->logMessage("*** FrameListener initialized ***");
    }
    catch(Ogre::Exception& e)
    {
        ODApplication::displayErrorMessage("Ogre exception when initialising the render manager:\n"
            + e.getFullDescription(), false);
        requestExit();
    }
    catch (std::exception& e)
    {
        ODApplication::displayErrorMessage("Exception when initialising the render manager:\n"
            + std::string(e.what()), false);
        requestExit();
    }
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
    // We deinit it here since it was also created in this class.
    delete RenderManager::getSingletonPtr();
}

void ODFrameListener::requestExit()
{
    mExitRequested.set(true);
}

void ODFrameListener::exitApplication()
{
    LogManager::getSingleton().logMessage("\nClosing down.");
    //Mark that we want the threads to stop.
    requestStopThreads();

    ServerNotification* exitServerNotification = new ServerNotification();
    exitServerNotification->type = ServerNotification::exit;
    sem_wait(&ServerNotification::mServerNotificationQueueLockSemaphore);
    while(!ServerNotification::serverNotificationQueue.empty())
    {
        delete ServerNotification::serverNotificationQueue.front();
        ServerNotification::serverNotificationQueue.pop_front();
    }
    //serverNotificationQueue.push_back(exitServerNotification);
    sem_post(&ServerNotification::mServerNotificationQueueLockSemaphore);
    ODServer::queueServerNotification(exitServerNotification);

    ClientNotification* exitClientNotification = new ClientNotification();
    exitClientNotification->mType = ClientNotification::exit;
    //TODO: There should be a function to do this.
    sem_wait(&ClientNotification::mClientNotificationQueueLockSemaphore);
    //Empty the queue so we don't get any crashes here.
    while(!ClientNotification::mClientNotificationQueue.empty())
    {
        delete ClientNotification::mClientNotificationQueue.front();
        ClientNotification::mClientNotificationQueue.pop_front();
    }
    ClientNotification::mClientNotificationQueue.push_back(exitClientNotification);
    sem_post(&ClientNotification::mClientNotificationQueueLockSemaphore);

    //Wait for threads to exit
    //TODO: Add a timeout here.
    Ogre::LogManager::getSingleton().logMessage("Trying to close server notification thread..", Ogre::LML_NORMAL);
    pthread_join(mServerNotificationThread, NULL);
    Ogre::LogManager::getSingleton().logMessage("Trying to close client notification thread..", Ogre::LML_NORMAL);
    //pthread_join(clientNotificationThread, NULL);
    //TODO - change this back to join when we know what causes this to lock up sometimes.
    pthread_cancel(mClientNotificationThread);
    Ogre::LogManager::getSingleton().logMessage("Trying to close creature thread..", Ogre::LML_NORMAL);
    pthread_join(mCreatureThread, NULL);
    /* Cancel the rest of the threads.
     * NOTE:Threads should ideally not be cancelled, but told to exit instead.
     * However, these threads are blocking while waiting for network data.
     * This could be changed if we want a different behaviour later.
     */
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel client thread..", Ogre::LML_NORMAL);
    pthread_cancel(mClientThread);
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel client handler threads..", Ogre::LML_NORMAL);
    //FIXME: Does the thread handles here actually need to be pointers?
    for(std::vector<pthread_t*>::iterator it = mClientHandlerThreads.begin();
        it != mClientHandlerThreads.end(); ++it)
    {
        pthread_cancel(*(*it));
    }
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel server thread..", Ogre::LML_NORMAL);
    pthread_cancel(mServerThread);
    Ogre::LogManager::getSingleton().logMessage("Clearing game map..", Ogre::LML_NORMAL);
    mGameMap->clearAll();
    RenderManager::getSingletonPtr()->getSceneManager()->destroyQuery(mRaySceneQuery);

    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);

    mInitialized = false;
}

bool ODFrameListener::getThreadStopRequested()
{
    return mThreadStopRequested.get();
}

void ODFrameListener::setThreadStopRequested(bool value)
{
    mThreadStopRequested.set(value);
}

void ODFrameListener::requestStopThreads()
{
    mThreadStopRequested.set(true);
}

bool ODFrameListener::frameStarted(const Ogre::FrameEvent& evt)
{
    if (mWindow->isClosed())
        return false;

    CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);

    // Increment the number of threads locking this turn for the gameMap to allow for proper deletion of objects.
    //NOTE:  If this function exits early the corresponding unlock function must be called.
    long int currentTurnNumber = GameMap::turnNumber.get();

    mGameMap->threadLockForTurn(currentTurnNumber);

    MusicPlayer::getSingletonPtr()->update();
    RenderManager::getSingletonPtr()->processRenderRequests();

    std::string chatBaseString = "\n---------- Chat ----------\n";
    mChatString = chatBaseString;

    // Delete any chat messages older than the maximum allowable age
    //TODO:  Lock this queue before doing this stuff
    time_t now;
    time(&now);
    ChatMessage* currentMessage;
    unsigned int i = 0;
    while (i < mChatMessages.size())
    {
        std::deque<ChatMessage*>::iterator itr = mChatMessages.begin() + i;
        currentMessage = *itr;
        if (difftime(now, currentMessage->getRecvTime()) > mChatMaxTimeDisplay)
        {
            mChatMessages.erase(itr);
        }
        else
        {
            ++i;
        }
    }

    // Only keep the N newest chat messages of the ones that remain
    while (mChatMessages.size() > mChatMaxMessages)
    {
        delete mChatMessages.front();
        mChatMessages.pop_front();
    }

    // Fill up the chat window with the arrival time and contents of all the chat messages left in the queue.
    for (unsigned int i = 0; i < mChatMessages.size(); ++i)
    {
        std::stringstream chatSS("");
        struct tm *friendlyTime = localtime(&mChatMessages[i]->getRecvTime());
        chatSS << friendlyTime->tm_hour << ":" << friendlyTime->tm_min << ":"
                << friendlyTime->tm_sec << "  " << mChatMessages[i]->getClientNick()
                << ":  " << mChatMessages[i]->getMessage();
        mChatString += chatSS.str() + "\n";
    }

    // Display the terminal, the current turn number, and the
    // visible chat messages at the top of the screen
    string nullString = "";
    string turnString = "";
    turnString.reserve(100);

    //TODO - we should call printText only when the text changes.
    bool isEditor = (mModeManager->getCurrentModeType() == ModeManager::EDITOR);
    if (!isEditor && Socket::serverSocket == NULL)
    {
        // Tells the user the game is loading.
        printText("\nLoading...");
    }
    else
    {
        if (mShowDebugInfo)
        {
            turnString = "On average the creature AI is finishing ";
            turnString += Ogre::StringConverter::toString((Ogre::Real) fabs(
                    mGameMap->averageAILeftoverTime)).substr(0, 4) + " s ";
            turnString += (mGameMap->averageAILeftoverTime >= 0.0 ? "early" : "late");
            double maxTps = 1.0 / ((1.0 / ODApplication::turnsPerSecond)
                - mGameMap->averageAILeftoverTime);
            turnString += "\nMax tps est. at " + Ogre::StringConverter::toString(
                static_cast<Ogre::Real>(maxTps)).substr(0, 4);
            turnString += "\nFPS: " + Ogre::StringConverter::toString(
                mWindow->getStatistics().lastFPS);
            turnString += "\ntriangleCount: " + Ogre::StringConverter::toString(
                mWindow->getStatistics().triangleCount);
            turnString += "\nBatches: " + Ogre::StringConverter::toString(
                mWindow->getStatistics().batchCount);
            turnString += "\nTurn number:  "
                + Ogre::StringConverter::toString(GameMap::turnNumber.get());

            printText(ODApplication::MOTD + "\n" + turnString
                    + "\n" + (!mChatMessages.empty() ? mChatString : nullString));
        }
        else
        {
            printText("");
        }
    }

    // Update the animations on any AnimatedObjects which have them
    for (unsigned int i = 0; i < mGameMap->numAnimatedObjects(); ++i)
    {
        MovableGameEntity* currentAnimatedObject = mGameMap->getAnimatedObject(i);

        if (!currentAnimatedObject)
            continue;

        currentAnimatedObject->update(evt.timeSinceLastFrame);
    }

    // Advance the "flickering" of the lights by the amount of time that has passed since the last frame.
    for (unsigned int i = 0; i < mGameMap->numMapLights(); ++i)
    {
        MapLight* tempMapLight = mGameMap->getMapLight(i);
        tempMapLight->advanceFlicker(evt.timeSinceLastFrame);
    }

    std::stringstream tempSS("");
    // Update the CEGUI displays of gold, mana, etc.
    if (isConnected())
    {
        /*
        * We only need to recreate the info windows when each turn when
        * the text updates.
        * TODO Update these only when needed.
        */
        if(mPreviousTurn < currentTurnNumber)
        {
            mPreviousTurn = currentTurnNumber;

            /*
            * NOTE: currently running this in the main thread
            *Once per turn. We could put this in it's own thread
            *if proper locking is done.
            */
            mGameMap->doPlayerAITurn(evt.timeSinceLastFrame);

            Seat* mySeat = mGameMap->getLocalPlayer()->getSeat();

            //! \brief Updates common info on screen.
            CEGUI::Window *tempWindow = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(Gui::DISPLAY_TERRITORY);
            tempSS.str("");
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

            if (isConnected())// && gameMap->me->seat->getHasGoalsChanged())
            {
                mySeat->resetGoalsChanged();
                // Update the goals display in the message window.
                tempWindow =  Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu) ->getChild(Gui::MESSAGE_WINDOW);
                tempSS.str("");
                bool iAmAWinner = mGameMap->seatIsAWinner(mySeat);

                if (mySeat->numGoals() > 0)
                {
                    // Loop over the list of unmet goals for the seat we are sitting in an print them.
                    tempSS << "Unfinished Goals:\n---------------------\n";
                    for (unsigned int i = 0; i < mySeat->numGoals(); ++i)
                    {
                        Goal *tempGoal = mySeat->getGoal(i);
                        tempSS << tempGoal->getDescription() << "\n";
                    }
                }

                if (mySeat->numCompletedGoals() > 0)
                {
                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS << "\nCompleted Goals:\n---------------------\n";
                    for (unsigned int i = 0; i < mySeat->numCompletedGoals(); ++i)
                    {
                        Goal *tempGoal = mySeat->getCompletedGoal(i);
                        tempSS << tempGoal->getSuccessMessage() << "\n";
                    }
                }

                if (mySeat->numFailedGoals() > 0)
                {
                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS << "\nFailed Goals: (You cannot complete this level!)\n---------------------\n";
                    for (unsigned int i = 0; i < mySeat->numFailedGoals(); ++i)
                    {
                        Goal *tempGoal = mySeat->getFailedGoal(i);
                        tempSS << tempGoal->getFailedMessage() << "\n";
                    }
                }

                if (iAmAWinner)
                {
                    tempSS
                            << "\nCongratulations, you have completed this level.\nOpen the terminal and run the \'next\'\n";
                    tempSS
                            << "command to move on to move on to the next level.\n\nThe next level is:  "
                            << mGameMap->nextLevel;
                }
                tempWindow->setText(tempSS.str());
            }
        }
    }

    // Decrement the number of threads locking this turn for the gameMap to allow for proper deletion of objects.
    mGameMap->threadUnlockForTurn(currentTurnNumber);

    //Need to capture/update each device
    mModeManager->checkModeChange();
    AbstractApplicationMode* currentMode = mModeManager->getCurrentMode();
    currentMode->getKeyboard()->capture();
    currentMode->getMouse()->capture();

    currentMode->onFrameStarted(evt);

    if (cm != NULL)
       cm->onFrameStarted();

    // Sleep to limit the framerate to the max value
    mFrameDelay -= evt.timeSinceLastFrame;
    if (mFrameDelay > 0.0)
    {
        usleep(1e6 * mFrameDelay);
    }
    else
    {
        //FIXME: I think this 2.0 should be a 1.0 but this gives the
        // correct result.  This probably indicates a bug.
        mFrameDelay += 2.0 / ODApplication::MAX_FRAMES_PER_SECOND;
    }

    //If an exit has been requested, start cleaning up.
    if(mExitRequested.get() || mContinue == false)
    {
        exitApplication();
        mContinue = false;
    }

    return mContinue;
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
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
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

void ODFrameListener::addChatMessage(ChatMessage *message)
{
    mChatMessages.push_back(message);
}
