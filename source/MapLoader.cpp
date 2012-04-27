/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sstream>

#include "GameMap.h"
#include "ODApplication.h"
#include "Goal.h"
#include "Creature.h"
#include "CreatureDefinition.h"
#include "Trap.h"
#include "Seat.h"
#include "MapLight.h"
#include "LogManager.h"

#include "MapLoader.h"

namespace MapLoader {

bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap_b)
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
    gameMap_b.allocateMapMemory(GameMap::mapSizeX, GameMap::mapSizeY ); 
    gameMap_b.clearAll();

    // Read in the name of the next level to load after this one is complete.
    levelFile >> gameMap_b.nextLevel;

    int objectsToLoad = 0;

    // Read in the seats from the level file
    Seat* tempSeat;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempSeat = new Seat;
        levelFile >> tempSeat;

        gameMap_b.addEmptySeat(tempSeat);
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> objectsToLoad;
    Goal* tempGoal;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempGoal = Goal::instantiateFromStream(levelFile, gameMap_b);

        if (tempGoal != NULL)
            gameMap_b.addGoalForAllSeats(tempGoal);
    }

    // Read in the map tiles from disk
    Tile* tempTile;
    levelFile >> objectsToLoad;

    gameMap_b.disableFloodFill();
    for (int i = 0; i < objectsToLoad; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        tempTile = new Tile;
        levelFile >> tempTile;


	tempTile->x+=GameMap::mapSizeX/2;
	tempTile->y+=GameMap::mapSizeY/2;
        tempTile->setGameMap(&gameMap_b);
        gameMap_b.addTile(tempTile);
	delete tempTile;
    }
    gameMap_b.enableFloodFill();

    // Loop over all the tiles and force them to examine their
    // neighbors.  This allows them to switch to a mesh with fewer
    // polygons if some are hidden by the neighbors.

    // for(int ii=0 ; ii < mapSizeX; ii++ ){
    //   for(int jj=0 ; jj < mapSizeY; jj++ ){

    for(int ii=0 ; ii < gameMap_b.mapSizeX; ii++ ){
      for(int jj=0 ; jj < gameMap_b.mapSizeY; jj++ ){

	gameMap_b.getTile( ii, jj)->setFullness(gameMap_b.getTile( ii, jj)->getFullness());
      }
    }

    // for(TileMap_t::iterator itr = gameMap_b.firstTile(), last = gameMap_b.lastTile();
    //         itr != last; ++itr)
    // {
    //     itr->second->setFullness(itr->second->getFullness());
    // }

    // Read in the rooms
    Room* tempRoom;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempRoom = Room::createRoomFromStream(levelFile, &gameMap_b);

        gameMap_b.addRoom(tempRoom);
    }

    // Read in the traps
    Trap* tempTrap;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempTrap = Trap::createTrapFromStream(levelFile, &gameMap_b);
        tempTrap->createMesh();

        gameMap_b.addTrap(tempTrap);
    }

    // Read in the lights
    MapLight* tempLight;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {
        tempLight = new MapLight;
        levelFile >> tempLight;

        gameMap_b.addMapLight(tempLight);
    }

    // Read in the creature class descriptions
    CreatureDefinition* tempClass;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i )
    {
        tempClass = new CreatureDefinition;
        levelFile >> tempClass;

        gameMap_b.addClassDescription(tempClass);
    }

    // Read in the actual creatures themselves
    Creature* tempCreature;
    Ogre::Vector3 tempVector;
    levelFile >> objectsToLoad;
    for (int i = 0; i < objectsToLoad; ++i)
    {

        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and writeGameMapToFile.
        //Changes to this code should be reflected in that code as well
        tempCreature = new Creature(&gameMap_b);
        levelFile >> tempCreature;
	

	tempVector = tempCreature->getPosition();
	tempVector.x+=GameMap::mapSizeX/2;
	tempVector.y+=GameMap::mapSizeY/2;
	tempCreature->setPosition(tempVector);
        gameMap_b.addCreature(tempCreature);


    }

    return true;
}

