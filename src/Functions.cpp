#include "Functions.h"

#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreTexture.h>
#include <OgreMaterial.h>

#if defined(WIN32) || defined(_WIN32)
//TODO: Add the proper windows include file for this (handling directory listings).
#else
#include <dirent.h>
#endif

#include "Globals.h"
#include "Defines.h"
#include "Creature.h"
#include "MapLight.h"
#include "Network.h"
#include "Goal.h"
#include "Seat.h"
#include "Trap.h"
#include "GameMap.h"
#include "RenderRequest.h"
#include "ServerNotification.h"
#include "ProtectedObject.h"
#include "Player.h"
#include "CreatureAction.h"
#include "CreatureSound.h"

//#if defined(WIN32) || defined(_WIN32)
//double const M_PI = 2 * acos(0.0);
//Changed this to be more consistant with the definition
//in the gnu headers
#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif

bool readGameMapFromFile(const std::string& fileName)
{
    Seat *tempSeat;
    Goal *tempGoal;
    Tile *tempTile;
    Room *tempRoom;
    Trap *tempTrap;
    MapLight *tempLight;
    std::string tempString, tempString2;
    Creature tempCreature;
    int objectsToLoad;

    // Try to open the input file for reading and throw an error if we can't.
    ifstream baseLevelFile(fileName.c_str(), ifstream::in);
    if (!baseLevelFile.good())
    {
        cerr << "ERROR: File not found:  " << fileName << "\n\n\n";
        return false;
    }

    // Read in the whole baseLevelFile, strip it of comments and feed it into
    // the stringstream levelFile, to be read by the rest of the function.
    std::stringstream levelFile;
    while (baseLevelFile.good())
    {
        getline(baseLevelFile, tempString);
        levelFile << stripCommentsFromLine(tempString) << "\n";
    }

    baseLevelFile.close();

    // Read in the version number from the level file
    levelFile >> tempString;
    if (tempString.compare(versionString) != 0)
    {
        cerr
                << "\n\n\nERROR:  Attempting to load a file produced by a different version of OpenDungeons.\n";
        cerr << "ERROR:  Filename:  " << fileName;
        cerr << "\nERROR:  The file is for OpenDungeons:  " << tempString;
        cerr << "\nERROR:  This version of OpenDungeons:  " << versionString
                << "\n\n\n";
        exit(1);
    }

    gameMap.clearAll();

    // Read in the name of the next level to load after this one is complete.
    levelFile >> gameMap.nextLevel;

    // Read in the seats from the level file
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempSeat = new Seat;
        levelFile >> tempSeat;

        gameMap.addEmptySeat(tempSeat);
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempGoal = Goal::instantiateFromStream(levelFile);

        if (tempGoal != NULL)
            gameMap.addGoalForAllSeats(tempGoal);
    }

    // Read in the map tiles from disk
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
    TileMap_t::iterator itr = gameMap.firstTile();
    while (itr != gameMap.lastTile())
    {
        itr->second->setFullness(itr->second->getFullness());
        ++itr;
    }

    // Read in the rooms
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempRoom = Room::createRoomFromStream(levelFile);

        gameMap.addRoom(tempRoom);
    }

    // Read in the traps
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempTrap = Trap::createTrapFromStream(levelFile);
        tempTrap->createMeshes();

        gameMap.addTrap(tempTrap);
    }

    // Read in the lights
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempLight = new MapLight;
        levelFile >> tempLight;

        gameMap.addMapLight(tempLight);
    }

    // Read in the creature class descriptions
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        CreatureClass *tempClass = new CreatureClass;
        levelFile >> tempClass;

        gameMap.addClassDescription(tempClass);
    }

    // Read in the actual creatures themselves
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {

        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and writeGameMapToFile.
        //Changes to this code should be reflected in that code as well
        Creature *newCreature = new Creature;

        levelFile >> newCreature;

        gameMap.addCreature(newCreature);
    }

    return true;
}

