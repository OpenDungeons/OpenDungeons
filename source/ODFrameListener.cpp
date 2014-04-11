/*!
 * \file   ODFrameListener.cpp
 * \date   09 April 2011
 * \author Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
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

/*! \brief This constructor is where the OGRE system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ODFrameListener::ODFrameListener(Ogre::RenderWindow* win, Ogre::OverlaySystem* tmpOverlaySystem) :
    cm(NULL),
    mWindow(win),
    renderManager(RenderManager::getSingletonPtr()),
    sfxHelper(SoundEffectsHelper::getSingletonPtr()),
    mShowDebugInfo(false),
    mContinue(true),
    terminalActive(false),
    terminalWordWrap(78),
    chatMaxMessages(10),
    chatMaxTimeDisplay(20),
    frameDelay(0.0),
    previousTurn(-1)
{
    LogManager* logManager = LogManager::getSingletonPtr();
    logManager->logMessage("Creating frame listener...", Ogre::LML_NORMAL);

    // Use Ogre::SceneType enum instead of string to identify the scene manager type; this is more robust!
    Ogre::SceneManager* sceneManager = ODApplication::getSingletonPtr()->getRoot()->createSceneManager(Ogre::ST_GENERIC,
                                                                                                       "SceneManager");
    sceneManager->addRenderQueueListener(tmpOverlaySystem);
    gameMap = new GameMap;

    cm = new CameraManager(sceneManager, gameMap);

    logManager->logMessage("Created everything :)", Ogre::LML_NORMAL);

    renderManager = new RenderManager();
    renderManager->setGameMap(gameMap);
    renderManager->setCameraManager(cm);
    renderManager->setViewport(cm->getViewport());

    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    rockSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Rock_scene_node");
    creatureSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Creature_scene_node");
    roomSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Room_scene_node");
    fieldSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Field_scene_node");
    lightSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Light_scene_node");
    mRaySceneQuery = mSceneMgr->createRayQuery(Ogre::Ray());

    modeManager = new ModeManager();
    cm->setModeManager(modeManager);

    //Set initial mouse clipping size
    windowResized(mWindow);

    //Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    TextRenderer::getSingleton().addTextBox(ODApplication::POINTER_INFO_STRING, "",
            0, 0, 200, 50, Ogre::ColourValue::White);

    renderManager->setSceneNodes(rockSceneNode, roomSceneNode, creatureSceneNode,
                                     lightSceneNode, fieldSceneNode);
    //Available team colours
    //red
    playerColourValues.push_back(Ogre::ColourValue(1.0, 0.0, 0.0, 1.0));
    //yellow
    playerColourValues.push_back(Ogre::ColourValue(1.0, 1.0, 0.0, 1.0));
    //green
    playerColourValues.push_back(Ogre::ColourValue(0.0, 1.0, 0.0, 1.0));
    //cyan
    playerColourValues.push_back(Ogre::ColourValue(0.0, 1.0, 1.0, 1.0));
    //blue
    playerColourValues.push_back(Ogre::ColourValue(0.0, 0.0, 1.0, 1.0));
    //violet
    playerColourValues.push_back(Ogre::ColourValue(1.0, 0.0, 1.0, 1.0));
    //white
    playerColourValues.push_back(Ogre::ColourValue(1.0, 1.0, 1.0, 1.0));
    //black
    playerColourValues.push_back(Ogre::ColourValue(0.5, 0.5, 0.5, 1.0));

    threadStopRequested.set(false);
    exitRequested.set(false);

    // Init the render manager
    try
    {
        logManager->logMessage("Creating scene...", Ogre::LML_NORMAL);
        renderManager->createScene();
        logManager->logMessage("Creating compositors...", Ogre::LML_NORMAL);
        renderManager->createCompositors();
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

    LogManager::getSingletonPtr()->logMessage("*** FrameListener initialized ***");
}

/*! \brief Adjust mouse clipping area
 *
 */
void ODFrameListener::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);

    const OIS::MouseState &ms = modeManager->getCurrentMode()->getMouse()->getMouseState();
    ms.width = width;
    ms.height = height;

    //Notify CEGUI that the display size has changed.
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size<float>(
            static_cast<float> (width), static_cast<float> (height)));
}

/*! \brief Unattach OIS before window shutdown (very important under Linux)
 *
 */
void ODFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
    if(rw == mWindow)
    {
        if(modeManager)
        {
            delete modeManager;
        }
    }
}