void writeGameMapToFile(const std::string& fileName, GameMap& gameMap_b)
{
    std::ofstream levelFile(fileName.c_str(), std::ifstream::out);
    Tile *tempTile;

    // Write the identifier string and the version number
    levelFile << ODApplication::VERSIONSTRING
            << "  # The version of OpenDungeons which created this file (for compatability reasons).\n";

    // write out the name of the next level to load after this one is complete.
    levelFile << gameMap_b.nextLevel
            << " # The level to load after this level is complete.\n";

    // Write out the seats to the file
    levelFile << "\n# Seats\n" << gameMap_b.numEmptySeats()
            + gameMap_b.numFilledSeats() << "  # The number of seats to load.\n";
    levelFile << "# " << Seat::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap_b.numEmptySeats(); ++i)
    {
        levelFile << gameMap_b.getEmptySeat(i);
    }

    for (unsigned int i = 0; i < gameMap_b.numFilledSeats(); ++i)
    {
        levelFile << gameMap_b.getFilledSeat(i);
    }

    // Write out the goals shared by all players to the file.
    levelFile << "\n# Goals\n" << gameMap_b.numGoalsForAllSeats()
            << "  # The number of goals to load.\n";
    levelFile << "# " << Goal::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap_b.numGoalsForAllSeats(); i < num; ++i)
    {
        levelFile << gameMap_b.getGoalForAllSeats(i);
    }

    // Write out the tiles to the file
    levelFile << "\n# Tiles\n" << gameMap_b.numTiles()
            << "  # The number of tiles to load.\n";
    levelFile << "# " << Tile::getFormat() << "\n";



    for(int ii=0 ; ii < gameMap_b.mapSizeX; ii++ ){
      for(int jj=0 ; jj < gameMap_b.mapSizeY; jj++ ){

	tempTile = gameMap_b.getTile(ii,jj);
        levelFile << tempTile->x << "\t" << tempTile->y << "\t";
        levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

        levelFile << std::endl;


      }
    }
    // TileMap_t::iterator itr = gameMap_b.firstTile();
    // while (itr != gameMap_b.lastTile())
    // {
    //     //NOTE: This code is duplicated in the client side method
    //     //"addclass" defined in src/Client.cpp and readGameMapFromFile.
    //     //Changes to this code should be reflected in that code as well
    //     tempTile = itr->second;
    //     levelFile << tempTile->x << "\t" << tempTile->y << "\t";
    //     levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

    //     levelFile << std::endl;

    //     ++itr;
    // }

    // Write out the rooms to the file
    levelFile << "\n# Rooms\n" << gameMap_b.numRooms()
            << "  # The number of rooms to load.\n";
    levelFile << "# " << Room::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap_b.numRooms(); i < num; ++i)
    {
        levelFile << gameMap_b.getRoom(i) << std::endl;
    }

    // Write out the traps to the file
    levelFile << "\n# Traps\n" << gameMap_b.numTraps()
            << "  # The number of traps to load.\n";
    levelFile << "# " << Trap::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap_b.numTraps(); ++i)
    {
        levelFile << gameMap_b.getTrap(i) << std::endl;
    }

    // Write out the lights to the file.
    levelFile << "\n# Lights\n" << gameMap_b.numMapLights()
            << "  # The number of lights to load.\n";
    levelFile << "# " << MapLight::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap_b.numMapLights(); i < num; ++i)
    {
        levelFile << gameMap_b.getMapLight(i) << std::endl;
    }

    // Write out the creature descriptions to the file
    levelFile << "\n# Creature classes\n" << gameMap_b.numClassDescriptions()
            << "  # The number of creature classes to load.\n";
    levelFile << CreatureDefinition::getFormat() << "\n";
    for (unsigned int i = 0; i < gameMap_b.numClassDescriptions(); ++i)
    {
        levelFile << gameMap_b.getClassDescription(i) << "\n";
    }

    // Write out the individual creatures to the file
    levelFile << "\n# Creatures\n" << gameMap_b.numCreatures()
            << "  # The number of creatures to load.\n";
    levelFile << "# " << Creature::getFormat() << "\n";
    for (unsigned int i = 0, num = gameMap_b.numCreatures(); i < num; ++i)
    {
        //NOTE: This code is duplicated in the client side method
        //"addclass" defined in src/Client.cpp and readGameMapFromFile.
        //Changes to this code should be reflected in that code as well
        levelFile << gameMap_b.getCreature(i) << std::endl;
    }

    levelFile << std::endl;

    levelFile.close();
}

Ogre::SharedPtr<CreatureDefinition> loadCreatureDefinition(const std::string& fileName)
{
    //STUB
    Ogre::SharedPtr<CreatureDefinition> p;
    return p;
}

}
