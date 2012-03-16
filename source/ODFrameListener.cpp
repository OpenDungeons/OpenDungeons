/*!
 * \file   ODFrameListener.cpp
 * \date   09 April 2011
 * \author Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 */

#include <iostream>
#include <algorithm>

#include <CEGUI.h>

#include "Socket.h"
#include "Functions.h"
#include "Creature.h"
#include "ChatMessage.h"
#include "Network.h"
#include "Sleep.h"
#include "Field.h"
#include "Trap.h"
#include "GameMap.h"
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
#include "GameState.h"
#include "LogManager.h"
#include "InputManager.h"
#include "CameraManager.h"
#include "MapLoader.h"
#include "Seat.h"

#include "ODFrameListener.h"

template<> ODFrameListener*
        Ogre::Singleton<ODFrameListener>::ms_Singleton = 0;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define snprintf _snprintf
#endif

/*! \brief This constructor is where the OGRE system is initialized and started.
 *
 * The primary function of this routine is to initialize variables, and start
 * up the OGRE system.
 */
ODFrameListener::ODFrameListener(Ogre::RenderWindow* win, GameMap* gameMap) :
        mWindow(win),
        renderManager(RenderManager::getSingletonPtr()),
        sfxHelper(SoundEffectsHelper::getSingletonPtr()),
        mContinue(true),
        terminalActive(false),
        terminalWordWrap(78),
        chatMaxMessages(10),
        chatMaxTimeDisplay(20),
        frameDelay(0.0),
        previousTurn(-1),
        gameMap(gameMap)
{

    //FIXME: this should be changed to a function or something.
    gameMap->me = new Player();
    gameMap->me->setNick("defaultNickName");
    gameMap->me->setGameMap(gameMap);
    
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
    creatureSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Creature_scene_node");
    roomSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Room_scene_node");
    fieldSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Field_scene_node");
    lightSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
            "Light_scene_node");
    mRaySceneQuery = mSceneMgr->createRayQuery(Ogre::Ray());

    inputManager = new InputManager(gameMap);

    //Set initial mouse clipping size
    windowResized(mWindow);

    //Register as a Window listener
    Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);

    TextRenderer::getSingleton().addTextBox(ODApplication::POINTER_INFO_STRING, "",
            0, 0, 200, 50, Ogre::ColourValue::White);

    renderManager = RenderManager::getSingletonPtr();
    renderManager->setSceneNodes(roomSceneNode, creatureSceneNode,
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

    const OIS::MouseState &ms = inputManager->getMouse()->getMouseState();
    ms.width = width;
    ms.height = height;

    //Notify CEGUI that the display size has changed.
    CEGUI::System::getSingleton().notifyDisplaySizeChanged(CEGUI::Size(
            static_cast<float> (width), static_cast<float> (height)));
}

/*! \brief Unattach OIS before window shutdown (very important under Linux)
 *
 */
void ODFrameListener::windowClosed(Ogre::RenderWindow* rw)
{
    if(rw == mWindow)
    {
        if(inputManager)
        {
            delete inputManager;
        }
    }
}