ODFrameListener::~ODFrameListener()
{
    if (modeManager)
        delete modeManager;
    delete cm;
    delete gameMap;
}

void ODFrameListener::requestExit()
{
    exitRequested.set(true);
}

void ODFrameListener::exitApplication()
{

    LogManager::getSingleton().logMessage("\nClosing down.");
    //Mark that we want the threads to stop.
    requestStopThreads();

    ServerNotification* exitServerNotification = new ServerNotification();
    exitServerNotification->type = ServerNotification::exit;
    sem_wait(&ServerNotification::serverNotificationQueueLockSemaphore);
    while(!ServerNotification::serverNotificationQueue.empty())
    {
        delete ServerNotification::serverNotificationQueue.front();
        ServerNotification::serverNotificationQueue.pop_front();
    }
    //serverNotificationQueue.push_back(exitServerNotification);
    sem_post(&ServerNotification::serverNotificationQueueLockSemaphore);
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
    pthread_join(serverNotificationThread, NULL);
    Ogre::LogManager::getSingleton().logMessage("Trying to close client notification thread..", Ogre::LML_NORMAL);
    //pthread_join(clientNotificationThread, NULL);
    //TODO - change this back to join when we know what causes this to lock up sometimes.
    pthread_cancel(clientNotificationThread);
    Ogre::LogManager::getSingleton().logMessage("Trying to close creature thread..", Ogre::LML_NORMAL);
    pthread_join(creatureThread, NULL);
    /* Cancel the rest of the threads.
     * NOTE:Threads should ideally not be cancelled, but told to exit instead.
     * However, these threads are blocking while waiting for network data.
     * This could be changed if we want a different behaviour later.
     */
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel client thread..", Ogre::LML_NORMAL);
    pthread_cancel(clientThread);
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel client handler threads..", Ogre::LML_NORMAL);
    //FIXME: Does the thread handles here actually need to be pointers?
    for(std::vector<pthread_t*>::iterator it = clientHandlerThreads.begin();
        it != clientHandlerThreads.end(); ++it)
    {
        pthread_cancel(*(*it));
    }
    Ogre::LogManager::getSingleton().logMessage("Trying to cancel server thread..", Ogre::LML_NORMAL);
    pthread_cancel(serverThread);
    Ogre::LogManager::getSingleton().logMessage("Clearing game map..", Ogre::LML_NORMAL);
    gameMap->clearAll();
    RenderManager::getSingletonPtr()->getSceneManager()->destroyQuery(mRaySceneQuery);

    //Remove ourself as a Window listener
    Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
    windowClosed(mWindow);
}

bool ODFrameListener::getThreadStopRequested()
{
    return threadStopRequested.get();
}

void ODFrameListener::setThreadStopRequested(bool value)
{
    threadStopRequested.set(value);
}

void ODFrameListener::requestStopThreads()
{
    threadStopRequested.set(true);
}

/*! \brief The main rendering function for the OGRE 3d environment.
 *
 * This function is the one which actually carries out all of the rendering in
 * the OGRE 3d system.  Since all the rendering must happen here, one of the
 * operations performed by this function is the processing of a request queue
 * full of RenderRequest structures.
 */
