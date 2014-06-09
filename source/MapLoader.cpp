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

#include "MapLoader.h"

#include "GameMap.h"
#include "ODApplication.h"
#include "Goal.h"
#include "Creature.h"
#include "CreatureDefinition.h"
#include "Trap.h"
#include "Seat.h"
#include "MapLight.h"
#include "LogManager.h"
#include "Helper.h"


#include <iostream>
#include <sstream>


using Ogre::uchar;

namespace MapLoader {

bool writeGameMapFromTgaFile(const std::string& fileName, GameMap& gameMap)
{
    Ogre::Image mapImage;
    uchar* pictureBuffer;
    int numFaces = 1;
    pictureBuffer = new uchar [numFaces*Ogre::PixelUtil::getMemorySize( gameMap.getMapSizeX(), gameMap.getMapSizeY(), 1, Ogre::PF_R8G8B8 )];
    mapImage.loadDynamicImage(pictureBuffer, gameMap.getMapSizeX(), gameMap.getMapSizeY(), 1, Ogre::PF_R8G8B8 );

    Ogre::ColourValue cv;
    for(int ii = 0 ; ii <  gameMap.getMapSizeX(); ii++)
	for(int jj = 0 ; jj <  gameMap.getMapSizeY(); jj++)
	    {
		cv.r = cv.g = cv.b = 0.1 * static_cast<float>(gameMap.getTile(ii, jj)->getType());
		mapImage.setColourAt(cv, ii, jj, 0);
	    }
    mapImage.save(fileName);
    delete [] pictureBuffer ;
}

bool readGameMapFromTgaFile(const std::string& fileName, GameMap& gameMap)
{
    Ogre::Image img;
    Ogre::ColourValue cv;
    bool image_loaded = false;
    std::ifstream ifs(fileName.c_str(), std::ios::binary|std::ios::in);
    if (ifs.is_open())
    {
	Ogre::String tex_ext;
	Ogre::String::size_type index_of_extension = fileName.find_last_of('.');
	if (index_of_extension != Ogre::String::npos)
	{
	    tex_ext = fileName.substr(index_of_extension+1);
	    Ogre::DataStreamPtr data_stream(new Ogre::FileStreamDataStream(fileName, &ifs, false));

	    img.load(data_stream, tex_ext);
	    image_loaded = true;
	}
	ifs.close();
    }
    for(int ii = 0 ; ii <  gameMap.getMapSizeX(); ii++)
	for(int jj = 0 ; jj <  gameMap.getMapSizeY(); jj++)
	    {
		cv = img.getColourAt( ii , jj, 0);
		gameMap.getTile(ii, jj)->setType(static_cast<Tile::TileType>(10.0*cv.r));
	    }
    return image_loaded;
}

bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap)
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
    std::string nextParam;
    while (baseLevelFile.good())
    {
        std::getline(baseLevelFile, nextParam);
        /* Find the first occurrence of the comment symbol on the
         * line and return everything before that character.
         */
        levelFile << nextParam.substr(0, nextParam.find('#')) << "\n";
    }

    baseLevelFile.close();

    // Read in the version number from the level file
    levelFile >> nextParam;
    if (nextParam.compare(ODApplication::VERSIONSTRING) != 0)
    {
        std::cerr
                << "\n\n\nERROR:  Attempting to load a file produced by a different version of OpenDungeons.\n"
                << "ERROR:  Filename:  " << fileName
                << "\nERROR:  The file is for OpenDungeons:  " << nextParam
                << "\nERROR:  This version of OpenDungeons:  " << ODApplication::VERSION
                << "\n\n\n";
        return false;
    }

    levelFile >> nextParam;
    if (nextParam != "[Seats]")
    {
        std::cout << "Invalid seats start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    // Read in the seats from the level file
    while (true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Seats]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << entire_line << std::endl;

        Seat* tempSeat = new Seat;
        Seat::loadFromLine(entire_line, tempSeat);

        gameMap.addEmptySeat(tempSeat);
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> nextParam;
    if (nextParam != "[Goals]")
    {
        std::cout << "Invalid Goals start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Goals]")
            break;

        Goal* tempGoal = Goal::instantiateFromStream(nextParam, levelFile);

        if (tempGoal != NULL)
            gameMap.addGoalForAllSeats(tempGoal);
    }

    levelFile >> nextParam;
    if (nextParam != "[Tiles]")
    {
        std::cout << "Invalid tile start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    // Load the map size on next two lines
    int mapSizeX;
    int mapSizeY;
    levelFile >> mapSizeX;
    levelFile >> mapSizeY;

    if (!gameMap.createNewMap(mapSizeX, mapSizeY))
        return false;

    // Read in the map tiles from disk
    Tile tempTile;
    tempTile.setGameMap(&gameMap);

    gameMap.disableFloodFill();

    while (true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Tiles]")
            break;

        // Get all the params together in order to prepare for the new parsing function
        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        Tile::loadFromLine(entire_line, &tempTile);

        gameMap.addTile(tempTile);
    }

    gameMap.setAllFullnessAndNeighbors();
    gameMap.enableFloodFill();

    // Read in the rooms
    levelFile >> nextParam;
    if (nextParam != "[Rooms]")
    {
        std::cout << "Invalid Rooms start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Rooms]")
            break;

        Room* tempRoom = Room::createRoomFromStream(nextParam, levelFile, &gameMap);

        gameMap.addRoom(tempRoom);
    }

    // Read in the traps
    levelFile >> nextParam;
    if (nextParam != "[Traps]")
    {
        std::cout << "Invalid Traps start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Traps]")
            break;

        Trap* tempTrap = Trap::createTrapFromStream(nextParam, levelFile, &gameMap);
        tempTrap->createMesh();

        gameMap.addTrap(tempTrap);
    }

    // Read in the lights
    levelFile >> nextParam;
    if (nextParam != "[Lights]")
    {
        std::cout << "Invalid Lights start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Lights]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        MapLight* tempLight = new MapLight;
        tempLight->setGameMap(&gameMap);
        MapLight::loadFromLine(entire_line, tempLight);

        gameMap.addMapLight(tempLight);
    }

    // Load the creatures defintions filename and file.
    levelFile >> nextParam;
    if (nextParam != "[Creatures_Definition]")
    {
        std::cout << "Invalid Creatures definition start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }
    levelFile >> nextParam;
    MapLoader::loadCreatureDefinition(nextParam, gameMap);

    // Read in the creatures spawn pool for each team
    levelFile >> nextParam;
    if (nextParam != "[Spawn_Pools]")
    {
        std::cout << "Invalid 'Spawn Pools' start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Spawn_Pools]")
            break;

        if (nextParam != "[Spawn_Pool]")
        {
            std::cout << "Invalid 'Spawn Pool' start format." << std::endl;
            std::cout << "Line was " << nextParam << std::endl;
            return false;
        }

        levelFile >> nextParam;
        if (nextParam == "[/Spawn_Pool]")
            continue;

        // Get the corresponding seat
        int team_color = Helper::toInt(nextParam);
        Seat* seat = gameMap.getSeatByColor(team_color);
        if (seat != NULL)
            seat->resetSpawnPool();

        std::cout << "Spawn pool for team: " << team_color << std::endl;

        while(true)
        {
            levelFile >> nextParam;
            if (nextParam == "[/Spawn_Pool]")
                break;

            if (seat != NULL)
                seat->addSpawnableCreature(nextParam);
            std::cout << "Added spawnable creature: " << nextParam << std::endl;
        }
    }

    // Read in the actual creatures themselves
    levelFile >> nextParam;
    if (nextParam != "[Creatures]")
    {
        std::cout << "Invalid Creatures start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Creatures]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        Creature* tempCreature = new Creature(&gameMap);
        Creature::loadFromLine(entire_line, tempCreature);

        gameMap.addCreature(tempCreature);
    }

    return true;
}