ODFrameListener::~ODFrameListener()
{

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
    queueServerNotification(exitServerNotification);
    
    ClientNotification* exitClientNotification = new ClientNotification();
    exitClientNotification->type = ClientNotification::exit;
    //TODO: There should be a function to do this.
    sem_wait(&ClientNotification::clientNotificationQueueLockSemaphore);
    //Empty the queue so we don't get any crashes here.
    while(!ClientNotification::clientNotificationQueue.empty())
    {
        delete ClientNotification::clientNotificationQueue.front();
        ClientNotification::clientNotificationQueue.pop_front();
    }
    ClientNotification::clientNotificationQueue.push_back(exitClientNotification);
    sem_post(&ClientNotification::clientNotificationQueueLockSemaphore);

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
    if (Socket::serverSocket != 0)
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
    }
    turnString += "\nTurn number:  " + Ogre::StringConverter::toString(
            GameMap::turnNumber.get());
    //TODO - we shouldn't have to reprint this every frame.
    printText(ODApplication::MOTD + "\n" + (terminalActive ? (commandOutput + "\n")
            : nullString) + (terminalActive ? prompt : nullString)
            + (terminalActive ? promptCommand : nullString) + "\n" + turnString
            + "\n" + (!chatMessages.empty() ? chatString : nullString));

    // Update the animations on any AnimatedObjects which have them
    for (unsigned int i = 0; i < gameMap->numAnimatedObjects(); ++i)
    {
        MovableGameEntity *currentAnimatedObject = gameMap->getAnimatedObject(i);

        // Advance the animation
        if (currentAnimatedObject->animationState != NULL)
        {
            currentAnimatedObject->animationState->addTime(ODApplication::turnsPerSecond
                    * evt.timeSinceLastFrame
                    * currentAnimatedObject->getAnimationSpeedFactor());
        }

        // Move the creature
        sem_wait(&currentAnimatedObject->walkQueueLockSemaphore);
        if (!currentAnimatedObject->walkQueue.empty())
        {
            // If the previously empty walk queue has had a destination added to it we need to rotate the creature to face its initial walk direction.
            if (currentAnimatedObject->walkQueueFirstEntryAdded)
            {
                currentAnimatedObject->walkQueueFirstEntryAdded = false;
                currentAnimatedObject->faceToward(
                        currentAnimatedObject->walkQueue.front().x,
                        currentAnimatedObject->walkQueue.front().y);
            }

            //FIXME: The moveDist should probably be tied to the scale of the creature as well
            //FIXME: When the client and the server are using different frame rates, the creatures walk at different speeds
            double moveDist = ODApplication::turnsPerSecond
                    * currentAnimatedObject->getMoveSpeed()
                    * evt.timeSinceLastFrame;
            currentAnimatedObject->shortDistance -= moveDist;

            // Check to see if we have walked to, or past, the first destination in the queue
            if (currentAnimatedObject->shortDistance <= 0.0)
            {
                // Compensate for any overshoot and place the creature at the intended destination
                currentAnimatedObject->setPosition(
                        currentAnimatedObject->walkQueue.front());
                currentAnimatedObject->walkQueue.pop_front();

                // If there are no more places to walk to still left in the queue
                if (currentAnimatedObject->walkQueue.empty())
                {
                    // Stop walking
                    currentAnimatedObject->stopWalking();
                }
                else // There are still entries left in the queue
                {
                    // Turn to face the next direction
                    currentAnimatedObject->faceToward(
                            currentAnimatedObject->walkQueue.front().x,
                            currentAnimatedObject->walkQueue.front().y);

                    // Compute the distance to the next location in the queue and store it in the shortDistance datamember.
                    Ogre::Vector3 tempVector =
                            currentAnimatedObject->walkQueue.front()
                                    - currentAnimatedObject->getPosition();
                    currentAnimatedObject->shortDistance
                            = tempVector.normalise();
                }
            }
            else // We have not reached the destination at the front of the queue
            {
                // Move the object closer to its destination by the amount it should travel this frame.
                currentAnimatedObject->setPosition(
                        currentAnimatedObject->getPosition()
                                + currentAnimatedObject->walkDirection
                                        * moveDist);
            }
        }
        sem_post(&currentAnimatedObject->walkQueueLockSemaphore);
    }

    // Advance the "flickering" of the lights by the amount of time that has passed since the last frame.
    for (unsigned int i = 0; i < gameMap->numMapLights(); ++i)
    {
        MapLight *tempMapLight = gameMap->getMapLight(i);
        tempMapLight->advanceFlicker(evt.timeSinceLastFrame);
    }

    std::stringstream tempSS("");
    // Update the CEGUI displays of gold, mana, etc.
    if (isInGame())
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

            CEGUI::WindowManager *windowManager =
                    CEGUI::WindowManager::getSingletonPtr();

            CEGUI::Window *tempWindow = windowManager->getWindow(
                    (CEGUI::utf8*) "Root/TerritoryDisplay");
            tempSS.str("");
            tempSS << mySeat->getNumClaimedTiles();
            tempWindow->setText(tempSS.str());

            tempWindow
                    = windowManager->getWindow((CEGUI::utf8*) "Root/GoldDisplay");
            tempSS.str("");
            tempSS << mySeat->getGold();
            tempWindow->setText(tempSS.str());

            tempWindow
                    = windowManager->getWindow((CEGUI::utf8*) "Root/ManaDisplay");
            tempSS.str("");
            tempSS << mySeat->getMana() << " " << (mySeat->getManaDelta() >= 0 ? "+" : "-")
                    << mySeat->getManaDelta();
            tempWindow->setText(tempSS.str());


            if (isInGame())// && gameMap->me->seat->getHasGoalsChanged())
            {
                mySeat->resetGoalsChanged();
                // Update the goals display in the message window.
                tempWindow = windowManager->getWindow(
                        (CEGUI::utf8*) "Root/MessagesDisplayWindow");
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
    inputManager->getKeyboard()->capture();
    inputManager->getMouse()->capture();

    CameraManager::getSingleton().moveCamera(evt.timeSinceLastFrame);

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

Ogre::RaySceneQueryResult& ODFrameListener::doRaySceneQuery(
        const OIS::MouseEvent &arg)
{
    // Setup the ray scene query, use CEGUI's mouse position
    CEGUI::Point mousePos = CEGUI::MouseCursor::getSingleton().getPosition();// * mMouseScale;
    Ogre::Ray mouseRay = CameraManager::getSingleton().getCamera()->getCameraToViewportRay(mousePos.d_x / float(
            arg.state.width), mousePos.d_y / float(arg.state.height));
    mRaySceneQuery->setRay(mouseRay);
    mRaySceneQuery->setSortByDistance(true);

    // Execute query
    return mRaySceneQuery->execute();
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

//TODO: make rest of commands scriptable
/*! \brief Process the commandline from the terminal and carry out the actions
 *  specified in by the user.
 */
bool ODFrameListener::executePromptCommand(const std::string& command, std::string arguments)
{
    std::stringstream tempSS;

    /*
    // Exit the program
    if (command.compare("quit") == 0 || command.compare("exit") == 0)
    {
        //NOTE: converted to AS
        requestExit();
    }

    // Repeat the arguments of the command back to you
    else if (command.compare("echo") == 0)
    {
        //NOTE: dropped in AS (was this any useful?)
        commandOutput += "\n" + arguments + "\n";
    } */

    /*
    // Write the current level out to file specified as an argument
    if (command.compare("save") == 0)
    {
        //NOTE: convetred to AS
        if (arguments.empty())
        {
            commandOutput
                    += "No level name given: saving over the last loaded level: "
                            + gameMap->getLevelFileName() + "\n\n";
            arguments = gameMap->getLevelFileName();
        }

        string tempFileName = "levels/" + arguments + ".level";
        MapLoader::writeGameMapToFile(tempFileName, *gameMap);
        commandOutput += "\nFile saved to   " + tempFileName + "\n";

        gameMap->setLevelFileName(arguments);
    }*/

    // Clear the current level and load a new one from a file
    if (command.compare("load") == 0)
    {
        if (arguments.empty())
        {
            commandOutput
                    += "No level name given: loading the last loaded level: "
                            + gameMap->getLevelFileName() + "\n\n";
            arguments = gameMap->getLevelFileName();
        }

        if (Socket::clientSocket == NULL)
        {
            /* If the starting point of the string found is equal to the size
             * of the level name minus the extension (.level)
             */
            string tempString = "levels/" + arguments;
            if(arguments.find(".level") != (arguments.size() - 6))
            {
                tempString += ".level";
            }

            if (Socket::serverSocket != NULL)
            {
                gameMap->nextLevel = tempString;
                gameMap->loadNextLevel = true;
            }
            else
            {
                if (MapLoader::readGameMapFromFile(tempString, *gameMap))
                {
                    tempSS << "Successfully loaded file:  " << tempString
                            << "\nNum tiles:  " << gameMap->numTiles()
                            << "\nNum classes:  "
                            << gameMap->numClassDescriptions()
                            << "\nNum creatures:  " << gameMap->numCreatures();
                    commandOutput += tempSS.str();

                    gameMap->createAllEntities();
                }
                else
                {
                    tempSS << "ERROR: Could not load game map \'" << tempString
                            << "\'.";
                    commandOutput += tempSS.str();
                }
            }

            gameMap->setLevelFileName(arguments);
        }
        else
        {
            commandOutput
                    += "ERROR:  Cannot load a level if you are a client, only the sever can load new levels.";
        }
    }

    // Set the ambient light color
    else if (command.compare("ambientlight") == 0)
    {
        Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();
        if (!arguments.empty())
        {
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> tempR >> tempG >> tempB;
            mSceneMgr->setAmbientLight(Ogre::ColourValue(tempR, tempG, tempB));
            commandOutput += "\nAmbient light set to:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempR) + "    Green:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempG) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempB) + "\n";

        }
        else
        {
            Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
            commandOutput += "\nCurrent ambient light is:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.r)
                    + "    Green:  " + Ogre::StringConverter::toString(
                    (Ogre::Real) curLight.g) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.b) + "\n";
        }
    }

    // Print the help message
    else if (command.compare("help") == 0)
    {
        commandOutput += (!arguments.empty())
                ? "\nHelp for command:  " + arguments + "\n\n" + getHelpText(arguments) + "\n"
                : "\n" + ODApplication::HELP_MESSAGE + "\n";
    }

    /*
    // A utility to set the wordrap on the terminal to a specific value
    else if (command.compare("termwidth") == 0)
    {
        //NOTE: dropped in AS (this done by the console)
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> terminalWordWrap;
        }

        // Print the "tens" place line at the top
        for (int i = 0; i < terminalWordWrap / 10; ++i)
        {
            commandOutput += "         " + Ogre::StringConverter::toString(i + 1);
        }

        commandOutput += "\n";

        // Print the "ones" place
        const std::string tempString = "1234567890";
        for (int i = 0; i < terminalWordWrap - 1; ++i)
        {
            commandOutput += tempString.substr(i % 10, 1);
        }

    } */

    // A utility which adds a new section of the map given as the
    // rectangular region between two pairs of coordinates
    else if (command.compare("addtiles") == 0)
    {
        int x1, y1, x2, y2;
        tempSS.str(arguments);
        tempSS >> x1 >> y1 >> x2 >> y2;
        int xMin, yMin, xMax, yMax;
        xMin = min(x1, x2);
        xMax = max(x1, x2);
        yMin = min(y1, y2);
        yMax = max(y1, y2);

        for (int j = yMin; j < yMax; ++j)
        {
            for (int i = xMin; i < xMax; ++i)
            {
                if (gameMap->getTile(i, j) == NULL)
                {

                    char tempArray[255];
                    snprintf(tempArray, sizeof(tempArray), "Level_%3i_%3i", i,
                            j);
                    Tile *t = new Tile(i, j, Tile::dirt, 100);
                    t->name = tempArray;
                    gameMap->addTile(t);
                    t->createMesh();
                }
            }
        }

        commandOutput += "\nCreating tiles for region:\n\n\t("
                + Ogre::StringConverter::toString(xMin) + ", "
                + Ogre::StringConverter::toString(yMin) + ")\tto\t("
                + Ogre::StringConverter::toString(xMax) + ", "
                + Ogre::StringConverter::toString(yMax) + ")\n";
    }

    /*// A utility to set the camera movement speed
    else if (command.compare("movespeed") == 0)
    {
        //NOTE: converted to AS
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            CameraManager::getSingleton().setMoveSpeedAccel(2.0 * tempDouble);
            commandOutput += "\nmovespeed set to " + Ogre::StringConverter::toString(
                    tempDouble) + "\n";
        }
        else
        {
            commandOutput += "\nCurrent movespeed is "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getMoveSpeed())
                    + "\n";
        }
    } */

    // A utility to set the camera rotation speed.
    else if (command.compare("rotatespeed") == 0)
    {
        if (!arguments.empty())
        {
			Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            CameraManager::getSingleton().setRotateSpeed(Ogre::Degree(tempDouble));
            commandOutput += "\nrotatespeed set to "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    CameraManager::getSingleton().getRotateSpeed().valueDegrees()))
                    + "\n";
        }
        else
        {
            commandOutput += "\nCurrent rotatespeed is "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    CameraManager::getSingleton().getRotateSpeed().valueDegrees()))
                    + "\n";
        }
    }

    /*
    // Set max frames per second
    else if (command.compare("fps") == 0)
    {
        //NOTE: converted to AS
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            ODApplication::MAX_FRAMES_PER_SECOND = tempDouble;
            commandOutput += "\nMaximum framerate set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
        else
        {
            commandOutput += "\nCurrent maximum framerate is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
    }*/

    /*
    // Set the max number of threads the gameMap should spawn when it does the creature AI.
    else if (command.compare("aithreads") == 0)
    {
        //NOTE: converted to AS, but gameMap needs to be prepared
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            unsigned int tempInt;
            tempSS >> tempInt;
            if (tempInt >= 1)
            {
                gameMap->setMaxAIThreads(tempInt);
                commandOutput
                        += "\nMaximum number of creature AI threads set to "
                                + Ogre::StringConverter::toString(
                                        gameMap->getMaxAIThreads()) + "\n";
            }
            else
            {
                commandOutput
                        += "\nERROR: Maximum number of threads must be >= 1.\n";
            }
        }
        else
        {
            commandOutput
                    += "\nCurrent maximum number of creature AI threads is "
                            + Ogre::StringConverter::toString(gameMap->maxAIThreads)
                            + "\n";
        }
    } */

    // Set the turnsPerSecond variable to control the AI speed
    else if(command.compare("turnspersecond") == 0
            || command.compare("tps") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> ODApplication::turnsPerSecond;

            // Clear the queue of early/late time counts to reset the moving window average in the AI time display.
            gameMap->previousLeftoverTimes.clear();

            if (Socket::serverSocket != NULL)
            {
                try
                {
                    // Inform any connected clients about the change
                    ServerNotification *serverNotification =
                            new ServerNotification;
                    serverNotification->type
                            = ServerNotification::setTurnsPerSecond;
                    serverNotification->doub = ODApplication::turnsPerSecond;

                    queueServerNotification(serverNotification);
                }
                catch (bad_alloc&)
                {
                    Ogre::LogManager::getSingleton().logMessage("\n\nERROR:  bad alloc in terminal command \'turnspersecond\'\n\n", Ogre::LML_CRITICAL);
                    requestExit();
                }
            }

            commandOutput += "\nMaximum turns per second set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
        }
        else
        {
            commandOutput += "\nCurrent maximum turns per second is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
        }
    }

    // Set near clip distance
    else if (command.compare("nearclip") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            CameraManager::getSingleton().getCamera()->setNearClipDistance(tempDouble);
            commandOutput += "\nNear clip distance set to "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getNearClipDistance())
                    + "\n";
        }
        else
        {
            commandOutput += "\nCurrent near clip distance is "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getNearClipDistance())
                    + "\n";
        }
    }

    // Set far clip distance
    else if (command.compare("farclip") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempDouble;
            tempSS.str(arguments);
            tempSS >> tempDouble;
            CameraManager::getSingleton().getCamera()->setFarClipDistance(tempDouble);
            commandOutput += "\nFar clip distance set to "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getFarClipDistance()) + "\n";
        }
        else
        {
            commandOutput += "\nCurrent far clip distance is "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getFarClipDistance()) + "\n";
        }
    }

    /*
    //Set/get the mouse movement scaling (sensitivity)
    else if (command.compare("mousespeed") == 0)
    {
        //NOTE: dropped in AS
        //Doesn't do anything at the moment, after the mouse input to cegui change.
        //TODO - remove or make usable.
        commandOutput += "The command is disabled\n";
        //		if(!arguments.empty())
        //		{
        //			float speed;
        //			tempSS.str(arguments);
        //			tempSS >> speed;
        //			CEGUI::System::getSingleton().setMouseMoveScaling(speed);
        //			tempSS.str("");
        //			tempSS << "Mouse speed changed to: " << speed;
        //			commandOutput += "\n" + tempSS.str() + "\n";
        //		}
        //		else
        //		{
        //			commandOutput += "\nCurrent mouse speed is: "
        //				+ StringConverter::toString(static_cast<Real>(
        //				CEGUI::System::getSingleton().getMouseMoveScaling())) + "\n";
        //		}
    } */

    // Add a new instance of a creature to the current map.  The argument is
    // read as if it were a line in a .level file.
    else if (command.compare("addcreature") == 0)
    {
        if (!arguments.empty())
        {
            // Creature the creature and add it to the gameMap
            Creature *tempCreature = new Creature(gameMap);
            std::stringstream tempSS(arguments);
            CreatureDefinition *tempClass = gameMap->getClassDescription(
                    tempCreature->getDefinition()->getClassName());
            if (tempClass != NULL)
            {
                *tempCreature = tempClass;
                tempSS >> tempCreature;

                gameMap->addCreature(tempCreature);

                // Create the mesh and SceneNode for the new creature
                Ogre::Entity *ent = RenderManager::getSingletonPtr()->getSceneManager()->createEntity("Creature_"
                        + tempCreature->getName(), tempCreature->getDefinition()->getMeshName());
                Ogre::SceneNode *node = creatureSceneNode->createChildSceneNode(
                        tempCreature->getName() + "_node");
                //node->setPosition(tempCreature->getPosition()/BLENDER_UNITS_PER_OGRE_UNIT);
                node->setPosition(tempCreature->getPosition());
                node->setScale(tempCreature->getDefinition()->getScale());
                node->attachObject(ent);
                commandOutput += "\nCreature added successfully\n";
            }
            else
            {
                commandOutput
                        += "\nInvalid creature class name, you need to first add a class with the \'addclass\' terminal command.\n";
            }
        }
    }

    // Adds the basic information about a type of creature (mesh name, scaling, etc)
    else if (command.compare("addclass") == 0)
    {
        if (!arguments.empty())
        {
            CreatureDefinition *tempClass = new CreatureDefinition;
            tempSS.str(arguments);
            tempSS >> tempClass;

            gameMap->addClassDescription(tempClass);
        }

    }

    // Print out various lists of information, the creatures in the
    // scene, the levels available for loading, etc
    else if (command.compare("list") == 0 || command.compare("ls") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str("");

            if (arguments.compare("creatures") == 0)
            {
                tempSS << "Class:\tCreature name:\tLocation:\tColor:\tLHand:\tRHand\n\n";
                for (unsigned int i = 0; i < gameMap->numCreatures(); ++i)
                {
                    tempSS << gameMap->getCreature(i) << endl;
                }
            }

            else if (arguments.compare("classes") == 0)
            {
                tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
                for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
                {
                    CreatureDefinition *currentClassDesc =
                            gameMap->getClassDescription(i);
                    tempSS << currentClassDesc << "\n";
                }
            }

            else if (arguments.compare("players") == 0)
            {
                // There are only players if we are in a game.
                if (isInGame())
                {
                    tempSS << "Player:\tNick:\tColor:\n\n";
                    tempSS << "me\t\t" << gameMap->getLocalPlayer()->getNick() << "\t"
                            << gameMap->getLocalPlayer()->getSeat()->getColor() << "\n\n";
                    for (unsigned int i = 0; i < gameMap->numPlayers(); ++i)
                    {
                        const Player *currentPlayer = gameMap->getPlayer(i);
                        tempSS << i << "\t\t" << currentPlayer->getNick() << "\t"
                                << currentPlayer->getSeat()->getColor() << "\n";
                    }
                }
                else
                {
                    tempSS << "You must either host or join a game before you can list the players in the game.\n";
                }
            }

            else if (arguments.compare("network") == 0)
            {
                if (Socket::clientSocket != NULL)
                {
                    tempSS << "You are currently connected to a server.";
                }

                if (Socket::serverSocket != NULL)
                {
                    tempSS << "You are currently acting as a server.";
                }

                if (!isInGame())
                {
                    tempSS << "You are currently in the map editor.";
                }
            }

            else if (arguments.compare("rooms") == 0)
            {
                tempSS << "Name:\tColor:\tNum tiles:\n\n";
                for (unsigned int i = 0; i < gameMap->numRooms(); ++i)
                {
                    Room *currentRoom;
                    currentRoom = gameMap->getRoom(i);
                    tempSS << currentRoom->getName() << "\t" << currentRoom->color
                            << "\t" << currentRoom->numCoveredTiles() << "\n";
                }
            }

            else if (arguments.compare("colors") == 0 || arguments.compare(
                    "colours") == 0)
            {
                tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
                for (unsigned int i = 0; i < playerColourValues.size(); ++i)
                {
                    tempSS << "\n" << i << "\t\t" << playerColourValues[i].r
                            << "\t\t" << playerColourValues[i].g << "\t\t"
                            << playerColourValues[i].b;
                }
            }

            // Loop over level directory and display only level files
            else if (arguments.compare("levels") == 0)
            {
                std::vector<string> tempVector;
                size_t found;
                size_t found2;
                string suffix = ".level";
                string suffix2 = ".level.";
                tempVector = ResourceManager::getSingletonPtr()->
                        listAllFiles("./levels/");
                for (unsigned int j = 0; j < tempVector.size(); ++j)
                {
                    found = tempVector[j].find(suffix);
                    found2 = tempVector[j].find(suffix2);
                    if (found != string::npos && (!(found2 != string::npos)))
                    {
                        tempSS << tempVector[j] << endl;
                    }
                }
            }

            else if (arguments.compare("goals") == 0)
            {
                if (isInGame())
                {
                    // Loop over the list of unmet goals for the seat we are sitting in an print them.
                    tempSS
                            << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i < gameMap->me->getSeat()->numGoals(); ++i)
                    {
                        Goal *tempGoal = gameMap->me->getSeat()->getGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getDescription() << "\n";
                    }

                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS
                            << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i
                            < gameMap->me->getSeat()->numCompletedGoals(); ++i)
                    {
                        Goal *tempGoal = gameMap->me->getSeat()->getCompletedGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getSuccessMessage() << "\n";
                    }
                }
                else
                {
                    tempSS << "\n\nERROR: You do not have any goals to meet until you host or join a game.\n\n";
                }
            }

            else
            {
                tempSS << "ERROR:  Unrecognized list.  Type \"list\" with no arguments to see available lists.";
            }

            commandOutput += "+\n" + tempSS.str() + "\n";
        }
        else
        {
            commandOutput
                    += "lists available:\n\t\tclasses\tcreatures\tplayers\n\t\tnetwork\trooms\tcolors\n\t\tgoals\tlevels\n";
        }
    }

    /*
    // clearmap   Erase all of the tiles leaving an empty map
    else if (command.compare("newmap") == 0)
    {
        //NOTE: Converted to AS
        if (!arguments.empty())
        {
            int tempX, tempY;

            tempSS.str(arguments);
            tempSS >> tempX >> tempY;
            gameMap->createNewMap(tempX, tempY);
        }
    }*/

    /*
    // refreshmesh   Clear all the Ogre entities and redraw them so they reload their appearence.
    else if (command.compare("refreshmesh") == 0)
    {
        //NOTE: Converted to AS
        gameMap->destroyAllEntities();
        gameMap->createAllEntities();
        commandOutput += "\nRecreating all meshes.\n";
    }*/

    // Set your nickname
    else if (command.compare("nick") == 0)
    {
        if (!arguments.empty())
        {
            gameMap->me->setNick(arguments);
            commandOutput += "\nNickname set to:  ";
        }
        else
        {
            commandOutput += "\nCurrent nickname is:  ";
        }

        commandOutput += gameMap->me->getNick() + "\n";
    }

    /*
    // Set chat message variables
    else if (command.compare("maxtime") == 0)
    {
        //NOTE: Converted to AS
        if (!arguments.empty())
        {
            chatMaxTimeDisplay = atoi(arguments.c_str());
            tempSS << "Max display time for chat messages was changed to: "
                    << arguments;
        }

        else
        {
            tempSS << "Max display time for chat messages is: "
                    << chatMaxTimeDisplay;
        }

        commandOutput += "\n " + tempSS.str() + "\n";
    } */

    /*
    else if (command.compare("maxmessages") == 0)
    {
        //NOTE: converted to as
        if (!arguments.empty())
        {
            chatMaxMessages = atoi(arguments.c_str());
            tempSS << "Max chat messages to display has been set to: "
                    << arguments;
        }
        else
        {
            tempSS << "Max chat messages to display is: " << chatMaxMessages;
        }

        commandOutput += "\n" + tempSS.str() + "\n";
    } */

    // Connect to a server
    else if (command.compare("connect") == 0)
    {
        // Make sure we have set a nickname.
        if (!gameMap->me->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!isInGame())
            {
                // Make sure an IP address to connect to was provided
                if (!arguments.empty())
                {
                    Socket::clientSocket = new Socket;

                    if (!Socket::clientSocket->create())
                    {
                        Socket::clientSocket = NULL;
                        commandOutput
                                += "\nERROR:  Could not create client socket!\n";
                        goto ConnectEndLabel;
                    }

                    if (Socket::clientSocket->connect(arguments, ODApplication::PORT_NUMBER))
                    {
                        commandOutput += "\nConnection successful.\n";

                        CSPStruct *csps = new CSPStruct;
                        csps->nSocket = Socket::clientSocket;
                        csps->nFrameListener = this;

                        // Start a thread to talk to the server
                        pthread_create(&clientThread, NULL,
                                clientSocketProcessor, (void*) csps);

                        // Start the thread which will watch for local events to send to the server
                        CNPStruct *cnps = new CNPStruct;
                        cnps->nFrameListener = this;
                        pthread_create(&clientNotificationThread, NULL,
                                clientNotificationProcessor, cnps);

                        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
                        gameMap->clearMapLightIndicators();
                    }
                    else
                    {
                        Socket::clientSocket = NULL;
                        commandOutput += "\nConnection failed!\n";
                    }
                }
                else
                {
                    commandOutput
                            += "\nYou must specify the IP address of the server you want to connect to.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1\n";
                }

            }
            else
            {
                commandOutput
                        += "\nYou are already connected to a server.  You must disconnect before you can connect to a new game.\n";
            }
        }
        else
        {
            commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can join a server.\n";
        }

        ConnectEndLabel: commandOutput += "\n";

    }

    // Host a server
    else if (command.compare("host") == 0)
    {
        // Make sure we have set a nickname.
        if (!gameMap->getLocalPlayer()->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!isInGame())
            {

                if (startServer(*gameMap))
                {
                    commandOutput += "\nServer started successfully.\n";

                    // Automatically closes the terminal
                    terminalActive = false;
                }
                else
                {
                    commandOutput += "\nERROR:  Could not start server!\n";
                }

            }
            else
            {
                commandOutput
                        += "\nERROR:  You are already connected to a game or are already hosting a game!\n";
            }
        }
        else
        {
            commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can host a server.\n";
        }

    }

    // Send help command information to all players
    else if (command.compare("chathelp") == 0)
    {
        if (isInGame())
        {

            if (!arguments.empty())
            {
                // call getHelpText()
                string tempString;
                tempString = getHelpText(arguments);

                if (tempString.compare("Help for command:  \"" + arguments
                        + "\" not found.") == 0)
                {
                    tempSS << tempString << "\n";
                }
                else
                {
                    executePromptCommand("chat", "\n" + tempString);
                }
            }

            else
            {
                tempSS << "No command argument specified. See 'help' for a list of arguments.\n";
            }
        }

        else
        {
            tempSS << "Please host or connect to a game before running chathelp.\n";
        }

        commandOutput += "\n " + tempSS.str() + "\n";
    }

    // Send a chat message
    else if (command.compare("chat") == 0 || command.compare("c") == 0)
    {
        if (Socket::clientSocket != NULL)
        {
            sem_wait(&Socket::clientSocket->semaphore);
            Socket::clientSocket->send(formatCommand("chat", gameMap->me->getNick() + ":"
                    + arguments));
            sem_post(&Socket::clientSocket->semaphore);
        }
        else if (Socket::serverSocket != NULL)
        {
            // Send the chat to all the connected clients
            for (unsigned int i = 0; i < clientSockets.size(); ++i)
            {
                sem_wait(&clientSockets[i]->semaphore);
                clientSockets[i]->send(formatCommand("chat", gameMap->me->getNick()
                        + ":" + arguments));
                sem_post(&clientSockets[i]->semaphore);
            }

            // Display the chat message in our own message queue
            chatMessages.push_back(new ChatMessage(gameMap->me->getNick(), arguments,
                    time(NULL), time(NULL)));
        }
        else
        {
            commandOutput
                    += "\nYou must be either connected to a server, or hosting a server to use chat.\n";
        }
    }

    // Start the visual debugging indicators for a given creature
    else if (command.compare("visdebug") == 0)
    {
        if (Socket::serverSocket != NULL)
        {
            if (arguments.length() > 0)
            {
                // Activate visual debugging
                Creature *tempCreature = gameMap->getCreature(arguments);
                if (tempCreature != NULL)
                {
                    if (!tempCreature->getHasVisualDebuggingEntities())
                    {
                        tempCreature->createVisualDebugEntities();
                        commandOutput
                                += "\nVisual debugging entities created for creature:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        tempCreature->destroyVisualDebugEntities();
                        commandOutput
                                += "\nVisual debugging entities destroyed for creature:  "
                                        + arguments + "\n";
                    }
                }
                else
                {
                    commandOutput
                            += "\nCould not create visual debugging entities for creature:  "
                                    + arguments + "\n";
                }
            }
            else
            {
                commandOutput
                        += "\nERROR:  You must supply a valid creature name to create debug entities for.\n";
            }
        }
        else
        {
            commandOutput
                    += "\nERROR:  Visual debugging only works when you are hosting a game.\n";
        }
    }

    else if (command.compare("addcolor") == 0)
    {
        if (!arguments.empty())
        {
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> tempR >> tempG >> tempB;
            playerColourValues.push_back(Ogre::ColourValue(tempR, tempG, tempB));
            tempSS.str("");
            tempSS << "Color number " << playerColourValues.size() << " added.";
            commandOutput += "\n" + tempSS.str() + "\n";
        }
        else
        {
            commandOutput
                    += "\nERROR:  You need to specify and RGB triplet with values in (0.0, 1.0)\n";
        }
    }

    else if (command.compare("setcolor") == 0)
    {
        if (!arguments.empty())
        {
            unsigned int index;
            Ogre::Real tempR, tempG, tempB;
            tempSS.str(arguments);
            tempSS >> index >> tempR >> tempG >> tempB;
            if (index < playerColourValues.size())
            {
                playerColourValues[index] = Ogre::ColourValue(tempR, tempG, tempB);
                tempSS.str("");
                tempSS << "Color number " << index << " changed to " << tempR
                        << "\t" << tempG << "\t" << tempB;
                commandOutput += "an" + tempSS.str() + "\n";
            }

        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify a color index between 0 and "
                    << playerColourValues.size()
                    << " and an RGB triplet with values in (0.0, 1.0)";
            commandOutput += "\n" + tempSS.str() + "\n";
        }
    }

    //FIXME:  This function is not yet implemented.
    else if (command.compare("disconnect") == 0)
    {
        commandOutput += (Socket::serverSocket != NULL)
            ? "\nStopping server.\n"
            : (Socket::clientSocket != NULL)
                ? "\nDisconnecting from server.\n"
                : "\nYou are not connected to a server and you are not hosting a server.";
    }

    // Load the next level.
    else if (command.compare("next") == 0)
    {
        if (gameMap->seatIsAWinner(gameMap->me->getSeat()))
        {
            gameMap->loadNextLevel = true;
            commandOutput += (string) "\nLoading level levels/"
                    + gameMap->nextLevel + ".level\n";
        }
        else
        {
            commandOutput += "\nYou have not completed this level yet.\n";
        }
    }

    else
    {
        //try AngelScript interpreter
        return false;
        //commandOutput
        //        += "\nCommand not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.\n";
    }

    return true;
}