bool ODFrameListener::frameStarted(const Ogre::FrameEvent& evt)
{
    if (mWindow->isClosed())
        return false;

    CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);

    // Increment the number of threads locking this turn for the gameMap to allow for proper deletion of objects.
    //NOTE:  If this function exits early the corresponding unlock function must be called.
    long int currentTurnNumber = GameMap::turnNumber.get();

    gameMap->threadLockForTurn(currentTurnNumber);

    MusicPlayer::getSingletonPtr()->update();
    renderManager->processRenderRequests();

    string chatBaseString = "\n---------- Chat ----------\n";
    chatString = chatBaseString;

    // Delete any chat messages older than the maximum allowable age
    //TODO:  Lock this queue before doing this stuff
    time_t now;
    time(&now);
    ChatMessage* currentMessage;
    unsigned int i = 0;
    while (i < chatMessages.size())
    {
        std::deque<ChatMessage*>::iterator itr = chatMessages.begin() + i;
        currentMessage = *itr;
        if (difftime(now, currentMessage->getRecvTime()) > chatMaxTimeDisplay)
        {
            chatMessages.erase(itr);
        }
        else
        {
            ++i;
        }
    }

    // Only keep the N newest chat messages of the ones that remain
    while (chatMessages.size() > chatMaxMessages)
    {
        delete chatMessages.front();
        chatMessages.pop_front();
    }

    // Fill up the chat window with the arrival time and contents of all the chat messages left in the queue.
    for (unsigned int i = 0; i < chatMessages.size(); ++i)
    {
        std::stringstream chatSS("");
        struct tm *friendlyTime = localtime(&chatMessages[i]->getRecvTime());
        chatSS << friendlyTime->tm_hour << ":" << friendlyTime->tm_min << ":"
                << friendlyTime->tm_sec << "  " << chatMessages[i]->getClientNick()
                << ":  " << chatMessages[i]->getMessage();
        chatString += chatSS.str() + "\n";
    }

    // Display the terminal, the current turn number, and the
    // visible chat messages at the top of the screen
    string nullString = "";
    string turnString = "";
    turnString.reserve(100);

    //TODO - we should call printText only when the text changes.
    bool isEditor = (modeManager->getCurrentModeType() == ModeManager::EDITOR);
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
                    gameMap->averageAILeftoverTime)).substr(0, 4) + " s ";
            turnString += (gameMap->averageAILeftoverTime >= 0.0 ? "early" : "late");
            double maxTps = 1.0 / ((1.0 / ODApplication::turnsPerSecond)
                - gameMap->averageAILeftoverTime);
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
                    + "\n" + (!chatMessages.empty() ? chatString : nullString));
        }
        else
        {
            printText("");
        }
    }

    // Update the animations on any AnimatedObjects which have them
    for (unsigned int i = 0; i < gameMap->numAnimatedObjects(); ++i)
    {
        MovableGameEntity *currentAnimatedObject = gameMap->getAnimatedObject(i);

        // Advance the animation
        if (currentAnimatedObject->mAnimationState != NULL)
        {
            currentAnimatedObject->mAnimationState->addTime((Ogre::Real)(ODApplication::turnsPerSecond
                    * evt.timeSinceLastFrame
                    * currentAnimatedObject->getAnimationSpeedFactor()));
        }

        // Move the creature : TODO The code should be moved within the movable game entity class
        sem_wait(&currentAnimatedObject->mWalkQueueLockSemaphore);
        if (!currentAnimatedObject->mWalkQueue.empty())
        {
            // If the previously empty walk queue has had a destination added to it we need to rotate the creature to face its initial walk direction.
            if (currentAnimatedObject->mWalkQueueFirstEntryAdded)
            {
                currentAnimatedObject->mWalkQueueFirstEntryAdded = false;
                currentAnimatedObject->faceToward(
                        (int)currentAnimatedObject->mWalkQueue.front().x,
                        (int)currentAnimatedObject->mWalkQueue.front().y);
            }

            //FIXME: The moveDist should probably be tied to the scale of the creature as well
            //FIXME: When the client and the server are using different frame rates, the creatures walk at different speeds
            double moveDist = ODApplication::turnsPerSecond
                    * currentAnimatedObject->getMoveSpeed()
                    * evt.timeSinceLastFrame;
            currentAnimatedObject->mShortestDistance -= moveDist;

            // Check to see if we have walked to, or past, the first destination in the queue
            if (currentAnimatedObject->mShortestDistance <= 0.0)
            {
                // Compensate for any overshoot and place the creature at the intended destination
                currentAnimatedObject->setPosition(
                        currentAnimatedObject->mWalkQueue.front());
                currentAnimatedObject->mWalkQueue.pop_front();

                // If there are no more places to walk to still left in the queue
                if (currentAnimatedObject->mWalkQueue.empty())
                {
                    // Stop walking
                    currentAnimatedObject->stopWalking();
                }
                else // There are still entries left in the queue
                {
                    // Turn to face the next direction
                    currentAnimatedObject->faceToward((int)currentAnimatedObject->mWalkQueue.front().x,
                                                      (int)currentAnimatedObject->mWalkQueue.front().y);

                    // Compute the distance to the next location in the queue and store it in the shortDistance datamember.
                    Ogre::Vector3 tempVector =
                            currentAnimatedObject->mWalkQueue.front()
                                    - currentAnimatedObject->getPosition();
                    currentAnimatedObject->mShortestDistance
                            = tempVector.normalise();
                }
            }
            else // We have not reached the destination at the front of the queue
            {
                // Move the object closer to its destination by the amount it should travel this frame.
                currentAnimatedObject->setPosition(
                        currentAnimatedObject->getPosition()
                                + currentAnimatedObject->mWalkDirection
                                        * (Ogre::Real)moveDist);
            }
        }
        sem_post(&currentAnimatedObject->mWalkQueueLockSemaphore);
    }

    // Advance the "flickering" of the lights by the amount of time that has passed since the last frame.
    for (unsigned int i = 0; i < gameMap->numMapLights(); ++i)
    {
        MapLight *tempMapLight = gameMap->getMapLight(i);
        tempMapLight->advanceFlicker(evt.timeSinceLastFrame);
    }

    std::stringstream tempSS("");
    // Update the CEGUI displays of gold, mana, etc.
    if (isConnected())
    {
        //
        /*We only need to recreate the info windows when each turn when
        *the text updates.
        *TODO Update these only when needed.
        */
        if(previousTurn < currentTurnNumber)
        {
            previousTurn = currentTurnNumber;

            /*NOTE: currently running this in the main thread
            *Once per turn. We could put this in it's own thread
            *if proper locking is done.
            */
            gameMap->doPlayerAITurn(evt.timeSinceLastFrame);

            Seat *mySeat = gameMap->getLocalPlayer()->getSeat();

            CEGUI::Window *tempWindow
		= Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild( Gui::DISPLAY_TERRITORY );
            tempSS.str("");
            tempSS << mySeat->getNumClaimedTiles();
            tempWindow->setText(tempSS.str());

            tempWindow
               = Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild(  Gui::DISPLAY_GOLD );
            tempSS.str("");
            tempSS << mySeat->getGold();
            tempWindow->setText(tempSS.str());

            tempWindow
		= Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu)->getChild( Gui::DISPLAY_MANA  );
            tempSS.str("");
            tempSS << mySeat->getMana() << " " << (mySeat->getManaDelta() >= 0 ? "+" : "-")
                    << mySeat->getManaDelta();
            tempWindow->setText(tempSS.str());


            if (isConnected())// && gameMap->me->seat->getHasGoalsChanged())
            {
                mySeat->resetGoalsChanged();
                // Update the goals display in the message window.
                tempWindow =  Gui::getSingletonPtr()->getGuiSheet(Gui::inGameMenu) ->getChild( Gui::MESSAGE_WINDOW );
                tempSS.str("");
                bool iAmAWinner = gameMap->seatIsAWinner(mySeat);

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
                    tempSS << "\n\nCompleted Goals:\n---------------------\n";
                    for (unsigned int i = 0; i
                            < mySeat->numCompletedGoals(); ++i)
                    {
                        Goal *tempGoal = mySeat->getCompletedGoal(i);
                        tempSS << tempGoal->getSuccessMessage() << "\n";
                    }
                }

                if (mySeat->numFailedGoals() > 0)
                {
                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS
                            << "\n\nFailed Goals: (You cannot complete this level!)\n---------------------\n";
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
                            << gameMap->nextLevel;
                }
                tempWindow->setText(tempSS.str());
            }
        }
    }

    // Decrement the number of threads locking this turn for the gameMap to allow for proper deletion of objects.
    gameMap->threadUnlockForTurn(currentTurnNumber);

    //Need to capture/update each device
    modeManager->checkModeChange();
    AbstractApplicationMode* currentMode = modeManager->getCurrentMode();
    currentMode->getKeyboard()->capture();
    currentMode->getMouse()->capture();

    currentMode->onFrameStarted(evt);

    if (cm != NULL)
       cm->onFrameStarted();

    // Sleep to limit the framerate to the max value
    frameDelay -= evt.timeSinceLastFrame;
    if (frameDelay > 0.0)
    {
        usleep(1e6 * frameDelay);
    }
    else
    {
        //FIXME: I think this 2.0 should be a 1.0 but this gives the
        // correct result.  This probably indicates a bug.
        frameDelay += 2.0 / ODApplication::MAX_FRAMES_PER_SECOND;
    }

    //If an exit has been requested, start cleaning up.
    if(exitRequested.get() || mContinue == false)
    {
        exitApplication();
        mContinue = false;
    }

    return mContinue;
}

bool ODFrameListener::frameEnded(const Ogre::FrameEvent& evt)
{
    AbstractApplicationMode* currentMode = modeManager->getCurrentMode();
    currentMode->onFrameEnded(evt);

    if (cm != NULL)
        cm->onFrameEnded();

    return true;
}

/*! \brief Exit the game.
 *
 */
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
    //TODO: this exact function is also in InputManager, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}


/*! \brief Print a string in the upper left corner of the screen.
 *
 * Displays the given text on the screen starting in the upper-left corner.
 * This is the function which displays the text on the in game console.
 */
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

        if (lineLength < terminalWordWrap)
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