void writeGameMapToFile(const std::string& fileName, GameMap& gameMap)
{
    std::ofstream levelFile(fileName.c_str(), std::ifstream::out);
    Tile *tempTile;

    // Write the identifier string and the version number
    levelFile << ODApplication::VERSIONSTRING
            << "  # The version of OpenDungeons which created this file (for compatibility reasons).\n";

    // Write out the seats to the file
    levelFile << "\n[Seats]\n";
    levelFile << "# " << Seat::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numEmptySeats(); ++i)
    {
        levelFile << gameMap.getEmptySeat(i);
    }

    for (unsigned int i = 0; i < gameMap.numFilledSeats(); ++i)
    {
        levelFile << gameMap.getFilledSeat(i);
    }
    levelFile << "[/Seats]" << std::endl;

    // Write out the goals shared by all players to the file.
    levelFile << "\n[Goals]\n";
    levelFile << "# " << Goal::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numGoalsForAllSeats(); i < num; ++i)
    {
        levelFile << gameMap.getGoalForAllSeats(i);
    }
    levelFile << "[/Goals]" << std::endl;

    levelFile << "\n[Tiles]\n";
    int mapSizeX = gameMap.getMapSizeX();
    int mapSizeY = gameMap.getMapSizeY();
    levelFile << "# Map Size" << std::endl;
    levelFile << mapSizeX << " # MapSizeX" << std::endl;
    levelFile << mapSizeY << " # MapSizeY" << std::endl;

    // Write out the tiles to the file
    levelFile << "# " << Tile::getFormat() << "\n";

    for(int ii = 0; ii < gameMap.getMapSizeX(); ++ii)
    {
        for(int jj = 0; jj < gameMap.getMapSizeY(); ++jj)
        {
            tempTile = gameMap.getTile(ii, jj);
            // Don't save standard tiles as they're auto filled in at load time.
            if (tempTile->getType() == Tile::dirt && tempTile->getFullness() >= 100.0)
                continue;

            levelFile << tempTile->x << "\t" << tempTile->y << "\t";
            levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

            levelFile << std::endl;
        }
    }
    levelFile << "[/Tiles]" << std::endl;

    // Write out the rooms to the file
    levelFile << "\n[Rooms]\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numRooms(); i < num; ++i)
    {
        levelFile << gameMap.getRoom(i);
    }
    levelFile << "[/Rooms]" << std::endl;

    // Write out the traps to the file
    levelFile << "\n[Traps]\n";
    levelFile << "# " << Trap::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap.numTraps(); ++i)
    {
        levelFile << gameMap.getTrap(i);
    }
    levelFile << "[/Traps]" << std::endl;

    // Write out the lights to the file.
    levelFile << "\n[Lights]\n";
    levelFile << "# " << MapLight::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numMapLights(); i < num; ++i)
    {
        levelFile << gameMap.getMapLight(i) << std::endl;
    }
    levelFile << "[/Lights]" << std::endl;

    // Write out where to find the creatures definition file.
    levelFile << "\n# The file containing the creatures definition." << std::endl
    << "[Creatures_Definition]" << std::endl
    << gameMap.getCreatureDefinitionFileName() << std::endl;

    // Write out the spawn pools
    levelFile << "\n[Spawn_Pools]\n";
    levelFile << "# Each team's spawn pool." << std::endl;
    levelFile << "# Describes what each team can spawn." << std::endl << std::endl;

    // Creates a common seat vector
    std::vector<Seat*> seats;
    for (unsigned int i = 0; i < gameMap.numFilledSeats(); ++i)
    {
        Seat* seat = gameMap.getFilledSeat(i);
        if (seat == NULL)
            continue;

        bool foundDuplicate = false;
        for (unsigned int j = 0; j < seats.size(); ++j)
        {
            if (seats[j] == seat)
            {
                foundDuplicate = true;
                break;
            }
        }

        if (!foundDuplicate)
            seats.push_back(seat);
    }

    for (unsigned int i = 0; i < gameMap.numEmptySeats(); ++i)
    {
        Seat* seat = gameMap.getEmptySeat(i);
        if (seat == NULL)
            continue;

        bool foundDuplicate = false;
        for (unsigned int j = 0; j < seats.size(); ++j)
        {
            if (seats[j] == seat)
            {
                foundDuplicate = true;
                break;
            }
        }

        if (!foundDuplicate)
            seats.push_back(seat);
    }

    // For each seat, add the spawn pool
    for (unsigned int i = 0; i < seats.size(); ++i)
    {
        // Write out each spawn pools
        Seat* seat = seats[i];
        if (seat == NULL)
            continue;

        int team_color = seat->getColor();
        if (team_color == 0)
            continue;

        const std::vector<std::string>& spawnPool = seat->getSpawnPool();
        if (spawnPool.empty())
            continue;

        levelFile << "[Spawn_Pool]" << std::endl
            << "# team id" << std::endl
            << team_color << std::endl
            << "# Creature list" << std::endl;

        for (unsigned int j = 0; j < spawnPool.size(); ++j)
            levelFile << spawnPool.at(j) << std::endl;

        levelFile << "[/Spawn_Pool]" << std::endl << std::endl;
    }

    levelFile << "[/Spawn_Pools]" << std::endl;

    // Write out the individual creatures to the file
    levelFile << "\n[Creatures]\n";
    levelFile << "# " << Creature::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap.numCreatures(); i < num; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        levelFile << gameMap.getCreature(i) << std::endl;
    }
    levelFile << "[/Creatures]" << std::endl;

    levelFile.close();
}

