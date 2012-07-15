/*!
 * \file   Console.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 */

/* TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */
#include "Console.h"
#include "Functions.h"
#include "Socket.h"
#include "ASWrapper.h"
#include "Network.h"
#include "GameMode.h"
#include "LogManager.h"
#include "ODApplication.h"
#include "ODFrameListener.h"
#include "RenderManager.h"
#include "MapLoader.h"
#include "CameraManager.h"
#include "ServerNotification.h"
#include "Player.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "AllGoals.h"
#include "ModeManager.h"

template<> Console* Ogre::Singleton<Console>::ms_Singleton = 0;
using std::min;
using std::max;


Console::Console() :
        //these two define how much text goes into the console
        odf(ODFrameListener::getSingletonPtr()),
        consoleLineLength   (100),
        consoleLineCount    (14),
        blinkSpeed          (0.5),
        timeSinceLastBlink  (0.0),
        visible             (false),
        updateOverlay       (true),
        allowTrivial        (false),
        allowNormal         (false),
        allowCritical       (true),
        chatMode            (false),
        cursorVisible       (true),
        startLine           (0),
        cursorChar          ("_"),
        curHistPos          (0)
{
    LogManager::getSingleton().logMessage("*** Initiliasing Console ***");
    ODApplication::getSingleton().getRoot()->addFrameListener(this);
    Ogre::OverlayManager& olMgr = Ogre::OverlayManager::getSingleton();

    // Create a panel
    panel = static_cast<Ogre::OverlayContainer*>(
            olMgr.createOverlayElement("Panel", "ConsolePanel"));
    panel->setPosition(0, 0.7);
    panel->setDimensions(1, 0.3);
    panel->setMaterialName("console/background");

    // Create a text area
    textbox = olMgr.createOverlayElement("TextArea", "ConsoleText");
    textbox->setPosition(0, 0);
    textbox->setParameter("font_name", "FreeMono");
    textbox->setParameter("char_height", "0.02");

    // Create an overlay, and add the panel
    overlay = olMgr.create("Console");
    overlay->add2D(panel);

    // Add the text area to the panel
    panel->addChild(textbox);

    LogManager::getSingleton().getLog().addListener(this);
}

Console::~Console()
{
    delete panel;
    delete textbox;
    delete overlay;
}

/*! \brief Handles the mouse movement on the Console
 *
 */
void Console::onMouseMoved(const OIS::MouseEvent& arg, const bool isCtrlDown)
{
    if(arg.state.Z.rel == 0 || !visible)
    {
        return;
    }

    if(isCtrlDown)
    {
        scrollHistory(arg.state.Z.rel > 0);
    }
    else
    {
        scrollText(arg.state.Z.rel > 0);
    }

    updateOverlay = true;
}

/*! \brief Handles the key input on the Console
 *
 */
void Console::onKeyPressed(const OIS::KeyEvent& arg)
{
    if (!visible)
    {
        return;
    }

    switch(arg.key)
    {
        case OIS::KC_GRAVE:
        case OIS::KC_ESCAPE:
        case OIS::KC_F12:
            Console::getSingleton().setVisible(false);
            ODFrameListener::getSingleton().setTerminalActive(false);
            modeManager->getCurrentMode()->getKeyboard()->setTextTranslation(OIS::Keyboard::Off);
            break;

        case OIS::KC_RETURN:
        {
            //only do this for non-empty input
            if(!prompt.empty())
            {
                //print our input and push it to the history
                print(prompt);
                history.push_back(prompt);
                ++curHistPos;

                //split the input into it's space-separated "words"
                std::vector<Ogre::String> params = split(prompt, ' ');

                //TODO: remove this until AS console handler is ready
                Ogre::String command = params[0];
                Ogre::String arguments = "";
                for(size_t i = 1; i< params.size(); ++i)
                {
                    arguments += params[i];
                    if(i < params.size() - 1)
                    {
                      arguments += ' ';
                    }
                }
                //remove until this point

                // Force command to lower case
                //TODO: later do this only for params[0]
                std::transform(command.begin(), command.end(), command.begin(), ::tolower);
                std::transform(params[0].begin(), params[0].end(), params[0].begin(), ::tolower);

                
                
                //TODO: remove executePromptCommand after it is fully converted
                //for now try hardcoded commands, and if none is found try AS
                if(!executePromptCommand(command, arguments))
                {
                    LogManager::getSingleton().logMessage("Console command: " + command + " - arguments: " + arguments + " - actionscript");
                    ASWrapper::getSingleton().executeConsoleCommand(params);
                }

                prompt = "";
            }
            else
            {
                //set history position back to last entry
                curHistPos = history.size();
            }
            break;
        }

        case OIS::KC_BACK:
            prompt = prompt.substr(0, prompt.length() - 1);
            break;

        case OIS::KC_PGUP:
            scrollText(true);
            break;

        case OIS::KC_PGDOWN:
            scrollText(false);
            break;

        case OIS::KC_UP:
            scrollHistory(true);
            break;

        case OIS::KC_DOWN:
            scrollHistory(false);
            break;

        case OIS::KC_F10:
        {
            LogManager::getSingleton().logMessage("RTSS test----------");
            RenderManager::getSingleton().rtssTest();
            break;
        }

        default:
            if (std::string("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?1234567890-=\\!@#$%^&*()_+|;\':\"[]{}").find(
                    arg.text) != std::string::npos)
            {
                prompt += arg.text;
            }
            break;
    }

    updateOverlay = true;
}


