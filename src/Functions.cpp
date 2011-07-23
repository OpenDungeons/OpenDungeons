//TODO: get rid of this whole file.
//      - The Gamemap/load/saving related stuff should go into GameMap or
//        ResourceManager or a new "Savegame" handler class.
//      - The server stuff should go into some of the network classes.
//      - The coloring stuff should go into rendering or player related classes

#include <CEGUI.h>

#include "Globals.h"
#include "Network.h"
#include "Socket.h"
#include "ServerNotification.h"
#include "Creature.h"
#include "GameMap.h"
#include "MapLight.h"
#include "Goal.h"
#include "Seat.h"
#include "Trap.h"
#include "Player.h"
#include "ODApplication.h"
#include "ODFrameListener.h"

#include "Functions.h"

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

bool readGameMapFromFile(const std::string& fileName)
{
    // Try to open the input file for reading and throw an error if we can't.
    std::ifstream baseLevelFile(fileName.c_str(), std::ifstream::in);
    if (!baseLevelFile.good())
    {
        std::cerr << "ERROR: File not found:  " << fileName << "\n\n\n";
        return false;
    }

    // Read in the whole baseLevelFile, strip it of comments and feed it into
    // the stringstream levelFile, to be read by the rest of the function.
    std::stringstream levelFile;
    std::string tempString;
    while (baseLevelFile.good())
    {
        getline(baseLevelFile, tempString);
        /* Find the first occurrence of the comment symbol on the
         * line and return everything before that character.
         */
        levelFile << tempString.substr(0, tempString.find('#')) << "\n";
    }

    baseLevelFile.close();

    // Read in the version number from the level file
    levelFile >> tempString;
    if (tempString.compare(ODApplication::VERSIONSTRING) != 0)
    {
        std::cerr
                << "\n\n\nERROR:  Attempting to load a file produced by a different version of OpenDungeons.\n"
                << "ERROR:  Filename:  " << fileName
                << "\nERROR:  The file is for OpenDungeons:  " << tempString
                << "\nERROR:  This version of OpenDungeons:  " << ODApplication::VERSION
                << "\n\n\n";
        exit(1);
    }

    gameMap.clearAll();

    // Read in the name of the next level to load after this one is complete.
    levelFile >> gameMap.nextLevel;

    int objectsToLoad = 0;

    // Read in the seats from the level file
    Seat* tempSeat;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempSeat = new Seat;
        levelFile >> tempSeat;

        gameMap.addEmptySeat(tempSeat);
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> objectsToLoad;
    Goal* tempGoal;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempGoal = Goal::instantiateFromStream(levelFile);

        if (tempGoal != NULL)
            gameMap.addGoalForAllSeats(tempGoal);
    }

    // Read in the map tiles from disk
    Tile* tempTile;
    levelFile >> objectsToLoad;
    gameMap.disableFloodFill();
    for (int i = 0; i < objectsToLoad; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        tempTile = new Tile;
        levelFile >> tempTile;

        gameMap.addTile(tempTile);
    }
    gameMap.enableFloodFill();

    // Loop over all the tiles and force them to examine their
    // neighbors.  This allows them to switch to a mesh with fewer
    // polygons if some are hidden by the neighbors.
    for(TileMap_t::iterator itr = gameMap.firstTile(), last = gameMap.lastTile();
            itr != last; ++itr)
    {
        itr->second->setFullness(itr->second->getFullness());
    }

    // Read in the rooms
    Room* tempRoom;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempRoom = Room::createRoomFromStream(levelFile);

        gameMap.addRoom(tempRoom);
    }

    // Read in the traps
    Trap* tempTrap;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempTrap = Trap::createTrapFromStream(levelFile);
        tempTrap->createMeshes();

        gameMap.addTrap(tempTrap);
    }

    // Read in the lights
    MapLight* tempLight;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempLight = new MapLight;
        levelFile >> tempLight;

        gameMap.addMapLight(tempLight);
    }

    // Read in the creature class descriptions
    CreatureClass* tempClass;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempClass = new CreatureClass;
        levelFile >> tempClass;

        gameMap.addClassDescription(tempClass);
    }

    // Read in the actual creatures themselves
    Creature* tempCreature;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {

        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and writeGameMapToFile.
        //Changes to this code should be reflected in that code as well
        tempCreature = new Creature;
        levelFile >> tempCreature;

        gameMap.addCreature(tempCreature);
    }

    return true;
}