bool loadCreatureDefinition(const std::string& fileName, GameMap& gameMap)
{
    // First, clear the previous creature definitions
    gameMap.clearClasses();
    gameMap.setCreatureDefinitionFileName(fileName);

    std::cout << "Load creature definition file: " << fileName << std::endl;

    // Try to open the input file for reading and throw an error if we can't.
    std::ifstream creatureDefFile(fileName.c_str(), std::ifstream::in);
    if (!creatureDefFile.good())
    {
        std::cout << "ERROR: Creature definition file not found:  " << fileName << "\n\n\n";
        return false;
    }

    // Read in the whole creatureDefFile, strip it of comments and feed it into
    // the stringstream defFile, to be read by the rest of the function.
    std::stringstream defFile;
    std::string nextParam;
    while (creatureDefFile.good())
    {
        std::getline(creatureDefFile, nextParam);
        /* Find the first occurrence of the comment symbol on the
         * line and return everything before that character.
         */
        defFile << nextParam.substr(0, nextParam.find('#')) << "\n";
    }

    creatureDefFile.close();

    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Creatures_definition]")
    {
        std::cout << "Invalid Creature classes start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    while(true)
    {
        defFile >> nextParam;
        if (nextParam == "[/Creatures_definition]")
            break;

        std::string entire_line = nextParam;
        std::getline(defFile, nextParam);
        entire_line += nextParam;
        //std::cout << entire_line << std::endl;

        CreatureDefinition* tempClass = new CreatureDefinition;
        CreatureDefinition::loadFromLine(entire_line, tempClass);

        gameMap.addClassDescription(tempClass);
    }
    return true;
}

} // Namespace MapLoader
