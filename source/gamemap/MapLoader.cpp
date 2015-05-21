/*
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include "gamemap/MapLoader.h"

#include "creaturemood/CreatureMood.h"

#include "gamemap/GameMap.h"
#include "game/Seat.h"
#include "goals/Goal.h"
#include "goals/GoalLoading.h"

#include "entities/ChickenEntity.h"
#include "entities/CraftedTrap.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/EntityLoading.h"
#include "entities/GameEntity.h"
#include "entities/MapLight.h"
#include "entities/MissileObject.h"
#include "entities/ResearchEntity.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "entities/Weapon.h"

#include "rooms/Room.h"
#include "rooms/RoomManager.h"

#include "spells/Spell.h"

#include "traps/Trap.h"
#include "traps/TrapManager.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/ResourceManager.h"

#include "ODApplication.h"

#include <iostream>
#include <sstream>

namespace MapLoader {

bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap)
{
    std::stringstream levelFile;
    if(!Helper::readFileWithoutComments(fileName, levelFile))
        return false;

    std::string nextParam;
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

    // By default, we use the default tileSet
    gameMap.setTileSetName("");

    // Read in the seats from the level file
    while (true)
    {
        if(!levelFile.good())
            return false;
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

        param = "TileSet\t";
        if (nextParam.compare(0, param.size(), param) == 0)
        {
            std::string tileSet = nextParam.substr(param.size());
            gameMap.setTileSetName(tileSet);
            LogManager::getSingleton().logMessage("TileSet: " + tileSet);
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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Seats]")
            break;

        if (nextParam != "[Seat]")
        {
            LogManager::getSingleton().logMessage("Expected a Seat tag but got " + nextParam);
            return false;
        }

        Seat* tempSeat = new Seat(&gameMap);
        if(!tempSeat->importSeatFromStream(levelFile))
        {
            delete tempSeat;
            return false;
        }

        if(!gameMap.addSeat(tempSeat))
        {
            LogManager::getSingleton().logMessage("Couldn't add seat id="
                + Helper::toString(tempSeat->getId()));
            delete tempSeat;
        }
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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Goals]")
            break;

        std::unique_ptr<Goal> tempGoal = Goals::loadGoalFromStream(nextParam, levelFile);

        if (tempGoal.get() != nullptr)
            gameMap.addGoalForAllSeats(std::move(tempGoal));
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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Tiles]")
            break;

        // Get all the params together in order to prepare for the new parsing function
        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        Tile* tempTile = new Tile(&gameMap, true);

        Tile::loadFromLine(entire_line, tempTile);
        tempTile->computeTileVisual();

        gameMap.addTile(tempTile);
    }

    gameMap.setAllFullnessAndNeighbors();

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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Rooms]")
            break;

        if (gameMap.isServerGameMap() && (nextParam != "[Room]"))
            return false;

        if(!gameMap.isServerGameMap())
            continue;

        Room* tempRoom = RoomManager::getRoomFromStream(&gameMap, levelFile);
        OD_ASSERT_TRUE(tempRoom != nullptr);
        if(tempRoom == nullptr)
            return false;

        tempRoom->importFromStream(levelFile);

        tempRoom->addToGameMap();

        levelFile >> nextParam;
        if (nextParam != "[/Room]")
            return false;
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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Traps]")
            break;

        if (nextParam != "[Trap]")
            return false;

        Trap* tempTrap = TrapManager::getTrapFromStream(&gameMap, levelFile);
        OD_ASSERT_TRUE(tempTrap != nullptr);
        if(tempTrap == nullptr)
            return false;

        tempTrap->importFromStream(levelFile);
        tempTrap->addToGameMap();

        levelFile >> nextParam;
        if (nextParam != "[/Trap]")
            return false;
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
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Lights]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;
        //std::cout << "Entire line: " << entire_line << std::endl;

        std::stringstream ss(entire_line);
        MapLight* tempLight = MapLight::getMapLightFromStream(&gameMap, ss);
        OD_ASSERT_TRUE(tempLight != nullptr);
        if(tempLight == nullptr)
            return false;
        tempLight->importFromStream(ss);
        tempLight->setName(gameMap.nextUniqueNameMapLight());
        tempLight->addToGameMap();
    }

    levelFile >> nextParam;

    if (nextParam == "[CreaturesMood]")
    {
        while(levelFile.good())
        {
            if(!(levelFile >> nextParam))
                break;

            if (nextParam == "[/CreaturesMood]")
                break;

            if (nextParam == "[/CreatureMood]")
                continue;

            if (nextParam != "[CreatureMood]")
            {
                OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood format. Line was " + nextParam);
                return false;
            }

            if(!(levelFile >> nextParam))
                    break;
            if (nextParam != "CreatureMoodName")
            {
                OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMoodName format. Line was " + nextParam);
                return false;
            }
            std::string moodModifierName;
            levelFile >> moodModifierName;
            std::vector<CreatureMood*> moodModifiers;

            while(levelFile.good())
            {
                if(!(levelFile >> nextParam))
                    break;

                if (nextParam == "[/CreatureMood]")
                    break;

                if (nextParam != "[MoodModifier]")
                {
                    OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood MoodModifier format. nextParam=" + nextParam);
                    return false;
                }

                // Load the definition
                CreatureMood* def = CreatureMood::load(levelFile);
                if (def == nullptr)
                {
                    OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood MoodModifier definition");
                    return false;
                }
                moodModifiers.push_back(def);
            }
            gameMap.addCreatureMoodModifiers(moodModifierName, moodModifiers);
        }

        levelFile >> nextParam;
    }

    if (nextParam == "[CreatureDefinitions]")
    {
        while(levelFile.good())
        {
            levelFile >> nextParam;
            if (nextParam == "[/CreatureDefinitions]")
                break;

            if (nextParam == "[/Creature]")
                continue;

            // Seek the [Creature] tag
            if (nextParam != "[Creature]")
            {
                std::cout << "Invalid Creature start format." << std::endl;
                std::cout << "Line was " << nextParam << std::endl;
                return false;
            }

            levelFile >> nextParam;
            if (nextParam == "Name")
            {
                levelFile >> nextParam;
                CreatureDefinition* def = gameMap.getClassDescriptionForTuning(nextParam);
                if (def == nullptr)
                {
                    std::cout << "Invalid Creature definition format." << std::endl;
                    return false;
                }
                if(!CreatureDefinition::update(def, levelFile, ConfigManager::getSingleton().getCreatureDefinitions()))
                    return false;
            }
        }

        levelFile >> nextParam;
    }

    if (nextParam == "[EquipmentDefinitions]")
    {
        while(levelFile.good())
        {
            levelFile >> nextParam;
            if (nextParam == "[/EquipmentDefinitions]")
                break;

            if (nextParam == "[/Equipment]")
                continue;

            if (nextParam != "[Equipment]")
            {
                std::cout << "Invalid Weapon start format." << std::endl;
                std::cout << "Line was " << nextParam << std::endl;
                return false;
            }

            levelFile >> nextParam;
            if (nextParam == "Name")
            {
                levelFile >> nextParam;
                Weapon* def = gameMap.getWeaponForTuning(nextParam);
                if (def == nullptr)
                {
                    std::cout << "Invalid Weapon definition format." << std::endl;
                    return false;
                }
                if(!Weapon::update(def, levelFile))
                    return false;
            }
        }

        levelFile >> nextParam;
    }

    // Read in the actual creatures themselves
    if (nextParam != "[Creatures]")
    {
        std::cout << "Invalid Creatures start format." << std::endl;
        std::cout << "Line was " << nextParam << std::endl;
        return false;
    }

    uint32_t nbCreatures = 0;
    while(true)
    {
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Creatures]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;

        std::stringstream ss(entire_line);
        Creature* tempCreature = Creature::getCreatureFromStream(&gameMap, ss);
        OD_ASSERT_TRUE(tempCreature != nullptr);
        if(tempCreature == nullptr)
            return false;

        tempCreature->importFromStream(ss);
        tempCreature->addToGameMap();
        ++nbCreatures;
    }
    LogManager::getSingleton().logMessage("Loaded " + Helper::toString(nbCreatures) + " creatures in level");

    if(!readGameEntity(gameMap, "Spells", GameEntityType::spell, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid Spells section");
        return false;
    }

    if(!readGameEntity(gameMap, "CraftedTraps", GameEntityType::craftedTrap, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid CraftedTraps section");
        return false;
    }

    if(!readGameEntity(gameMap, "ResearchEntity", GameEntityType::researchEntity, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid ResearchEntity section");
        return false;
    }

    if(!readGameEntity(gameMap, "Missiles", GameEntityType::missileObject, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid Missiles section");
        return false;
    }

    if(!readGameEntity(gameMap, "TreasuryObject", GameEntityType::treasuryObject, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid TreasuryObject section");
        return false;
    }

    if(!readGameEntity(gameMap, "Chickens", GameEntityType::chickenEntity, levelFile))
    {
        LogManager::getSingleton().logMessage("Invalid Chickens section");
        return false;
    }

    return true;
}

bool readGameEntity(GameMap& gameMap, const std::string& item, GameEntityType type, std::stringstream& levelFile)
{
    std::string nextParam;
    levelFile >> nextParam;
    if (nextParam != "[" + item + "]")
        return false;

    uint32_t nbEntity = 0;
    while(true)
    {
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/" + item + "]")
            break;

        std::string entire_line = nextParam;
        std::getline(levelFile, nextParam);
        entire_line += nextParam;

        std::stringstream ss(entire_line);
        GameEntity* entity = Entities::getGameEntityFromStream(&gameMap, type, ss);
        OD_ASSERT_TRUE(entity != nullptr);
        if(entity == nullptr)
            return false;

        entity->addToGameMap();
        ++nbEntity;
    }
    LogManager::getSingleton().logMessage("Loaded " + Helper::toString(nbEntity) + " " + item + " in level");

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
    if(!gameMap.getTileSetName().empty())
        levelFile << "TileSet\t" << gameMap.getTileSetName() << std::endl;

    levelFile << "[/Info]" << std::endl;


    // Write out the seats to the file
    levelFile << "\n[Seats]\n";
    const std::vector<Seat*> seats = gameMap.getSeats();
    for (Seat* seat : seats)
    {
        // We don't save rogue seat
        if(seat->isRogueSeat())
            continue;

        levelFile << "[Seat]" << std::endl;
        seat->exportSeatToStream(levelFile);
        levelFile << "[/Seat]" << std::endl;
    }
    levelFile << "[/Seats]" << std::endl;

    // Write out the goals shared by all players to the file.
    levelFile << "\n[Goals]\n";
    levelFile << "# " << Goal::getFormat() << "\n";
    for (auto& goal : gameMap.getGoalsForAllSeats())
    {
        levelFile << *goal.get();
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
            if (!tempTile->isClaimed() && tempTile->getType() == TileType::dirt && tempTile->getFullness() >= 100.0)
                continue;

            tempTile->exportToStream(levelFile);
            levelFile << std::endl;
        }
    }
    levelFile << "[/Tiles]" << std::endl;


    std::vector<Room*> rooms = gameMap.getRooms();
    std::sort(rooms.begin(), rooms.end(), Room::sortForMapSave);

    // Write out the rooms to the file
    levelFile << "\n[Rooms]\n";
    levelFile << "# " << Room::getRoomStreamFormat() << "\n";
    for (Room* room : rooms)
    {
        // Rooms with 0 tiles are removed during upkeep. In editor mode, we don't use upkeep so there might be some rooms with
        // 0 tiles (if a room has been erased for example). For this reason, we don't save rooms with 0 tiles
        if((gameMap.isInEditorMode()) && (room->numCoveredTiles() <= 0))
            continue;

        levelFile << "[Room]" << std::endl;
        room->exportHeadersToStream(levelFile);
        room->exportToStream(levelFile);
        levelFile << "[/Room]" << std::endl;
    }
    levelFile << "[/Rooms]" << std::endl;

    // Write out the traps to the file
    levelFile << "\n[Traps]\n";
    levelFile << "# " << Trap::getTrapStreamFormat() << "\n";
    for (Trap* trap : gameMap.getTraps())
    {
        // In editor mode, we don't use upkeep so there might be some traps with
        // 0 tiles (if a trap has been erased for example). For this reason, we don't save traps with 0 tiles
        if(gameMap.isInEditorMode() && trap->numCoveredTiles() <= 0)
            continue;

        levelFile << "[Trap]" << std::endl;
        trap->exportHeadersToStream(levelFile);
        trap->exportToStream(levelFile);
        levelFile << "[/Trap]" << std::endl;
    }
    levelFile << "[/Traps]" << std::endl;

    // Write out the lights to the file.
    levelFile << "\n[Lights]\n";
    levelFile << "# " << MapLight::getMapLightStreamFormat() << "\n";
    for (MapLight* mapLight : gameMap.getMapLights())
    {
        mapLight->exportHeadersToStream(levelFile);
        mapLight->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/Lights]" << std::endl;

    levelFile << std::endl << "[CreatureDefinitions]" << std::endl;
    gameMap.saveLevelClassDescriptions(levelFile);
    levelFile << "[/CreatureDefinitions]" << std::endl;

    levelFile << std::endl << "[EquipmentDefinitions]" << std::endl;
    gameMap.saveLevelEquipments(levelFile);
    levelFile << "[/EquipmentDefinitions]" << std::endl;

    // Write out the individual creatures to the file
    levelFile << "\n[Creatures]\n";
    levelFile << "# " << Creature::getCreatureStreamFormat() << "\n";
    for (Creature* creature : gameMap.getCreatures())
    {
        creature->exportHeadersToStream(levelFile);
        creature->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/Creatures]" << std::endl;

    // Write out the RenderedMovableEntities that need to
    const std::vector<RenderedMovableEntity*>& rendereds = gameMap.getRenderedMovableEntities();
    levelFile << "\n[Spells]\n";
    levelFile << "# " << Spell::getSpellStreamFormat() << "\n";
    for (Spell* spell : gameMap.getSpells())
    {
        spell->exportHeadersToStream(levelFile);
        spell->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/Spells]" << std::endl;

    levelFile << "\n[CraftedTraps]\n";
    levelFile << "# " << CraftedTrap::getCraftedTrapStreamFormat() << "\n";
    for (RenderedMovableEntity* rendered : rendereds)
    {
        if(rendered->getObjectType() != GameEntityType::craftedTrap)
            continue;

        rendered->exportHeadersToStream(levelFile);
        rendered->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/CraftedTraps]" << std::endl;

    levelFile << "\n[ResearchEntity]\n";
    levelFile << "# " << ResearchEntity::getResearchEntityStreamFormat() << "\n";
    for (RenderedMovableEntity* rendered : rendereds)
    {
        if(rendered->getObjectType() != GameEntityType::researchEntity)
            continue;

        rendered->exportHeadersToStream(levelFile);
        rendered->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/ResearchEntity]" << std::endl;

    levelFile << "\n[Missiles]\n";
    levelFile << "# " << MissileObject::getMissileObjectStreamFormat() << "\n";
    for (RenderedMovableEntity* rendered : rendereds)
    {
        if(rendered->getObjectType() != GameEntityType::missileObject)
            continue;

        rendered->exportHeadersToStream(levelFile);
        rendered->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/Missiles]" << std::endl;

    levelFile << "\n[TreasuryObject]\n";
    levelFile << "# " << TreasuryObject::getTreasuryObjectStreamFormat() << "\n";
    for (RenderedMovableEntity* rendered : rendereds)
    {
        if(rendered->getObjectType() != GameEntityType::treasuryObject)
            continue;

        rendered->exportHeadersToStream(levelFile);
        rendered->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/TreasuryObject]" << std::endl;

    levelFile << "\n[Chickens]\n";
    levelFile << "# " << ChickenEntity::getChickenEntityStreamFormat() << "\n";
    for (RenderedMovableEntity* rendered : rendereds)
    {
        if(rendered->getObjectType() != GameEntityType::chickenEntity)
            continue;

        rendered->exportHeadersToStream(levelFile);
        rendered->exportToStream(levelFile);
        levelFile << std::endl;
    }
    levelFile << "[/Chickens]" << std::endl;

    levelFile.close();
}

bool getMapInfo(const std::string& fileName, LevelInfo& levelInfo)
{
    // Prepare an invalid level reference
    std::stringstream levelFile;
    if(!Helper::readFileWithoutComments(fileName, levelFile))
        return false;

    std::string nextParam;
    // Read in the version number from the level file
    levelFile >> nextParam;
    if (nextParam.compare(ODApplication::VERSIONSTRING) != 0)
        return false;

    levelFile >> nextParam;
    if (nextParam != "[Info]")
        return false;

    std::stringstream mapInfo;

    // Read in the seats from the level file
    while (true)
    {
        if(!levelFile.good())
            return false;
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
        return true;
    }

    // Read in the seats from the level file
    int playerSeatNumber = 0;
    int AISeatNumber = 0;
    int seatConfigurable = 0;
    while (true)
    {
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Seats]")
            break;

        if (nextParam != "[Seat]")
            return false;

        while(true)
        {
            std::string line;
            std::getline(levelFile, line);
            std::stringstream ss(line);
            ss >> nextParam;
            if(nextParam == "[/Seat]")
                break;

            if(nextParam != "player")
                continue;

            // We get the player type
            ss >> nextParam;
            if (nextParam == Seat::PLAYER_TYPE_HUMAN)
                ++playerSeatNumber;
            else if (nextParam == Seat::PLAYER_TYPE_CHOICE)
                ++seatConfigurable;
            else if (nextParam == Seat::PLAYER_TYPE_AI)
                ++AISeatNumber;
        }
    }

    if (playerSeatNumber > 0 || AISeatNumber > 0)
    {
        std::string str;

        if (playerSeatNumber > 0)
            str += "Player slot(s): " + Helper::toString(playerSeatNumber);
        if (AISeatNumber > 0)
        {
            if(!str.empty())
                str += " / ";

            str += "AI: " + Helper::toString(AISeatNumber);
        }
        if (seatConfigurable > 0)
        {
            if(!str.empty())
                str += " / ";

            str += "Configurable: " + Helper::toString(seatConfigurable);
        }

        mapInfo << str << std::endl << std::endl;
    }

    // Read in the goals that are shared by all players, the first player to complete all these goals is the winner.
    levelFile >> nextParam;
    if (nextParam != "[Goals]")
    {
        levelInfo.mLevelDescription = mapInfo.str();
        return true;
    }

    while(true)
    {
        if(!levelFile.good())
            return false;

        levelFile >> nextParam;
        if (nextParam == "[/Goals]")
            break;
    }

    levelFile >> nextParam;
    if (nextParam != "[Tiles]")
    {
        levelInfo.mLevelDescription = mapInfo.str();
        return true;
    }

    // Load the map size on next two lines
    int mapSizeX;
    int mapSizeY;
    levelFile >> mapSizeX;
    levelFile >> mapSizeY;

    mapInfo << "Size: " << mapSizeX << "x" << mapSizeY << std::endl << std::endl;

    levelInfo.mLevelDescription = mapInfo.str();
    return true;
}

} // Namespace MapLoader