void writeGameMapToFile(const std::string& fileName)
{
    std::ofstream levelFile(fileName.c_str(), std::ifstream::out);
    Tile *tempTile;

    // Write the identifier string and the version number
    levelFile << ODApplication::VERSIONSTRING
            << "  # The version of OpenDungeons which created this file (for compatability reasons).\n";

    // write out the name of the next level to load after this one is complete.
    levelFile << gameMap.nextLevel
            << " # The level to load after this level is complete.\n";

    // Write out the seats to the file
    levelFile << "\n# Seats\n" << gameMap.numEmptySeats()
            + gameMap.numFilledSeats() << "  # The number of seats to load.\n";
    levelFile << "# " << Seat::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numEmptySeats(); ++i)
    {
        levelFile << gameMap.getEmptySeat(i);
    }

    for (unsigned int i = 0; i < gameMap.numFilledSeats(); ++i)
    {
        levelFile << gameMap.getFilledSeat(i);
    }

    // Write out the goals shared by all players to the file.
    levelFile << "\n# Goals\n" << gameMap.numGoalsForAllSeats()
            << "  # The number of goals to load.\n";
    levelFile << "# " << Goal::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numGoalsForAllSeats(); i < num; ++i)
    {
        levelFile << gameMap.getGoalForAllSeats(i);
    }

    // Write out the tiles to the file
    levelFile << "\n# Tiles\n" << gameMap.numTiles()
            << "  # The number of tiles to load.\n";
    levelFile << "# " << Tile::getFormat() << "\n";
    TileMap_t::iterator itr = gameMap.firstTile();
    while (itr != gameMap.lastTile())
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        tempTile = itr->second;
        levelFile << tempTile->x << "\t" << tempTile->y << "\t";
        levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

        levelFile << std::endl;

        ++itr;
    }

    // Write out the rooms to the file
    levelFile << "\n# Rooms\n" << gameMap.numRooms()
            << "  # The number of rooms to load.\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numRooms(); i < num; ++i)
    {
        levelFile << gameMap.getRoom(i) << std::endl;
    }

    // Write out the traps to the file
    levelFile << "\n# Traps\n" << gameMap.numTraps()
            << "  # The number of traps to load.\n";
    levelFile << "# " << Trap::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numTraps(); ++i)
    {
        levelFile << gameMap.getTrap(i) << std::endl;
    }

    // Write out the lights to the file.
    levelFile << "\n# Lights\n" << gameMap.numMapLights()
            << "  # The number of lights to load.\n";
    levelFile << "# " << MapLight::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numMapLights(); i < num; ++i)
    {
        levelFile << gameMap.getMapLight(i) << std::endl;
    }

    // Write out the creature descriptions to the file
    levelFile << "\n# Creature classes\n" << gameMap.numClassDescriptions()
            << "  # The number of creature classes to load.\n";
    levelFile << CreatureClass::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numClassDescriptions(); ++i)
    {
        levelFile << gameMap.getClassDescription(i) << "\n";
    }

    // Write out the individual creatures to the file
    levelFile << "\n# Creatures\n" << gameMap.numCreatures()
            << "  # The number of creatures to load.\n";
    levelFile << "# " << Creature::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numCreatures(); i < num; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        levelFile << gameMap.getCreature(i) << std::endl;
    }

    levelFile << std::endl;

    levelFile.close();
}

