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
#include "ResourceManager.h"

#include <iostream>
#include <sstream>

namespace MapLoader {

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
    if (nextParam != "[Info]")
    {
        std::cout << "Invalid info start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    // Read in the seats from the level file
    while (true)
    {
        // Information can contain spaces. We need to use std::getline to get content
        std::getline(levelFile, nextParam);
        std::string param;
        if (nextParam == "[/Info]")
        {
            break;
        }

        param = "Name\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            gameMap.setLevelName(nextParam.substr(param.size()));
            continue;
        }

        param = "Description\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            gameMap.setLevelDescription(nextParam.substr(param.size()));
            continue;
        }

        param = "Music\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            std::string musicFile = nextParam.substr(param.size());
            gameMap.setLevelMusicFile(musicFile);
            LogManager::getSingleton().logMessage("Level Music: " + musicFile);
            continue;
        }

        param = "FightMusic\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            std::string musicFile = nextParam.substr(param.size());
            gameMap.setLevelFightMusicFile(nextParam.substr(param.size()));
            LogManager::getSingleton().logMessage("Level Fight Music: " + musicFile);
            continue;
        }
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

        Goal* tempGoal = Goal::instantiateFromStream(nextParam, levelFile, &gameMap);

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

        Tile* tempTile = new Tile(&gameMap);

        Tile::loadFromLine(entire_line, tempTile);

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

        Room* tempRoom = Room::createRoomFromStream(&gameMap,nextParam, levelFile,
            std::string());

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

        Trap* tempTrap = Trap::createTrapFromStream(&gameMap, nextParam, levelFile,
            std::string());
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

        MapLight* tempLight = new MapLight(&gameMap, true);
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
    if (!MapLoader::loadCreatureDefinition(nextParam, gameMap))
        return false;

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
        int seatId = Helper::toInt(nextParam);
        Seat* seat = gameMap.getSeatById(seatId);
        if (seat != NULL)
            seat->resetSpawnPool();

        std::cout << "Spawn pool for seat id: " << seatId << std::endl;

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

        Creature* tempCreature = Creature::loadFromLine(entire_line, &gameMap);

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

    // Write map info
    levelFile << "\n[Info]\n";
    levelFile << "Name\t" << gameMap.getLevelName() << std::endl;
    levelFile << "Description\t" << gameMap.getLevelDescription() << std::endl;
    levelFile << "Music\t" << gameMap.getLevelMusicFile() << std::endl;
    levelFile << "FightMusic\t" << gameMap.getLevelFightMusicFile() << std::endl;
    levelFile << "[/Info]" << std::endl;


    // Write out the seats to the file
    levelFile << "\n[Seats]\n";
    levelFile << "# " << Seat::getFormat() << "\n";
    std::vector<Seat*> seats;
    for (unsigned int i = 0; i < gameMap.numEmptySeats(); ++i)
    {
        seats.push_back(gameMap.getEmptySeat(i));
    }

    for (unsigned int i = 0; i < gameMap.numFilledSeats(); ++i)
    {
        seats.push_back(gameMap.getFilledSeat(i));
    }

    std::sort(seats.begin(), seats.end(), Seat::sortForMapSave);
    for (std::vector<Seat*>::iterator it = seats.begin(); it != seats.end(); ++it)
    {
        Seat* seat = *it;
        levelFile << seat << std::endl;
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

            levelFile << tempTile << std::endl;
        }
    }
    levelFile << "[/Tiles]" << std::endl;


    std::vector<Room*> rooms;
    for (unsigned int i = 0, num = gameMap.numRooms(); i < num; ++i)
    {
        rooms.push_back(gameMap.getRoom(i));
    }

    std::sort(rooms.begin(), rooms.end(), Room::sortForMapSave);

    // Write out the rooms to the file
    levelFile << "\n[Rooms]\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (std::vector<Room*>::iterator it = rooms.begin(); it != rooms.end(); ++it)
    {
        Room* room = *it;
        levelFile << room;
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

    // For each seat, add the spawn pool
    for (std::vector<Seat*>::iterator it = seats.begin(); it != seats.end(); ++it)
    {
        // Write out each spawn pools
        Seat* seat = *it;
        int seatId = seat->getId();
        if (seatId == 0)
            continue;

        const std::vector<std::string>& spawnPool = seat->getSpawnPool();
        if (spawnPool.empty())
            continue;

        levelFile << "[Spawn_Pool]" << std::endl
            << "# seat id" << std::endl
            << seatId << std::endl
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
    std::ifstream creatureDefFile(ResourceManager::getSingleton().getResourcePath() + fileName.c_str(), std::ifstream::in);
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

LevelInfo getMapInfo(const std::string& fileName)
{
    // Prepare an invalid level reference
    static LevelInfo invalidLevel;
    invalidLevel.mLevelName = "Invalid map!";
    invalidLevel.mLevelDescription = invalidLevel.mLevelName;

    // Try to open the input file for reading and throw an error if we can't.
    std::ifstream baseLevelFile(fileName.c_str(), std::ifstream::in);
    if (!baseLevelFile.good())
        return invalidLevel;

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
        return invalidLevel;

    levelFile >> nextParam;
    if (nextParam != "[Info]")
        return invalidLevel;

    std::stringstream mapInfo;
    LevelInfo levelInfo;

    // Read in the seats from the level file
    while (true)
    {
        // Information can contain spaces. We need to use std::getline to get content
        std::getline(levelFile, nextParam);
        std::string param;
        if (nextParam == "[/Info]")
        {
            break;
        }

        param = "Name\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            levelInfo.mLevelName = nextParam.substr(param.size());
            mapInfo << levelInfo.mLevelName << std::endl << std::endl;
            continue;
        }

        param = "Description\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            mapInfo << nextParam.substr(param.size()) << std::endl << std::endl;
            continue;
        }

    }

    levelFile >> nextParam;
    if (nextParam != "[Seats]")
    {
        levelInfo.mLevelDescription = mapInfo.str();
        return levelInfo;
    }

    // Read in the seats from the level file
    int playerSeatNumber = 0;
    int AISeatNumber = 0;
    while (true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Seats]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << entire_line << std::endl;

        Seat* seat = new Seat;
        Seat::loadFromLine(entire_line, seat);

        if (seat->getFaction() == "Player")
            ++playerSeatNumber;
        else if (seat->getFaction() == "KeeperAI")
            ++AISeatNumber;

        delete seat;
    }

    if (playerSeatNumber > 0 || AISeatNumber > 0)
    {
        if (playerSeatNumber > 0)
            mapInfo << "Player slot(s): " << playerSeatNumber;
        if (playerSeatNumber > 0 && AISeatNumber > 0)
            mapInfo << " / AI: " << AISeatNumber;
        else if (AISeatNumber > 0)
            mapInfo << "AI: " << AISeatNumber;

        mapInfo << std::endl << std::endl;
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> nextParam;
    if (nextParam != "[Goals]")
    {
        levelInfo.mLevelDescription = mapInfo.str();
        return levelInfo;
    }

    while(true)
    {
        levelFile >> nextParam;
        if (nextParam == "[/Goals]")
            break;
    }

    levelFile >> nextParam;
    if (nextParam != "[Tiles]")
    {
        levelInfo.mLevelDescription = mapInfo.str();
        return levelInfo;
    }

    // Load the map size on next two lines
    int mapSizeX;
    int mapSizeY;
    levelFile >> mapSizeX;
    levelFile >> mapSizeY;

    mapInfo << "Size: " << mapSizeX << "x" << mapSizeY << std::endl << std::endl;

    levelInfo.mLevelDescription = mapInfo.str();
    return levelInfo;
}

} // Namespace MapLoader