/*! \brief Check if we are in editor mode
 *
 */



/*! \brief Defines the action on starting the current frame
 *
 *  The Console listener checks if it needs updating and if it does it will
 *  redraw itself with the new text
 */
bool Console::frameStarted(const Ogre::FrameEvent& evt)
{
    if(visible)
    {
        timeSinceLastBlink += evt.timeSinceLastFrame;

        if(timeSinceLastBlink >= blinkSpeed)
        {
            timeSinceLastBlink -= blinkSpeed;
            cursorVisible = !cursorVisible;
            updateOverlay = true;
        }
    }

    if(updateOverlay)
    {
        Ogre::String text;
        std::list<Ogre::String>::iterator i, start, end;

        //make sure is in range
        if(startLine > lines.size())
        {
            startLine = lines.size();
        }

        start = lines.begin();
        for (unsigned int c = 0; c < startLine; ++c)
        {
            ++start;
        }

        end = start;
        for (unsigned int c = 0; c < consoleLineCount; ++c)
        {
            if (end == lines.end())
            {
                break;
            }
            ++end;
        }

        unsigned int counter = 0;
        for (i = start; i != end; ++i)
        {
            text += (*i) + "\n";
            ++counter;
        }

        for(; counter < consoleLineCount; ++counter)
        {
            text += "\n";
        }
        //add the prompt
        text += ">>> " + prompt + (cursorVisible ? cursorChar : "");

        textbox->setCaption(text);
        updateOverlay = false;
    }

    return true;
}

/*! \brief what happens after frame
 *
 */
bool Console::frameEnded(const Ogre::FrameEvent& evt)
{
    return true;
}

/*! \brief print text to the console
 *
 * This function automatically checks if there are linebreaks in the text
 * and separates the text into separate strings
 *
 * \param text The text to be added to the console
 */
void Console::print(const Ogre::String& text)
{
    std::vector<Ogre::String> newLines = split(text, '\n');
    lines.insert(lines.end(), newLines.begin(), newLines.end());

    startLine = (lines.size() > consoleLineCount)
                            ? lines.size() - consoleLineCount
                            : 0;

    updateOverlay = true;
}

/*! \brief show or hide the console manually
 *
 */
void Console::setVisible(const bool newState)
{
    visible = newState;
    Gui::getSingleton().setVisible(!visible);
    checkVisibility();
}

/*! \brief enables or disables the console, depending on what state it has
 *
 */
void Console::toggleVisibility()
{
    visible = !visible;
    Gui::getSingleton().setVisible(!visible);
    checkVisibility();
}

/*! \brief Does the actual showing/hiding depending on bool visible
 *
 */
void Console::checkVisibility()
{
    if(visible)
    {
        overlay->show();
    }
    else
    {
        overlay->hide();
    }
}

/*! \brief Splits a string on every occurance of splitChar
 *
 *  \return A vector of all splitted sub strings
 *
 *  \param str The string to be splitted
 *  \param splitChar The character that defines the split positions
 */
std::vector<Ogre::String> Console::split(const Ogre::String& str, const char splitChar)
{
    std::vector<Ogre::String> splittedStrings;
    size_t lastPos = 0, pos = 0;
    do
    {
        pos = str.find(splitChar, lastPos);
        splittedStrings.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = pos + 1; //next time start AFTER the last space
    }
    while(pos != std::string::npos);

    return splittedStrings;
}