void writeGameMapToFile(const std::string& fileName)
{
    ofstream levelFile(fileName.c_str(), ifstream::out);
    Tile *tempTile;

    // Write the identifier string and the version number
    levelFile << versionString
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
    for (unsigned int i = 0; i < gameMap.numGoalsForAllSeats(); ++i)
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

        levelFile << endl;

        ++itr;
    }

    // Write out the rooms to the file
    levelFile << "\n# Rooms\n" << gameMap.numRooms()
            << "  # The number of rooms to load.\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numRooms(); ++i)
    {
        levelFile << gameMap.getRoom(i) << endl;
    }

    // Write out the traps to the file
    levelFile << "\n# Traps\n" << gameMap.numTraps()
            << "  # The number of traps to load.\n";
    levelFile << "# " << Trap::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numTraps(); ++i)
    {
        levelFile << gameMap.getTrap(i) << endl;
    }

    // Write out the lights to the file.
    levelFile << "\n# Lights\n" << gameMap.numMapLights()
            << "  # The number of lights to load.\n";
    levelFile << "# " << MapLight::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numMapLights(); ++i)
    {
        levelFile << gameMap.getMapLight(i) << endl;
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
    for (unsigned int i = 0; i < gameMap.numCreatures(); ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        levelFile << gameMap.getCreature(i) << endl;
    }

    levelFile << endl;

    levelFile.close();
}

namespace Random
{
unsigned long myRandomSeed = 14329;
unsigned long MAX = 32768;

// This is a very simple random number generator.  It is nothing fancy
// but should be good enough for our purposes.  The rand() function
// behave slightly differently on different systems and was causing
// bugs so it was replaced with this one.
unsigned long randgen()
{
    sem_wait(&randomGeneratorLockSemaphore);
    myRandomSeed = myRandomSeed * 1103515245 + 12345;
    unsigned long returnVal = (unsigned int) (myRandomSeed / 65536) % 32768;
    sem_post(&randomGeneratorLockSemaphore);

    return returnVal;
}

// uniformly distributed number [0;1)
double uniform()
{
    return randgen() * (1.0 / ((double) MAX));
}

// uniformly distributed number [0;hi)
double uniform(double hi)
{
    return uniform() * hi;
}

// uniformly distributed number [lo;hi)
double uniform(double lo, double hi)
{
    return (uniform() * (hi - lo)) + lo;
}

// random bit
signed randint()
{
    return uniform() > 0.5 ? 1 : 0;
}

// random integer [0;hi]
signed randint(signed hi)
{
    return static_cast<signed> (uniform() * (hi + 1));
}

// random integer [lo;hi]
signed randint(signed lo, signed hi)
{
    return static_cast<signed> (lo + (uniform() * ((hi + 1) - lo)));
}

// random unsigned integer [0;hi]
unsigned int randuint(unsigned int hi)
{
    return static_cast<unsigned int> (uniform() * (hi + 1));
}

// random unsigned integer [lo;hi]
unsigned int randuint(unsigned int lo, unsigned int hi)
{
    return static_cast<unsigned int> (lo + (uniform() * ((hi + 1) - lo)));
}
} //end namespace Random

/************************************************
 *randomDouble() returns a double between the
 *lower number entered and the higher number
 *entered.  One or both numbers can be negative
 ************************************************/
double randomDouble(double min, double max)
{
    if (min > max)
    {
        double temp = min;
        min = max;
        max = temp;
    }

    return Random::uniform(min, max);
}

int randomInt(int min, int max)
{
    if (min > max)
    {
        int temp = min;
        min = max;
        max = temp;
    }

    return Random::randint(min, max);
}

unsigned int randomUint(unsigned int min, unsigned int max)
{
    if (min > max)
    {
        unsigned int temp = min;
        min = max;
        max = temp;
    }

    return Random::randuint(min, max);
}

//returns a gaussian distributed random double value in [-1,1]
double gaussianRandomDouble()
{
    double temp1 = randomDouble(0.0, 1.0);
    double temp2 = randomDouble(0.0, 1.0);

    double val1;
    //double val2;

    val1 = sqrt(-2.0 * log(temp1)) * cos(2.0 * M_PI * temp2);
    // val2 = sqrt(-2.0*log(temp1)) * sin(2.0*M_PI*temp2);

    return val1;
}

void seedRandomNumberGenerator()
{
    Random::myRandomSeed = time(0);
}

void swap(int &a, int &b)
{
    int temp = a;
    a = b;
    b = temp;
}

string stripCommentsFromLine(string line)
{
    // Find the first occurrence of the comment symbol on the line and return everything before that character.
    size_t index = line.find('#');
    return line.substr(0, index);
}