/*! \brief A helper function to return a help text string for a given termianl command.
 *
 */
string ODFrameListener::getHelpText(std::string arg)
{
    std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

    if (arg.compare("save") == 0)
    {
        return "Save the current level to a file.  The file name is given as an argument to the save command, if no file name is given the last file loaded is overwritten by the save command.\n\nExample:\n"
                + prompt
                + "save Test\n\nThe above command will save the level to levels/Test.level.  The Test level is loaded automatically when OpenDungeons starts.";
    }

    else if (arg.compare("load") == 0)
    {
        return "Load a level from a file.  The new level replaces the current level.  The levels are stored in the levels directory and have a .level extension on the end.  Both the directory and the .level extension are automatically applied for you, if no file name is given the last file loaded is reloaded.\n\nExample:\n"
                + prompt
                + "load Level1\n\nThe above command will load the file Level1.level from the levels directory.";
    }

    else if (arg.compare("addclass") == 0)
    {
        return "Add a new class decription to the current map.  Because it is common to load many creatures of the same type creatures are given a class which stores their common information such as the mesh to load, scaling, etc.  Addclass defines a new class of creature, allowing creatures of this class to be loaded in the future.  The argument to addclass is interpreted in the same was as a class description line in the .level file format.\n\nExample:\n"
                + prompt
                + "addclass Skeleton Skeleton.mesh 0.01 0.01 0.01\n\nThe above command defines the class \"Skeleton\" which uses the mesh file \"Skeleton.mesh\" and has a scale factor of 0.01 in the X, Y, and Z dimensions.";
    }

    else if (arg.compare("addcreature") == 0)
    {
        return "Add a new creature to the current map.  The creature class to be used must be loaded first, either from the loaded map file or by using the addclass command.  Once a class has been declared a creature can be loaded using that class.  The argument to the addcreature command is interpreted in the same way as a creature line in a .level file.\n\nExample:\n"
                + prompt
                + "addcreature Skeleton Bob 10 15 0\n\nThe above command adds a creature of class \"Skeleton\" whose name is \"Bob\" at location X=10, y=15, and Z=0.  The new creature's name must be unique to the creatures in that level.  Alternatively the name can be se to \"autoname\" to have OpenDungeons assign a unique name.";
    }

    else if (arg.compare("quit") == 0)
    {
        return "Exits OpenDungeons";
    }

    else if (arg.compare("termwidth") == 0)
    {
        return "The termwidth program sets the maximum number of characters that can be displayed on the terminal without word wrapping taking place.  When run with no arguments, termwidth displays a ruler across the top of you terminal indicating the terminal's current width.  When run with an argument, termwidth sets the terminal width to a new value specified in the argument.\n\nExample:\n"
                + prompt
                + "termwidth 80\n\nThe above command sets the terminal width to 80.";
    }

    else if (arg.compare("addtiles") == 0)
    {
        return "The addtiles command adds a rectangular region of tiles to the map.  The tiles are initialized to a fullness of 100 and have their type set to dirt.  The region to be added is given as two pairs of X-Y coordinates.\n\nExample:\n"
                + prompt
                + "addtiles -10 -5 34 20\n\nThe above command adds the tiles in the given region to the map.  Tiles which overlap already existing tiles will be ignored.";
    }

    else if (arg.compare("newmap") == 0)
    {
        return "Replaces the existing map with a new rectangular map.  The X and Y dimensions of the new map are given as arguments to the newmap command.\n\nExample:\n"
                + prompt
                + "newmap 10 20\n\nThe above command creates a new map 10 tiles by 20 tiles.  The new map will be filled with dirt tiles with a fullness of 100.";
    }

    else if (arg.compare("refreshmesh") == 0)
    {
        return "Clears every mesh in the entire game (creatures, tiles, etc) and then reloads them so they have the new look in the material files, etc.";
    }

    else if (arg.compare("movespeed") == 0)
    {
        return "The movespeed command sets how fast the camera moves at.  When run with no argument movespeed simply prints out the current camera move speed.  With an argument movespeed sets the camera move speed.\n\nExample:\n"
                + prompt
                + "movespeed 3.7\n\nThe above command sets the camera move speed to 3.7.";
    }

    else if (arg.compare("rotatespeed") == 0)
    {
        return "The rotatespeed command sets how fast the camera rotates.  When run with no argument rotatespeed simply prints out the current camera rotation speed.  With an argument rotatespeed sets the camera rotation speed.\n\nExample:\n"
                + prompt
                + "rotatespeed 35\n\nThe above command sets the camera rotation speed to 35.";
    }

    else if (arg.compare("ambientlight") == 0)
    {
        return "The ambientlight command sets the minumum light that every object in the scene is illuminated with.  It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n"
                + prompt
                + "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.";
    }

    else if (arg.compare("host") == 0)
    {
        std::stringstream s;
        s << ODApplication::PORT_NUMBER;
        return "Starts a server thread running on this machine.  This utility takes a port number as an argument.  The port number is the port to listen on for a connection.  The default (if no argument is given) is to use" + s.str() + "for the port number.";
    }

    else if (arg.compare("connnect") == 0)
    {
        return "Connect establishes a connection with a server.  It takes as its argument an IP address specified in dotted decimal notation (such as 192.168.1.100), and starts a client thread which monitors the connection for events.";
    }

    else if (arg.compare("chat") == 0 || arg.compare("c") == 0)
    {
        return "Chat (or \"c\" for short) is a utility to send messages to other players participating in the same game.  The argument to chat is broadcast to all members of the game, along with the nick of the person who sent the chat message.  When a chat message is recieved it is added to the chat buffer along with a timestamp indicating when it was recieved.\n\nThe chat buffer displays the last n chat messages recieved.  The number of displayed messages can be set with the \"chathist\" command.  Displayed chat messages will also be removed from the chat buffer after they age beyond a certain point.";
    }

    else if (arg.compare("list") == 0 || arg.compare("ls") == 0)
    {
        return "List (or \"ls\" for short is a utility which lists various types of information about the current game.  Running list without an argument will produce a list of the lists available.  Running list with an argument displays the contents of that list.\n\nExample:\n"
                + prompt
                + "list creatures\n\nThe above command will produce a list of all the creatures currently in the game.";
    }

    else if (arg.compare("visdebug") == 0)
    {
        return "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n"
                + prompt
                + "visdebug skeletor\n\nThe above command wil turn on visual debugging for the creature named \'skeletor\'.  The same command will turn it back off again.";
    }

    else if (arg.compare("turnspersecond") == 0 || arg.compare("tps") == 0)
    {
        return "turnspersecond (or \"tps\" for short is a utility which displays or sets the speed at which the game is running.\n\nExample:\n"
                + prompt
                + "tps 5\n\nThe above command will set the current game speed to 5 turns per second.";
    }

    else if (arg.compare("mousespeed") == 0)
    {
        return "Mousespeed sets the mouse movement speed scaling factor. It takes a decimal number as argument, which the mouse movement will get multiplied by.";
    }

    else if (arg.compare("framespersecond") == 0 || arg.compare("fps") == 0)
    {
        return "framespersecond (or \"fps\" for short is a utility which displays or sets the maximum framerate at which the rendering will attempt to update the screen.\n\nExample:\n"
                + prompt
                + "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.";
    }

    else if (arg.compare("keys") == 0)
    {
        return "|| Action           || Keyboard 1       || Keyboard 2       ||\n\
                ==============================================================\n\
                || Zoom In          || Page Up          || e                ||\n\
                || Zoom Out         || Insert           || q                ||\n\
                || Pan Left         || Left             || a                ||\n\
                || Pan Right        || Right            || d                ||\n\
                || Pan Forward      || Up               || w                ||\n\
                || Pan Backward     || Down             || s                ||\n\
                || Pan Backward     || Down             || s                ||\n\
                || Tilt Up          || Home             || N/A              ||\n\
                || Tilt Down        || End              || N/A              ||\n\
                || Rotate Left      || Delete           || N/A              ||\n\
                || Rotate right     || Page Down        || N/A              ||\n\
                || Toggle Console   || `                || F12              ||\n\
                || Quit Game        || ESC              || N/A              ||\n\
                || Take screenshot  || Printscreen      || N/A              ||\n\
                || Toggle Framerate || f                || N/A              ||";
    }
    else if (arg.compare("aithreads") == 0)
    {
        return "Sets the maximum number of threads the gameMap will attempt to spawn during the f() method.  The set value must be greater than or equal to 1.";
    }
    else
    {
        return "Help for command:  \"" + arguments + "\" not found.";
    }
}

/*! \brief Check if we are in editor mode
 *
 */
bool ODFrameListener::isInGame()
{
    //TODO: this exact function is also in InputManager, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}