/*! \brief Send logged messages also to the Console
 *
 * We only allow critical messages to the console. Non-critical messages would
 * pollute the console window and make it hardly readable.
 */
void Console::messageLogged(const Ogre::String & message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String & logName)
{
    //test if the logLevel is allowed, if not then return
    switch(lml)
    {
        case Ogre::LML_CRITICAL:
            if(!allowCritical){return;}
            break;

        case Ogre::LML_TRIVIAL:
            if(!allowTrivial){return;}
            break;

        case Ogre::LML_NORMAL:
            if(!allowNormal){return;}
            break;

        default:
            return;
    }

    //if it was allowed then print the message
    print(logName + ": " + message);
}

/*! \brief Scrolls through the history of user entered commands
 *
 *  \param direction true means going up (old), false means going down (new)
 */
void Console::scrollHistory(const bool direction)
{
    if(direction)
    {
        //don't go unter 0, it's an unsigned int and the minimum index!
        if(curHistPos == 0)
        {
            return;
        }
        else
        {
            --curHistPos;
        }
    }
    else
    {
        //don't go over maximum index and clear the prompt when trying.
        if(++curHistPos >= history.size())
        {
            curHistPos = history.size();
            prompt = "";
            return;
        }

    }

    prompt = history[curHistPos];
}

/*! \brief Scrolls through the text output in the console
 *
 *  \param direction true means going up (old), false means going down (new)
 */
void Console::scrollText(const bool direction)
{
    if(direction)
    {
        if(startLine > 0)
        {
            --startLine;
        }
    }
    else
    {
        if(startLine < lines.size() && lines.size() - startLine > consoleLineCount)
        {
            ++startLine;
        }
    }
}