void colourizeEntity(Ogre::Entity *ent, int colour)
{
    // Colorize the the textures
    // Loop over the sub entities in the mesh
    for (unsigned int i = 0; i < ent->getNumSubEntities(); ++i)
    {
        Ogre::SubEntity *tempSubEntity = ent->getSubEntity(i);
        tempSubEntity->setMaterialName(colourizeMaterial(
                tempSubEntity->getMaterialName(), colour));
    }
}

std::string colourizeMaterial(const std::string& materialName, int colour)
{
    std::string tempString;
    std::stringstream tempSS;
    Ogre::HardwarePixelBufferSharedPtr tempPixBuf;
    Ogre::PixelBox tempPixelBox;
    Ogre::Technique *tempTechnique;
    Ogre::Pass *tempPass;
    Ogre::TexturePtr tempTexture;

    tempSS.str("");
    tempSS << "Color_" << colour << "_" << materialName;
    Ogre::MaterialPtr newMaterial = Ogre::MaterialPtr(
            Ogre::MaterialManager::getSingleton().getByName(tempSS.str()));

    //cout << "\nCloning material:  " << tempSS.str();

    // If this texture has not been copied and colourized yet then do so
    if (newMaterial.isNull())
    {
        // Check to see if we find a seat with the requested color, if not then just use the original, uncolored material.
        Seat *tempSeat = gameMap.getSeatByColor(colour);
        if (tempSeat == NULL)
            return materialName;

        std::cout << "\nMaterial does not exist, creating a new one.";
        newMaterial
                = Ogre::MaterialPtr(Ogre::MaterialManager::getSingleton().getByName(
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

void queueRenderRequest(RenderRequest *r)
{
    r->turnNumber = turnNumber.get();
    gameMap.threadLockForTurn(r->turnNumber);

    sem_wait(&renderQueueSemaphore);
    renderQueue.push_back(r);
    sem_post(&renderQueueSemaphore);
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

std::vector<std::string> listAllFiles(const std::string& directoryName)
{
    std::vector<std::string> tempVector;

#if defined(WIN32) || defined(_WIN32)
    //TODO: Add the proper code to do this under windows.
#else
    DIR *d;
    struct dirent *dir;
    d = opendir(directoryName.c_str());
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            tempVector.push_back(dir->d_name);
        }

        closedir(d);
    }
#endif
    return tempVector;
}

//NOTE: This function has not yet been tested.
void waitOnRenderQueueFlush()
{
    numThreadsWaitingOnRenderQueueEmpty.lock();
    unsigned int tempUnsigned = numThreadsWaitingOnRenderQueueEmpty.rawGet();
    ++tempUnsigned;
    numThreadsWaitingOnRenderQueueEmpty.rawSet(tempUnsigned);
    numThreadsWaitingOnRenderQueueEmpty.unlock();

    sem_wait(&renderQueueEmptySemaphore);
}

bool startServer()
{
    // Start the server socket listener as well as the server socket thread
    if (serverSocket == NULL && clientSocket == NULL && gameMap.numEmptySeats()
            > 0)
    {
        //NOTE: Code added to this routine may also need to be added to GameMap::doTurn() in the "loadNextLevel" stuff.
        // Sit down at the first available seat.
        gameMap.me->seat = gameMap.popEmptySeat();

        serverSocket = new Socket;

        // Start the server thread which will listen for, and accept, connections
        SSPStruct *ssps = new SSPStruct;
        ssps->nSocket = serverSocket;
        ssps->nFrameListener = exampleFrameListener;
        pthread_create(&exampleFrameListener->serverThread, NULL,
                serverSocketProcessor, (void*) ssps);

        // Start the thread which will watch for local events to send to the clients
        SNPStruct *snps = new SNPStruct;
        snps->nFrameListener = exampleFrameListener;
        pthread_create(&exampleFrameListener->serverNotificationThread, NULL,
                serverNotificationProcessor, snps);

        // Start the creature AI thread
        pthread_create(&exampleFrameListener->creatureThread, NULL,
                creatureAIThread, NULL);

        // Destroy the meshes associated with the map lights that allow you to see/drag them in the map editor.
        gameMap.clearMapLightIndicators();

        // Set the active tabs on the tab selector across the bottom of the screen so
        // the user doesn't have to click into them first to see the contents.
        CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();
        CEGUI::Window *window;
        window = wmgr->getWindow((CEGUI::utf8*) "Root/MainTabControl");
        ((CEGUI::TabControl*) window)->setSelectedTab(0);
    }

    return true;
}