void colourizeEntity(Ogre::Entity *ent, int colour)
{
    //Disabled for normal mapping. This has to be implemented in some other way.
    /*
    // Colorize the the textures
    // Loop over the sub entities in the mesh
    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);
        tempSubEntity->setMaterialName(colourizeMaterial(
                tempSubEntity->getMaterialName(), colour));
    }
    */
}

std::string colourizeMaterial(const std::string& materialName, int colour)
{
    std::stringstream tempSS;
    Ogre::Technique *tempTechnique;
    Ogre::Pass *tempPass;

    tempSS.str("");
    tempSS << "Color_" << colour << "_" << materialName;
    Ogre::MaterialPtr newMaterial = Ogre::MaterialPtr(
            Ogre::MaterialManager::getSingleton().getByName(tempSS.str()));

    //cout << "\nCloning material:  " << tempSS.str();

    // If this texture has not been copied and colourized yet then do so
    if (newMaterial.isNull())
    {
        // Check to see if we find a seat with the requested color, if not then just use the original, uncolored material.
        Seat* tempSeat = gameMap.getSeatByColor(colour);
        if (tempSeat == NULL)
            return materialName;

        std::cout << "\nMaterial does not exist, creating a new one.";
        newMaterial = Ogre::MaterialPtr(
                Ogre::MaterialManager::getSingleton().getByName(
                        materialName))->clone(tempSS.str());

        // Loop over the techniques for the new material
        for (unsigned int j = 0; j < newMaterial->getNumTechniques(); ++j)
        {
            tempTechnique = newMaterial->getTechnique(j);
            if (tempTechnique->getNumPasses() > 0)
            {
                tempPass = tempTechnique->getPass(0);
                tempPass->setSelfIllumination(tempSeat->colourValue);
            }
        }
    }

    return tempSS.str();

}

void queueServerNotification(ServerNotification *n)
{
    n->turnNumber = turnNumber.get();
    gameMap.threadLockForTurn(n->turnNumber);

    sem_wait(&serverNotificationQueueLockSemaphore);
    serverNotificationQueue.push_back(n);
    sem_post(&serverNotificationQueueLockSemaphore);

    sem_post(&serverNotificationQueueSemaphore);
}

bool startServer()
{
    // Start the server socket listener as well as the server socket thread
    if (serverSocket == NULL && clientSocket == NULL && gameMap.numEmptySeats()
            > 0)
    {
        //NOTE: Code added to this routine may also need to be added to GameMap::doTurn() in the "loadNextLevel" stuff.
        // Sit down at the first available seat.
        gameMap.me->setSeat(gameMap.popEmptySeat());

        serverSocket = new Socket;

        // Start the server thread which will listen for, and accept, connections
        SSPStruct* ssps = new SSPStruct;
        ssps->nSocket = serverSocket;
        ssps->nFrameListener = ODFrameListener::getSingletonPtr();
        pthread_create(&ODFrameListener::getSingletonPtr()->serverThread,
                NULL, serverSocketProcessor, (void*) ssps);

        // Start the thread which will watch for local events to send to the clients
        SNPStruct* snps = new SNPStruct;
        snps->nFrameListener = ODFrameListener::getSingletonPtr();
        pthread_create(&ODFrameListener::getSingletonPtr()->serverNotificationThread,
                NULL, serverNotificationProcessor, snps);

        // Start the creature AI thread
        pthread_create(&ODFrameListener::getSingletonPtr()->creatureThread,
                NULL, creatureAIThread, NULL);

        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
        gameMap.clearMapLightIndicators();

        // Set the active tabs on the tab selector across the bottom of the screen so
        // the user doesn't have to click into them first to see the contents.
        static_cast<CEGUI::TabControl*>(CEGUI::WindowManager::getSingletonPtr()->
                getWindow("Root/MainTabControl"))->setSelectedTab(0);
    }

    return true;
}