bool Console::executePromptCommand(const std::string& command, std::string arguments)
{
    std::stringstream tempSS;

    /*
    // Exit the program
    if (command.compare("quit") == 0 || command.compare("exit") == 0)
    {
        //NOTE: converted to AS
        odf->requestExit();
    }

    // Repeat the arguments of the command back to you
    else if (command.compare("echo") == 0)
    {
        //NOTE: dropped in AS (was this any useful?)
        odf->commandOutput += "\n" + arguments + "\n";
    } */

    /*
    // Write the current level out to file specified as an argument
    if (command.compare("save") == 0)
    {
        //NOTE: convetred to AS
        if (arguments.empty())
        {
            odf->commandOutput
                    += "No level name given: saving over the last loaded level: "
                            + odf->gameMap->getLevelFileName() + "\n\n";
            arguments = odf->gameMap->getLevelFileName();
        }

        string tempFileName = "levels/" + arguments + ".level";
        MapLoader::writeGameMapToFile(tempFileName, *odf->gameMap);
        odf->commandOutput += "\nFile saved to   " + tempFileName + "\n";

        odf->gameMap->setLevelFileName(arguments);
    }*/

    // Clear the current level and load a new one from a file
    if (command.compare("load") == 0)
    {
        if (arguments.empty())
        {
            odf->commandOutput
                    += "No level name given: loading the last loaded level: "
                            + odf->gameMap->getLevelFileName() + "\n\n";
            arguments = odf->gameMap->getLevelFileName();
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
                odf->gameMap->nextLevel = tempString;
                odf->gameMap->loadNextLevel = true;
            }
            else
            {
                if (MapLoader::readGameMapFromFile(tempString, *odf->gameMap))
                {
                    tempSS << "Successfully loaded file:  " << tempString
                            << "\nNum tiles:  " << odf->gameMap->numTiles()
                            << "\nNum classes:  "
                            << odf->gameMap->numClassDescriptions()
                            << "\nNum creatures:  " << odf->gameMap->numCreatures();
                    odf->commandOutput += tempSS.str();

                    odf->gameMap->createAllEntities();
                }
                else
                {
                    tempSS << "ERROR: Could not load game map \'" << tempString
                            << "\'.";
                    odf->commandOutput += tempSS.str();
                }
            }

            odf->gameMap->setLevelFileName(arguments);
        }
        else
        {
            odf->commandOutput
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
            odf->commandOutput += "\nAmbient light set to:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempR) + "    Green:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempG) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) tempB) + "\n";

        }
        else
        {
            Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
            odf->commandOutput += "\nCurrent ambient light is:\nRed:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.r)
                    + "    Green:  " + Ogre::StringConverter::toString(
                    (Ogre::Real) curLight.g) + "    Blue:  "
                    + Ogre::StringConverter::toString((Ogre::Real) curLight.b) + "\n";
        }
    }

    // Print the help message
    else if (command.compare("help") == 0)
    {
        odf->commandOutput += (!arguments.empty())
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
            odf->commandOutput += "         " + Ogre::StringConverter::toString(i + 1);
        }

        odf->commandOutput += "\n";

        // Print the "ones" place
        const std::string tempString = "1234567890";
        for (int i = 0; i < terminalWordWrap - 1; ++i)
        {
            odf->commandOutput += tempString.substr(i % 10, 1);
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
                if (odf->gameMap->getTile(i, j) == NULL)
                {

		stringstream ss;

		ss.str(std::string());
		ss << "Level";
		ss << "_";
		ss << i;
		ss << "_";
		ss << j;



		
		Tile *t = new Tile(i, j, Tile::dirt, 100);



		t->setName(ss.str());
		odf->gameMap->addTile(t);
		t->createMesh();
                }
            }
        }

        odf->commandOutput += "\nCreating tiles for region:\n\n\t("
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
            odf->commandOutput += "\nmovespeed set to " + Ogre::StringConverter::toString(
                    tempDouble) + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent movespeed is "
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
            odf->commandOutput += "\nrotatespeed set to "
                    + Ogre::StringConverter::toString(
                            static_cast<Ogre::Real>(
                                    CameraManager::getSingleton().getRotateSpeed().valueDegrees()))
                    + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent rotatespeed is "
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
            odf->commandOutput += "\nMaximum framerate set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent maximum framerate is "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::MAX_FRAMES_PER_SECOND))
                    + "\n";
        }
    }*/

    
    // Set the max number of threads the odf->gameMap should spawn when it does the creature AI.
    else if (command.compare("aithreads") == 0)
    {
        //NOTE: converted to AS, but odf->gameMap needs to be prepared
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            unsigned int tempInt;
            tempSS >> tempInt;
            if (tempInt >= 1)
            {
                odf->gameMap->setMaxAIThreads(tempInt);
                odf->commandOutput
                        += "\nMaximum number of creature AI threads set to "
                                + Ogre::StringConverter::toString(
                                        odf->gameMap->getMaxAIThreads()) + "\n";
            }
            else
            {
                odf->commandOutput
                        += "\nERROR: Maximum number of threads must be >= 1.\n";
            }
        }
        else
        {
            odf->commandOutput
                    += "\nCurrent maximum number of creature AI threads is "
		+ Ogre::StringConverter::toString(odf->gameMap->getMaxAIThreads())
                            + "\n";
        }
    } 

    // Set the turnsPerSecond variable to control the AI speed
    else if(command.compare("turnspersecond") == 0
            || command.compare("tps") == 0)
    {
        if (!arguments.empty())
        {
            tempSS.str(arguments);
            tempSS >> ODApplication::turnsPerSecond;

            // Clear the queue of early/late time counts to reset the moving window average in the AI time display.
            odf->gameMap->previousLeftoverTimes.clear();

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
                    odf->requestExit();
                }
            }

            odf->commandOutput += "\nMaximum turns per second set to "
                    + Ogre::StringConverter::toString(static_cast<Ogre::Real>(ODApplication::turnsPerSecond)) + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent maximum turns per second is "
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
            odf->commandOutput += "\nNear clip distance set to "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getNearClipDistance())
                    + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent near clip distance is "
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
            odf->commandOutput += "\nFar clip distance set to "
                    + Ogre::StringConverter::toString(
                            CameraManager::getSingleton().getCamera()->getFarClipDistance()) + "\n";
        }
        else
        {
            odf->commandOutput += "\nCurrent far clip distance is "
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
        odf->commandOutput += "The command is disabled\n";
        //		if(!arguments.empty())
        //		{
        //			float speed;
        //			tempSS.str(arguments);
        //			tempSS >> speed;
        //			CEGUI::System::getSingleton().setMouseMoveScaling(speed);
        //			tempSS.str("");
        //			tempSS << "Mouse speed changed to: " << speed;
        //			odf->commandOutput += "\n" + tempSS.str() + "\n";
        //		}
        //		else
        //		{
        //			odf->commandOutput += "\nCurrent mouse speed is: "
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
            // Creature the creature and add it to the odf->gameMap
            Creature *tempCreature = new Creature(odf->gameMap);
            std::stringstream tempSS(arguments);
            CreatureDefinition *tempClass = odf->gameMap->getClassDescription(
                    tempCreature->getDefinition()->getClassName());
            if (tempClass != NULL)
            {
                *tempCreature = tempClass;
                tempSS >> tempCreature;

                odf->gameMap->addCreature(tempCreature);

                // Create the mesh and SceneNode for the new creature
                Ogre::Entity *ent = RenderManager::getSingletonPtr()->getSceneManager()->createEntity("Creature_"
                        + tempCreature->getName(), tempCreature->getDefinition()->getMeshName());
                Ogre::SceneNode *node = odf->creatureSceneNode->createChildSceneNode(
                        tempCreature->getName() + "_node");
                //node->setPosition(tempCreature->getPosition()/BLENDER_UNITS_PER_OGRE_UNIT);
                node->setPosition(tempCreature->getPosition());
                node->setScale(tempCreature->getDefinition()->getScale());
                node->attachObject(ent);
                odf->commandOutput += "\nCreature added successfully\n";
            }
            else
            {
                odf->commandOutput
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

            odf->gameMap->addClassDescription(tempClass);
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
                for (unsigned int i = 0; i < odf->gameMap->numCreatures(); ++i)
                {
                    tempSS << odf->gameMap->getCreature(i) << endl;
                }
            }

            else if (arguments.compare("classes") == 0)
            {
                tempSS << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
                for (unsigned int i = 0; i < odf->gameMap->numClassDescriptions(); ++i)
                {
                    CreatureDefinition *currentClassDesc =
                            odf->gameMap->getClassDescription(i);
                    tempSS << currentClassDesc << "\n";
                }
            }

            else if (arguments.compare("players") == 0)
            {
                // There are only players if we are in a game.
                if (odf->isInGame())
                {
                    tempSS << "Player:\tNick:\tColor:\n\n";
                    tempSS << "me\t\t" << odf->gameMap->getLocalPlayer()->getNick() << "\t"
                            << odf->gameMap->getLocalPlayer()->getSeat()->getColor() << "\n\n";
                    for (unsigned int i = 0; i < odf->gameMap->numPlayers(); ++i)
                    {
                        const Player *currentPlayer = odf->gameMap->getPlayer(i);
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

                if (!odf->isInGame())
                {
                    tempSS << "You are currently in the map editor.";
                }
            }

            else if (arguments.compare("rooms") == 0)
            {
                tempSS << "Name:\tColor:\tNum tiles:\n\n";
                for (unsigned int i = 0; i < odf->gameMap->numRooms(); ++i)
                {
                    Room *currentRoom;
                    currentRoom = odf->gameMap->getRoom(i);
                    tempSS << currentRoom->getName() << "\t" << currentRoom->getColor()
                            << "\t" << currentRoom->numCoveredTiles() << "\n";
                }
            }

            else if (arguments.compare("colors") == 0 || arguments.compare(
                    "colours") == 0)
            {
                tempSS << "Number:\tRed:\tGreen:\tBlue:\n";
                for (unsigned int i = 0; i < odf->playerColourValues.size(); ++i)
                {
                    tempSS << "\n" << i << "\t\t" << odf->playerColourValues[i].r
                            << "\t\t" << odf->playerColourValues[i].g << "\t\t"
                            << odf->playerColourValues[i].b;
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
                if (odf->isInGame())
                {
                    // Loop over the list of unmet goals for the seat we are sitting in an print them.
                    tempSS
                            << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i < odf->gameMap->me->getSeat()->numGoals(); ++i)
                    {
                        Goal *tempGoal = odf->gameMap->me->getSeat()->getGoal(i);
                        tempSS << tempGoal->getName() << ":\t"
                                << tempGoal->getDescription() << "\n";
                    }

                    // Loop over the list of completed goals for the seat we are sitting in an print them.
                    tempSS
                            << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
                    for (unsigned int i = 0; i
                            < odf->gameMap->me->getSeat()->numCompletedGoals(); ++i)
                    {
                        Goal *tempGoal = odf->gameMap->me->getSeat()->getCompletedGoal(i);
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

            odf->commandOutput += "+\n" + tempSS.str() + "\n";
        }
        else
        {
            odf->commandOutput
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
            odf->gameMap->createNewMap(tempX, tempY);
        }
    }*/

    /*
    // refreshmesh   Clear all the Ogre entities and redraw them so they reload their appearence.
    else if (command.compare("refreshmesh") == 0)
    {
        //NOTE: Converted to AS
        odf->gameMap->destroyAllEntities();
        odf->gameMap->createAllEntities();
        odf->commandOutput += "\nRecreating all meshes.\n";
    }*/

    // Set your nickname
    else if (command.compare("nick") == 0)
    {
        if (!arguments.empty())
        {
            odf->gameMap->me->setNick(arguments);
            odf->commandOutput += "\nNickname set to:  ";
        }
        else
        {
            odf->commandOutput += "\nCurrent nickname is:  ";
        }

        odf->commandOutput += odf->gameMap->me->getNick() + "\n";
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

        odf->commandOutput += "\n " + tempSS.str() + "\n";
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

        odf->commandOutput += "\n" + tempSS.str() + "\n";
    } */

    // Connect to a server
    else if (command.compare("connect") == 0)
    {
        // Make sure we have set a nickname.
        if (!odf->gameMap->me->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!odf->isInGame())
            {
                // Make sure an IP address to connect to was provided
                if (!arguments.empty())
                {
                    Socket::clientSocket = new Socket;

                    if (!Socket::clientSocket->create())
                    {
                        Socket::clientSocket = NULL;
                        odf->commandOutput
                                += "\nERROR:  Could not create client socket!\n";
                        goto ConnectEndLabel;
                    }

                    if (Socket::clientSocket->connect(arguments, ODApplication::PORT_NUMBER))
                    {
                        odf->commandOutput += "\nConnection successful.\n";

                        CSPStruct *csps = new CSPStruct;
                        csps->nSocket = Socket::clientSocket;
                        csps->nFrameListener = odf;

                        // Start a thread to talk to the server
                        pthread_create(&(odf->clientThread), NULL,
                                clientSocketProcessor, (void*) csps);

                        // Start the thread which will watch for local events to send to the server
                        CNPStruct *cnps = new CNPStruct;
                        cnps->nFrameListener = odf;
                        pthread_create(&(odf->clientNotificationThread), NULL,
                                clientNotificationProcessor, cnps);

                        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
                        odf->gameMap->clearMapLightIndicators();
                    }
                    else
                    {
                        Socket::clientSocket = NULL;
                        odf->commandOutput += "\nConnection failed!\n";
                    }
                }
                else
                {
                    odf->commandOutput
                            += "\nYou must specify the IP address of the server you want to connect to.  Any IP address which is not a properly formed IP address will resolve to 127.0.0.1\n";
                }

            }
            else
            {
                odf->commandOutput
                        += "\nYou are already connected to a server.  You must disconnect before you can connect to a new game.\n";
            }
        }
        else
        {
            odf->commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can join a server.\n";
        }

        ConnectEndLabel: odf->commandOutput += "\n";

    }

    // Host a server
    else if (command.compare("host") == 0)
    {
        // Make sure we have set a nickname.
        if (!odf->gameMap->getLocalPlayer()->getNick().empty())
        {
            // Make sure we are not already connected to a server or hosting a game.
            if (!odf->isInGame())
            {

                if (startServer(*odf->gameMap))
                {
                    odf->commandOutput += "\nServer started successfully.\n";

                    // Automatically closes the terminal
                    odf->terminalActive = false;
                }
                else
                {
                    odf->commandOutput += "\nERROR:  Could not start server!\n";
                }

            }
            else
            {
                odf->commandOutput
                        += "\nERROR:  You are already connected to a game or are already hosting a game!\n";
            }
        }
        else
        {
            odf->commandOutput
                    += "\nYou must set a nick with the \"nick\" command before you can host a server.\n";
        }

    }

    // Send help command information to all players
    else if (command.compare("chathelp") == 0)
    {
        if (odf->isInGame())
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

        odf->commandOutput += "\n " + tempSS.str() + "\n";
    }

    // Send a chat message
    else if (command.compare("chat") == 0 || command.compare("c") == 0)
    {
        if (Socket::clientSocket != NULL)
        {
            sem_wait(&Socket::clientSocket->semaphore);
            Socket::clientSocket->send(formatCommand("chat", odf->gameMap->me->getNick() + ":"
                    + arguments));
            sem_post(&Socket::clientSocket->semaphore);
        }
        else if (Socket::serverSocket != NULL)
        {
            // Send the chat to all the connected clients
            for (unsigned int i = 0; i < odf->clientSockets.size(); ++i)
            {
                sem_wait(&odf->clientSockets[i]->semaphore);
                odf->clientSockets[i]->send(formatCommand("chat", odf->gameMap->me->getNick()
                        + ":" + arguments));
                sem_post(&odf->clientSockets[i]->semaphore);
            }

            // Display the chat message in our own message queue
            chatMessages.push_back(new ChatMessage(odf->gameMap->me->getNick(), arguments,
                    time(NULL), time(NULL)));
        }
        else
        {
            odf->commandOutput
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
                Creature *tempCreature = odf->gameMap->getCreature(arguments);
                if (tempCreature != NULL)
                {
                    if (!tempCreature->getHasVisualDebuggingEntities())
                    {
                        tempCreature->createVisualDebugEntities();
                        odf->commandOutput
                                += "\nVisual debugging entities created for creature:  "
                                        + arguments + "\n";
                    }
                    else
                    {
                        tempCreature->destroyVisualDebugEntities();
                        odf->commandOutput
                                += "\nVisual debugging entities destroyed for creature:  "
                                        + arguments + "\n";
                    }
                }
                else
                {
                    odf->commandOutput
                            += "\nCould not create visual debugging entities for creature:  "
                                    + arguments + "\n";
                }
            }
            else
            {
                odf->commandOutput
                        += "\nERROR:  You must supply a valid creature name to create debug entities for.\n";
            }
        }
        else
        {
            odf->commandOutput
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
            odf->playerColourValues.push_back(Ogre::ColourValue(tempR, tempG, tempB));
            tempSS.str("");
            tempSS << "Color number " << odf->playerColourValues.size() << " added.";
            odf->commandOutput += "\n" + tempSS.str() + "\n";
        }
        else
        {
            odf->commandOutput
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
            if (index < odf->playerColourValues.size())
            {
                odf->playerColourValues[index] = Ogre::ColourValue(tempR, tempG, tempB);
                tempSS.str("");
                tempSS << "Color number " << index << " changed to " << tempR
                        << "\t" << tempG << "\t" << tempB;
                odf->commandOutput += "an" + tempSS.str() + "\n";
            }

        }
        else
        {
            tempSS.str("");
            tempSS  << "ERROR:  You need to specify a color index between 0 and "
                    << odf->playerColourValues.size()
                    << " and an RGB triplet with values in (0.0, 1.0)";
            odf->commandOutput += "\n" + tempSS.str() + "\n";
        }
    }

    //FIXME:  This function is not yet implemented.
    else if (command.compare("disconnect") == 0)
    {
        odf->commandOutput += (Socket::serverSocket != NULL)
            ? "\nStopping server.\n"
            : (Socket::clientSocket != NULL)
                ? "\nDisconnecting from server.\n"
                : "\nYou are not connected to a server and you are not hosting a server.";
    }

    // Load the next level.
    else if (command.compare("next") == 0)
    {
        if (odf->gameMap->seatIsAWinner(odf->gameMap->me->getSeat()))
        {
            odf->gameMap->loadNextLevel = true;
            odf->commandOutput += (string) "\nLoading level levels/"
                    + odf->gameMap->nextLevel + ".level\n";
        }
        else
        {
            odf->commandOutput += "\nYou have not completed this level yet.\n";
        }
    }

    else
    {
        //try AngelScript interpreter
        return false;
        //odf->commandOutput
        //        += "\nCommand not found.  Try typing help to get info on how to use the console or just press enter to exit the console and return to the game.\n";
    }

    Console::getSingleton().print(odf->commandOutput);
    
    
    return true;
}




//TODO: make rest of commands scriptable
/*! \brief Process the commandline from the terminal and carry out the actions
 *  specified in by the user.
 */


/*! \brief A helper function to return a help text string for a given termianl command.
 *
 */
string Console::getHelpText(std::string arg)
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
        return "Help for command:  \"" + odf->arguments + "\" not found.";
    }
}
