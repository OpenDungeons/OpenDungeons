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

#include "utils/ConfigManager.h"

#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "entities/Weapon.h"

#include "creaturemood/CreatureMood.h"

#include "game/Research.h"

#include "gamemap/TileSet.h"

#include "spawnconditions/SpawnCondition.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <boost/dynamic_bitset.hpp>
#include <OgreRoot.h>

enum class USER_CONFIG_CATEGORY {
    CATEGORY_NONE  = 0,
    CATEGORY_AUDIO = 1,
    CATEGORY_VIDEO = 2,
    CATEGORY_INPUT = 3,
    CATEGORY_GAME  = 4
};

const std::vector<std::string> EMPTY_SPAWNPOOL;
const std::string EMPTY_STRING;
const std::string ConfigManager::DEFAULT_TILESET_NAME = "Default";

const std::string ConfigManager::DefaultWorkerCreatureDefinition = "DefaultWorker";

template<> ConfigManager* Ogre::Singleton<ConfigManager>::msSingleton = nullptr;

ConfigManager::ConfigManager(const std::string& configPath, const std::string& userConfigPath) :
    mNetworkPort(0),
    mClientConnectionTimeout(5000),
    mBaseSpawnPoint(10),
    mCreatureDeathCounter(10),
    mMaxCreaturesPerSeatAbsolute(30),
    mMaxCreaturesPerSeatDefault(10),
    mCreatureBaseMood(1000),
    mCreatureMoodHappy(1200),
    mCreatureMoodUpset(0),
    mCreatureMoodAngry(-1000),
    mCreatureMoodFurious(-2000),
    mSlapDamagePercent(15),
    mTimePayDay(300),
    mNbTurnsFuriousMax(120),
    mMaxManaPerSeat(250000.0),
    mClaimingWallPenalty(0.8),
    mCreatureDefinitionDefaultWorker(nullptr)
{
    mCreatureDefinitionDefaultWorker = new CreatureDefinition(DefaultWorkerCreatureDefinition,
        CreatureDefinition::CreatureJob::Worker,
        "Kobold.mesh",
        "Bed", 1, 2, Ogre::Vector3(0.04, 0.04, 0.04));
    if(!loadGlobalConfig(configPath))
    {
        OD_LOG_ERR("Couldn't read loadCreatureDefinitions");
        exit(1);
    }
    std::string fileName = configPath + mFilenameCreatureDefinition;
    if(!loadCreatureDefinitions(fileName))
    {
        OD_LOG_ERR("Couldn't read loadCreatureDefinitions");
        exit(1);
    }
    fileName = configPath + mFilenameEquipmentDefinition;
    if(!loadEquipements(fileName))
    {
        OD_LOG_ERR("Couldn't read loadEquipements");
        exit(1);
    }
    fileName = configPath + mFilenameSpawnConditions;
    if(!loadSpawnConditions(fileName))
    {
        OD_LOG_ERR("Couldn't read loadSpawnConditions");
        exit(1);
    }
    fileName = configPath + mFilenameFactions;
    if(!loadFactions(fileName))
    {
        OD_LOG_ERR("Couldn't read loadFactions");
        exit(1);
    }
    fileName = configPath + mFilenameRooms;
    if(!loadRooms(fileName))
    {
        OD_LOG_ERR("Couldn't read loadRooms");
        exit(1);
    }
    fileName = configPath + mFilenameTraps;
    if(!loadTraps(fileName))
    {
        OD_LOG_ERR("Couldn't read loadTraps");
        exit(1);
    }
    fileName = configPath + mFilenameSpells;
    if(!loadSpellConfig(fileName))
    {
        OD_LOG_ERR("Couldn't read loadSpellConfig");
        exit(1);
    }
    fileName = configPath + mFilenameCreaturesMood;
    if(!loadCreaturesMood(fileName))
    {
        OD_LOG_ERR("Couldn't read loadCreaturesMood");
        exit(1);
    }
    fileName = configPath + mFilenameResearches;
    if(!loadResearches(fileName))
    {
        OD_LOG_ERR("Couldn't read loadResearches");
        exit(1);
    }
    fileName = configPath + mFilenameTilesets;
    if(!loadTilesets(fileName))
    {
        OD_LOG_ERR("Couldn't read loadTilesets");
        exit(1);
    }

    if (!userConfigPath.empty())
        loadUserConfig(userConfigPath);
}

ConfigManager::~ConfigManager()
{
    for(auto pair : mCreatureDefs)
    {
        delete pair.second;
    }

    if(mCreatureDefinitionDefaultWorker != nullptr)
        delete mCreatureDefinitionDefaultWorker;

    for(const Weapon* def : mWeapons)
    {
        delete def;
    }
    mWeapons.clear();

    for(std::pair<const std::string, std::vector<const CreatureMood*>> p : mCreatureMoodModifiers)
    {
        for(const CreatureMood* creatureMood : p.second)
        {
            delete creatureMood;
        }
    }
    mCreatureMoodModifiers.clear();

    for(std::pair<const CreatureDefinition*, std::vector<const SpawnCondition*>> p : mCreatureSpawnConditions)
    {
        for(const SpawnCondition* spawnCondition : p.second)
        {
            delete spawnCondition;
        }
    }
    mCreatureSpawnConditions.clear();

    for(const Research* research : mResearches)
    {
        delete research;
    }
    mResearches.clear();

    for(std::pair<const std::string, const TileSet*> p : mTileSets)
    {
        delete p.second;
    }
    mTileSets.clear();
}

bool ConfigManager::loadGlobalConfig(const std::string& configPath)
{
    std::stringstream configFile;
    std::string fileName = configPath + "global.cfg";
    if(!Helper::readFileWithoutComments(fileName, configFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    uint32_t paramsOk = 0;
    while(configFile.good())
    {
        if(!(configFile >> nextParam))
            break;

        if(nextParam == "[SeatColors]")
        {
            if(!loadGlobalConfigSeatColors(configFile))
                break;

            paramsOk |= 1;
            continue;
        }

        if(nextParam == "[ConfigFiles]")
        {
            if(!loadGlobalConfigDefinitionFiles(configFile))
                break;

            paramsOk |= 2;
            continue;
        }

        if(nextParam == "[GameConfig]")
        {
            if(!loadGlobalGameConfig(configFile))
                break;

            paramsOk |= 4;
            continue;
        }
    }

    if(paramsOk != 0x07)
    {
        OD_LOG_ERR("Not enough parameters for config file paramsOk=" + Helper::toString(paramsOk));
        return false;
    }

    return true;
}

bool ConfigManager::loadGlobalConfigDefinitionFiles(std::stringstream& configFile)
{
    std::string nextParam;
    uint32_t filesOk = 0;
    while(configFile.good())
    {
        if(!(configFile >> nextParam))
            break;

        if(nextParam == "[/ConfigFiles]")
            break;

        if(nextParam != "[ConfigFile]")
        {
            OD_LOG_ERR("Wrong parameter read nextParam=" + nextParam);
            return false;
        }

        uint32_t paramsOk = 0;
        std::string type;
        std::string fileName;
        while(configFile.good())
        {
            if(!(configFile >> nextParam))
                break;

            if(nextParam == "[/ConfigFile]")
            {
                break;
            }

            if(nextParam == "Type")
            {
                configFile >> type;
                paramsOk |= 0x01;
                continue;
            }

            if(nextParam == "Filename")
            {
                configFile >> fileName;
                paramsOk |= 0x02;
                continue;
            }
        }

        if(paramsOk != 0x03)
        {
            OD_LOG_ERR("Missing parameters paramsOk=" + Helper::toString(paramsOk));
            return false;
        }

        if(type == "Creatures")
        {
            mFilenameCreatureDefinition = fileName;
            filesOk |= 1;
        }
        else if(type == "Equipments")
        {
            mFilenameEquipmentDefinition = fileName;
            filesOk |= 2;
        }
        else if(type == "SpawnConditions")
        {
            mFilenameSpawnConditions = fileName;
            filesOk |= 4;
        }
        else if(type == "Factions")
        {
            mFilenameFactions = fileName;
            filesOk |= 8;
        }
        else if(type == "Rooms")
        {
            mFilenameRooms = fileName;
            filesOk |= 0x10;
        }
        else if(type == "Traps")
        {
            mFilenameTraps = fileName;
            filesOk |= 0x20;
        }
        else if(type == "Spells")
        {
            mFilenameSpells = fileName;
            filesOk |= 0x40;
        }
        else if(type == "CreaturesMood")
        {
            mFilenameCreaturesMood = fileName;
            filesOk |= 0x80;
        }
        else if(type == "Researches")
        {
            mFilenameResearches = fileName;
            filesOk |= 0x100;
        }
        else if(type == "Tilesets")
        {
            mFilenameTilesets = fileName;
            filesOk |= 0x200;
        }
    }

    if(filesOk != 0x3FF)
    {
        OD_LOG_ERR("Missing parameter file filesOk=" + Helper::toString(filesOk));
        return false;
    }

    return true;
}

bool ConfigManager::loadGlobalConfigSeatColors(std::stringstream& configFile)
{
    std::string nextParam;
    while(configFile.good())
    {
        if(!(configFile >> nextParam))
            break;

        if(nextParam == "[/SeatColors]")
            break;

        if(nextParam != "[SeatColor]")
        {
            OD_LOG_ERR("Wrong parameter read nextParam=" + nextParam);
            return false;
        }

        uint32_t paramsOk = 0;
        std::string id;
        Ogre::ColourValue colourValue;
        while(configFile.good())
        {
            if(!(configFile >> nextParam))
                break;

            if(nextParam == "[/SeatColor]")
            {
                break;
            }

            if(nextParam == "ID")
            {
                configFile >> id;
                paramsOk |= 0x01;
                continue;
            }

            if(nextParam == "ColorR")
            {
                configFile >> nextParam;
                double v = Helper::toDouble(nextParam);
                if(v < 0.0 || v > 1.0)
                {
                    OD_LOG_ERR("Wrong parameter read nextParam=" + nextParam);
                    return false;
                }
                colourValue.r = v;
                paramsOk |= 0x02;
                continue;
            }

            if(nextParam == "ColorG")
            {
                configFile >> nextParam;
                double v = Helper::toDouble(nextParam);
                if(v < 0.0 || v > 1.0)
                {
                    OD_LOG_ERR("Wrong parameter read nextParam=" + nextParam);
                    return false;
                }
                colourValue.g = v;
                paramsOk |= 0x04;
                continue;
            }

            if(nextParam == "ColorB")
            {
                configFile >> nextParam;
                double v = Helper::toDouble(nextParam);
                if(v < 0.0 || v > 1.0)
                {
                    OD_LOG_ERR("Wrong parameter read nextParam=" + nextParam);
                    return false;
                }
                colourValue.b = v;
                paramsOk |= 0x08;
                continue;
            }
        }

        if(paramsOk != 0x0F)
        {
            OD_LOG_ERR("Missing parameters paramsOk=" + Helper::toString(paramsOk));
            return false;
        }

        mSeatColors[id] = colourValue;
    }

    return true;
}

bool ConfigManager::loadGlobalGameConfig(std::stringstream& configFile)
{
    std::string nextParam;
    uint32_t paramsOk = 0;
    while(configFile.good())
    {
        if(!(configFile >> nextParam))
            break;

        if(nextParam == "[/GameConfig]")
            break;

        if(nextParam == "NetworkPort")
        {
            configFile >> nextParam;
            mNetworkPort = Helper::toUInt32(nextParam);
            paramsOk |= 1;
        }

        if(nextParam == "ClientConnectionTimeout")
        {
            configFile >> nextParam;
            mClientConnectionTimeout = Helper::toUInt32(nextParam);
            // Not mandatory
        }

        if(nextParam == "CreatureDeathCounter")
        {
            configFile >> nextParam;
            mCreatureDeathCounter = Helper::toUInt32(nextParam);
            // Not mandatory
        }

        if(nextParam == "MaxCreaturesPerSeatAbsolute")
        {
            configFile >> nextParam;
            mMaxCreaturesPerSeatAbsolute = Helper::toUInt32(nextParam);
            // Not mandatory
        }

        if(nextParam == "MaxCreaturesPerSeatDefault")
        {
            configFile >> nextParam;
            mMaxCreaturesPerSeatDefault = Helper::toUInt32(nextParam);
            // Not mandatory
        }

        if(nextParam == "SlapDamagePercent")
        {
            configFile >> nextParam;
            mSlapDamagePercent = Helper::toDouble(nextParam);
            // Not mandatory
        }

        if(nextParam == "TimePayDay")
        {
            configFile >> nextParam;
            mTimePayDay = Helper::toInt(nextParam);
            // Not mandatory
        }

        if(nextParam == "NbTurnsFuriousMax")
        {
            configFile >> nextParam;
            mNbTurnsFuriousMax = Helper::toInt(nextParam);
            // Not mandatory
        }

        if(nextParam == "MaxManaPerSeat")
        {
            configFile >> nextParam;
            mMaxManaPerSeat = Helper::toDouble(nextParam);
            // Not mandatory
        }

        if(nextParam == "ClaimingWallPenalty")
        {
            configFile >> nextParam;
            mClaimingWallPenalty = Helper::toDouble(nextParam);
            // Not mandatory
        }
    }

    if(paramsOk != 0x01)
    {
        OD_LOG_ERR("Missing parameters paramsOk=" + Helper::toString(paramsOk));
        return false;
    }

    return true;
}

bool ConfigManager::loadCreatureDefinitions(const std::string& fileName)
{
    OD_LOG_INF("Load creature definition file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[CreatureDefinitions]")
    {
        OD_LOG_ERR("Invalid Creature classes start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/CreatureDefinitions]")
            break;

        if (nextParam == "[/Creature]")
            continue;

        // Seek the [Creature] tag
        if (nextParam != "[Creature]")
        {
            OD_LOG_ERR("Invalid Creature classes start format. Line was " + nextParam);
            return false;
        }

        // Load the creature definition until a [/Creature] tag is found
        CreatureDefinition* creatureDef = CreatureDefinition::load(defFile, mCreatureDefs);
        if (creatureDef == nullptr)
        {
            OD_LOG_ERR("Invalid Creature classes start format");
            return false;
        }
        mCreatureDefs.emplace(creatureDef->getClassName(), creatureDef);
    }

    return true;
}

bool ConfigManager::loadEquipements(const std::string& fileName)
{
    OD_LOG_INF("Load weapon definition file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[EquipmentDefinitions]")
    {
        OD_LOG_ERR("Invalid weapon start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/EquipmentDefinitions]")
            break;

        if (nextParam == "[/Equipment]")
            continue;

        if (nextParam != "[Equipment]")
        {
            OD_LOG_ERR("Invalid Weapon definition format. Line was " + nextParam);
            return false;
        }

        // Load the definition
        Weapon* weapon = Weapon::load(defFile);
        if (weapon == nullptr)
        {
            OD_LOG_ERR("Invalid Weapon definition format");
            return false;
        }
        mWeapons.push_back(weapon);
    }

    return true;
}

bool ConfigManager::loadSpawnConditions(const std::string& fileName)
{
    OD_LOG_INF("Load creature spawn conditions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[SpawnConditions]")
    {
        OD_LOG_ERR("Invalid creature spawn condition start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/SpawnConditions]")
            break;

        if (nextParam == "[/SpawnCondition]")
            continue;

        if (nextParam == "BaseSpawnPoint")
        {
            defFile >> nextParam;
            mBaseSpawnPoint = Helper::toUInt32(nextParam);
            continue;
        }

        if (nextParam != "[SpawnCondition]")
        {
            OD_LOG_ERR("Invalid creature spawn condition format. Line was " + nextParam);
            return false;
        }

        if(!(defFile >> nextParam))
                break;
        if (nextParam != "CreatureClass")
        {
            OD_LOG_ERR("Invalid creature spawn condition format. Line was " + nextParam);
            return false;
        }
        defFile >> nextParam;
        const CreatureDefinition* creatureDefinition = getCreatureDefinition(nextParam);
        if(creatureDefinition == nullptr)
        {
            OD_LOG_ERR("nextParam=" + nextParam);
            return false;
        }

        while(defFile.good())
        {
            if(!(defFile >> nextParam))
                break;

            if (nextParam == "[/SpawnCondition]")
                break;

            if (nextParam != "[Condition]")
            {
                OD_LOG_ERR("Invalid creature spawn condition format. nextParam=" + nextParam);
                return false;
            }

            // Load the definition
            SpawnCondition* def = SpawnCondition::load(defFile);
            if (def == nullptr)
            {
                OD_LOG_ERR("Invalid creature spawn condition format");
                return false;
            }
            mCreatureSpawnConditions[creatureDefinition].push_back(def);
        }
    }

    return true;
}

bool ConfigManager::loadFactions(const std::string& fileName)
{
    OD_LOG_INF("Load factions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Factions]")
    {
        OD_LOG_ERR("Invalid factions start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Factions]")
            break;

        if (nextParam != "[Faction]")
        {
            OD_LOG_ERR("Invalid faction. Line was " + nextParam);
            return false;
        }

        std::string factionName;
        std::string workerClass;
        while(defFile.good())
        {
            if(!(defFile >> nextParam))
                break;

            if (nextParam == "[/Faction]")
                break;

            if (nextParam == "[/Factions]")
                break;

            if (nextParam == "Name")
            {
                defFile >> factionName;
                continue;
            }
            if(factionName.empty())
            {
                OD_LOG_ERR("Empty or missing faction name is not allowed");
                return false;
            }

            if (nextParam == "WorkerClass")
            {
                defFile >> workerClass;
                continue;
            }
            if(workerClass.empty())
            {
                OD_LOG_ERR("Empty or missing WorkerClass name is not allowed");
                return false;
            }

            if (nextParam != "[SpawnPool]")
            {
                OD_LOG_ERR("Invalid faction. Line was " + nextParam);
                return false;
            }

            mFactions.push_back(factionName);
            mFactionDefaultWorkerClass[factionName] = workerClass;

            // The first faction in the config file is also used for the rogue seat
            if(mDefaultWorkerRogue.empty())
                mDefaultWorkerRogue = workerClass;

            while(defFile.good())
            {
                if(!(defFile >> nextParam))
                    break;

                if (nextParam == "[/SpawnPool]")
                    break;

                if (nextParam == "[/Faction]")
                    break;

                if (nextParam == "[/Factions]")
                    break;

                // We check if the creature definition exists
                const CreatureDefinition* creatureDefinition = getCreatureDefinition(nextParam);
                if(creatureDefinition == nullptr)
                {
                    OD_LOG_ERR("factionName=" + factionName + ", class=" + nextParam);
                    continue;
                }

                mFactionSpawnPool[factionName].push_back(nextParam);
            }
        }
    }

    return true;
}

bool ConfigManager::loadRooms(const std::string& fileName)
{
    OD_LOG_INF("Load Rooms file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Rooms]")
    {
        OD_LOG_ERR("Invalid factions start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Rooms]")
            break;

        defFile >> mRoomsConfig[nextParam];
    }

    return true;
}

bool ConfigManager::loadTraps(const std::string& fileName)
{
    OD_LOG_INF("Load traps file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Traps]")
    {
        OD_LOG_ERR("Invalid Traps start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Traps]")
            break;

        defFile >> mTrapsConfig[nextParam];
    }

    return true;
}

bool ConfigManager::loadSpellConfig(const std::string& fileName)
{
    OD_LOG_INF("Load Spell config file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Spells]")
    {
        OD_LOG_ERR("Invalid Spells start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Spells]")
            break;

        defFile >> mSpellConfig[nextParam];
    }

    return true;
}

bool ConfigManager::loadCreaturesMood(const std::string& fileName)
{
    OD_LOG_INF("Load creature spawn conditions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[CreaturesMood]")
    {
        OD_LOG_ERR("Invalid creature spawn condition start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/CreaturesMood]")
            break;

        if (nextParam == "[/CreatureMood]")
            continue;

        if (nextParam == "BaseMood")
        {
            defFile >> nextParam;
            mCreatureBaseMood = Helper::toInt(nextParam);
            continue;
        }

        if (nextParam == "MoodHappy")
        {
            defFile >> nextParam;
            mCreatureMoodHappy = Helper::toInt(nextParam);
            continue;
        }

        if (nextParam == "MoodUpset")
        {
            defFile >> nextParam;
            mCreatureMoodUpset = Helper::toInt(nextParam);
            continue;
        }

        if (nextParam == "MoodAngry")
        {
            defFile >> nextParam;
            mCreatureMoodAngry = Helper::toInt(nextParam);
            continue;
        }

        if (nextParam == "MoodFurious")
        {
            defFile >> nextParam;
            mCreatureMoodFurious = Helper::toInt(nextParam);
            continue;
        }

        if (nextParam != "[CreatureMood]")
        {
            OD_LOG_ERR("Invalid CreatureMood format. Line was " + nextParam);
            return false;
        }

        if(!(defFile >> nextParam))
                break;
        if (nextParam != "CreatureMoodName")
        {
            OD_LOG_ERR("Invalid CreatureMoodName format. Line was " + nextParam);
            return false;
        }
        std::string moodModifierName;
        defFile >> moodModifierName;
        std::vector<const CreatureMood*>& moodModifiers = mCreatureMoodModifiers[moodModifierName];

        while(defFile.good())
        {
            if(!(defFile >> nextParam))
                break;

            if (nextParam == "[/CreatureMood]")
                break;

            if (nextParam != "[MoodModifier]")
            {
                OD_LOG_ERR("Invalid CreatureMood MoodModifier format. nextParam=" + nextParam);
                return false;
            }

            // Load the definition
            CreatureMood* def = CreatureMood::load(defFile);
            if (def == nullptr)
            {
                OD_LOG_ERR("Invalid CreatureMood MoodModifier definition");
                return false;
            }
            moodModifiers.push_back(def);
        }
    }

    return true;
}

bool ConfigManager::loadResearches(const std::string& fileName)
{
    int32_t pointsRoomCrypt = 0;
    int32_t pointsRoomWorkshop = 0;
    int32_t pointsRoomTrainingHall = 0;
    int32_t pointsTrapCannon = 0;
    int32_t pointsTrapWoodenDoor = 0;
    int32_t pointsTrapBoulder = 0;
    int32_t pointsTrapSpike = 0;
    int32_t pointsSpellCallToWar = 0;
    int32_t pointsSpellCreatureHeal = 0;
    int32_t pointsSpellCreatureExplosion = 0;

    OD_LOG_INF("Load Researches file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Researches]")
    {
        OD_LOG_ERR("Invalid Researches start format. Line was " + nextParam);
        return false;
    }

    while(defFile.good())
    {
        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Researches]")
            break;

        if(nextParam == "RoomCryptPoints")
        {
            defFile >> pointsRoomCrypt;
            continue;
        }

        if(nextParam == "RoomWorkshopPoints")
        {
            defFile >> pointsRoomWorkshop;
            continue;
        }

        if(nextParam == "RoomTrainingHallPoints")
        {
            defFile >> pointsRoomTrainingHall;
            continue;
        }

        if(nextParam == "TrapCannonPoints")
        {
            defFile >> pointsTrapCannon;
            continue;
        }

        if(nextParam == "TrapWoodenDoorPoints")
        {
            defFile >> pointsTrapWoodenDoor;
            continue;
        }

        if(nextParam == "TrapBoulderPoints")
        {
            defFile >> pointsTrapBoulder;
            continue;
        }

        if(nextParam == "TrapSpikePoints")
        {
            defFile >> pointsTrapSpike;
            continue;
        }

        if(nextParam == "SpellCallToWarPoints")
        {
            defFile >> pointsSpellCallToWar;
            continue;
        }

        if(nextParam == "SpellCreatureHeal")
        {
            defFile >> pointsSpellCreatureHeal;
            continue;
        }

        if(nextParam == "SpellCreatureExplosion")
        {
            defFile >> pointsSpellCreatureExplosion;
            continue;
        }
    }

    // We build the research tree
    std::vector<const Research*> depends;
    Research* research;
    depends.clear();
    research = new Research(ResearchType::roomTrainingHall, pointsRoomTrainingHall, depends);
    mResearches.push_back(research);

    depends.clear();
    // Workshop depends on training hall
    depends.push_back(research);
    research = new Research(ResearchType::roomWorkshop, pointsRoomWorkshop, depends);
    mResearches.push_back(research);

    depends.clear();
    // Crypt depends on Workshop
    depends.push_back(research);
    research = new Research(ResearchType::roomCrypt, pointsRoomCrypt, depends);
    mResearches.push_back(research);

    depends.clear();
    research = new Research(ResearchType::trapDoorWooden, pointsTrapWoodenDoor, depends);
    mResearches.push_back(research);

    depends.clear();
    research = new Research(ResearchType::trapCannon, pointsTrapCannon, depends);
    mResearches.push_back(research);

    depends.clear();
    // Spikes depends on Cannon
    depends.push_back(research);
    research = new Research(ResearchType::trapSpike, pointsTrapSpike, depends);
    mResearches.push_back(research);

    depends.clear();
    // Boulder depends on spike
    depends.push_back(research);
    research = new Research(ResearchType::trapBoulder, pointsTrapBoulder, depends);
    mResearches.push_back(research);

    depends.clear();
    research = new Research(ResearchType::spellCallToWar, pointsSpellCallToWar, depends);
    mResearches.push_back(research);

    depends.clear();
    // CreatureHeal depends on CallToWar
    depends.push_back(research);
    research = new Research(ResearchType::spellCreatureHeal, pointsSpellCreatureHeal, depends);
    mResearches.push_back(research);

    depends.clear();
    // CreatureExplosion depends on CreatureHeal
    depends.push_back(research);
    research = new Research(ResearchType::spellCreatureExplosion, pointsSpellCreatureExplosion, depends);
    mResearches.push_back(research);
    return true;
}

bool ConfigManager::loadTilesets(const std::string& fileName)
{
    OD_LOG_INF("Load Tilesets file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    defFile >> nextParam;
    if (nextParam != "[Tilesets]")
    {
        OD_LOG_ERR("Invalid Tilesets start format. Line was " + nextParam);
        return false;
    }

    while(true)
    {
        defFile >> nextParam;
        if (nextParam == "[/Tilesets]")
            break;

        if (nextParam == "[/Tileset]")
            continue;

        if (nextParam != "[Tileset]")
        {
            OD_LOG_ERR("Expecting TileSet tag but got=" + nextParam);
            return false;
        }

        defFile >> nextParam;
        if (nextParam != "Name")
        {
            OD_LOG_ERR("Expecting Name tag but got=" + nextParam);
            return false;
        }

        std::string tileSetName;
        defFile >> tileSetName;

        defFile >> nextParam;
        if (nextParam != "Scale")
        {
            OD_LOG_ERR("Expecting Scale tag but got=" + nextParam);
            return false;
        }

        Ogre::Vector3 scale;
        defFile >> scale.x;
        defFile >> scale.y;
        defFile >> scale.z;

        TileSet* tileSet = new TileSet(scale);
        mTileSets[tileSetName] = tileSet;

        defFile >> nextParam;
        if(nextParam != "[TileLink]")
        {
            OD_LOG_ERR("Expecting TileLink tag but got=" + nextParam);
            return false;
        }

        while(true)
        {
            defFile >> nextParam;
            if(nextParam == "[/TileLink]")
                break;

            TileVisual tileVisual1 = Tile::tileVisualFromString(nextParam);
            if(tileVisual1 == TileVisual::nullTileVisual)
            {
                OD_LOG_ERR("Wrong TileVisual1 in tileset=" + nextParam);
                return false;
            }

            defFile >> nextParam;
            TileVisual tileVisual2 = Tile::tileVisualFromString(nextParam);
            if(tileVisual2 == TileVisual::nullTileVisual)
            {
                OD_LOG_ERR("Wrong TileVisual2 in tileset=" + nextParam);
                return false;
            }

            tileSet->addTileLink(tileVisual1, tileVisual2);
        }

        if(!loadTilesetValues(defFile, TileVisual::goldGround, tileSet->configureTileValues(TileVisual::goldGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::goldFull, tileSet->configureTileValues(TileVisual::goldFull)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::dirtGround, tileSet->configureTileValues(TileVisual::dirtGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::dirtFull, tileSet->configureTileValues(TileVisual::dirtFull)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::rockGround, tileSet->configureTileValues(TileVisual::rockGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::rockFull, tileSet->configureTileValues(TileVisual::rockFull)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::waterGround, tileSet->configureTileValues(TileVisual::waterGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::lavaGround, tileSet->configureTileValues(TileVisual::lavaGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::claimedGround, tileSet->configureTileValues(TileVisual::claimedGround)))
            return false;
        if(!loadTilesetValues(defFile, TileVisual::claimedFull, tileSet->configureTileValues(TileVisual::claimedFull)))
            return false;
    }

    // At least the default tileset should be defined
    if(mTileSets.count(DEFAULT_TILESET_NAME) <= 0)
    {
        OD_LOG_ERR("No tileset defined with name=" + DEFAULT_TILESET_NAME);
        return false;
    }
    return true;
}

bool ConfigManager::loadTilesetValues(std::istream& defFile, TileVisual tileVisual, std::vector<TileSetValue>& tileValues)
{
    std::string nextParam;
    std::string beginTag = "[" + Tile::tileVisualToString(tileVisual) + "]";
    std::string endTag = "[/" + Tile::tileVisualToString(tileVisual) + "]";
    defFile >> nextParam;
    if (nextParam != beginTag)
    {
        OD_LOG_ERR("Expecting " + beginTag + " tag but got=" + nextParam);
        return false;
    }
    while(true)
    {
        std::string indexStr;
        defFile >> indexStr;
        if(indexStr == endTag)
            return true;

        boost::dynamic_bitset<> x(indexStr);
        uint32_t index = static_cast<int>(x.to_ulong());

        std::string meshName;
        defFile >> meshName;

        std::string materialName;
        defFile >> materialName;
        if(materialName.compare("''") == 0)
            materialName.clear();

        double rotX;
        defFile >> rotX;

        double rotY;
        defFile >> rotY;

        double rotZ;
        defFile >> rotZ;

        if(index >= tileValues.size())
        {
            OD_LOG_ERR("Tileset index too high in tileset=" + endTag + ", index=" + indexStr);
            return false;
        }

        tileValues[index] = TileSetValue(meshName, materialName, rotX, rotY, rotZ);
    }
}

void ConfigManager::loadUserConfig(const std::string& fileName)
{
    if (fileName.empty())
        return;

    mFilenameUserCfg = fileName;

    OD_LOG_INF("Load user config file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_INF("Couldn't read " + fileName);
        return;
    }

    std::string nextParam;
    defFile >> nextParam;
    if (nextParam != "[Configuration]")
    {
        OD_LOG_WRN("Invalid User configuration start format. Line was " + nextParam);
        return;
    }

    std::string value;
    USER_CONFIG_CATEGORY category = USER_CONFIG_CATEGORY::CATEGORY_NONE;
    while(defFile.good())
    {
        if (!(defFile >> nextParam))
        {
            break;
        }
        else if (nextParam == "[/Configuration]")
        {
            break;
        }
        else if (nextParam == "[Audio]")
        {
            category = USER_CONFIG_CATEGORY::CATEGORY_AUDIO;
            continue;
        }
        else if (nextParam == "[Video]")
        {
            category = USER_CONFIG_CATEGORY::CATEGORY_VIDEO;
            continue;
        }
        else if (nextParam == "[Input]")
        {
            category = USER_CONFIG_CATEGORY::CATEGORY_INPUT;
            continue;
        }
        else if (nextParam == "[Game]")
        {
            category = USER_CONFIG_CATEGORY::CATEGORY_GAME;
            continue;
        }
        else if (nextParam == "[/Audio]" || nextParam == "[/Video]" || nextParam == "[/Input]"
                 || nextParam == "[/Game]")
        {
            category = USER_CONFIG_CATEGORY::CATEGORY_NONE;
            continue;
        }
        else if (!nextParam.empty())
        {
            std::string line;
            std::getline(defFile, line);
            // Make sure to cut the line only when encountering a tab.
            line = nextParam + line;
            std::vector<std::string> elements = Helper::split(line, '\t');
            if (elements.size() != 2)
            {
                OD_LOG_WRN("Invalid parameter line: " + line);
                continue;
            }

            switch(category)
            {
                case USER_CONFIG_CATEGORY::CATEGORY_AUDIO:
                    mAudioUserConfig[ elements[0] ] = elements[1];
                    break;
                case USER_CONFIG_CATEGORY::CATEGORY_VIDEO:
                    mVideoUserConfig[ elements[0] ] = elements[1];
                    break;
                case USER_CONFIG_CATEGORY::CATEGORY_INPUT:
                    mInputUserConfig[ elements[0] ] = elements[1];
                    break;
                case USER_CONFIG_CATEGORY::CATEGORY_GAME:
                    mGameUserConfig[ elements[0] ] = elements[1];
                    break;
                default:
                    OD_LOG_WRN("Parameter set in unknown category. Will be ignored: "
                               + elements[0] + ": " + elements[1]);
                    break;
            }
            continue;
        }
        else
        {
            continue;
        }
    }
}

bool ConfigManager::saveUserConfig()
{
    if (mFilenameUserCfg.empty())
    {
        OD_LOG_ERR("Can't save config. The user config file isn't set.");
        return false;
    }

    std::ofstream userFile(mFilenameUserCfg.c_str(), std::ifstream::out);
    if (!userFile.is_open())
    {
        OD_LOG_ERR("Couldn't open user config for writing: " + mFilenameUserCfg);
        return false;
    }

    userFile << "[Configuration]" << std::endl;

    userFile << "[Audio]" << std::endl;
    for(std::pair<std::string, std::string> audio : mAudioUserConfig)
        userFile << audio.first << "\t" << audio.second << std::endl;
    userFile << "[/Audio]" << std::endl;

    userFile << "[Video]" << std::endl;
    for(std::pair<std::string, std::string> video : mVideoUserConfig)
        userFile << video.first << "\t" << video.second << std::endl;
    userFile << "[/Video]" << std::endl;

    userFile << "[Input]" << std::endl;
    for(std::pair<std::string, std::string> input : mInputUserConfig)
        userFile << input.first << "\t" << input.second << std::endl;
    userFile << "[/Input]" << std::endl;

    userFile << "[Game]" << std::endl;
    for(std::pair<std::string, std::string> input : mGameUserConfig)
        userFile << input.first << "\t" << input.second << std::endl;
    userFile << "[/Game]" << std::endl;

    userFile << "[/Configuration]" << std::endl;
    userFile.close();
    return true;
}

const std::string& ConfigManager::getAudioValue(const std::string& param) const
{
    if(mAudioUserConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mAudioUserConfig.at(param);
}

const std::string& ConfigManager::getVideoValue(const std::string& param) const
{
    if(mVideoUserConfig.count(param) <= 0)
        return EMPTY_STRING;

    return mVideoUserConfig.at(param);
}

const std::string& ConfigManager::getInputValue(const std::string& param) const
{
    if(mInputUserConfig.count(param) <= 0)
        return EMPTY_STRING;

    return mInputUserConfig.at(param);
}

const std::string& ConfigManager::getGameValue(const std::string& param) const
{
    if(mGameUserConfig.count(param) <= 0)
        return EMPTY_STRING;

    return mGameUserConfig.at(param);
}

const std::string& ConfigManager::getRoomConfigString(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mRoomsConfig.at(param);
}

uint32_t ConfigManager::getRoomConfigUInt32(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mRoomsConfig.at(param));
}

int32_t ConfigManager::getRoomConfigInt32(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mRoomsConfig.at(param));
}

double ConfigManager::getRoomConfigDouble(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mRoomsConfig.at(param));
}

const std::string& ConfigManager::getTrapConfigString(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mTrapsConfig.at(param);
}

uint32_t ConfigManager::getTrapConfigUInt32(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mTrapsConfig.at(param));
}

int32_t ConfigManager::getTrapConfigInt32(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mTrapsConfig.at(param));
}

double ConfigManager::getTrapConfigDouble(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mTrapsConfig.at(param));
}

const std::string& ConfigManager::getSpellConfigString(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mSpellConfig.at(param);
}

uint32_t ConfigManager::getSpellConfigUInt32(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mSpellConfig.at(param));
}

int32_t ConfigManager::getSpellConfigInt32(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mSpellConfig.at(param));
}

double ConfigManager::getSpellConfigDouble(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_LOG_ERR("Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mSpellConfig.at(param));
}

const CreatureDefinition* ConfigManager::getCreatureDefinition(const std::string& name) const
{
    auto it = mCreatureDefs.find(name);
    if(it != mCreatureDefs.end())
    {
        return it->second;
    }
    return nullptr;
}

const Weapon* ConfigManager::getWeapon(const std::string& name) const
{
    for(const Weapon* def : mWeapons)
    {
        if(name.compare(def->getName()) == 0)
            return def;
    }

    return nullptr;
}

Ogre::ColourValue ConfigManager::getColorFromId(const std::string& id) const
{
    if(mSeatColors.count(id) > 0)
        return mSeatColors.at(id);

    return Ogre::ColourValue();
}

const std::vector<const SpawnCondition*>& ConfigManager::getCreatureSpawnConditions(const CreatureDefinition* def) const
{
    if(mCreatureSpawnConditions.count(def) == 0)
        return SpawnCondition::EMPTY_SPAWNCONDITIONS;

    return mCreatureSpawnConditions.at(def);
}

const std::vector<std::string>& ConfigManager::getFactionSpawnPool(const std::string& faction) const
{
    if(mFactionSpawnPool.count(faction) == 0)
        return EMPTY_SPAWNPOOL;

    return mFactionSpawnPool.at(faction);
}

const std::string& ConfigManager::getFactionWorkerClass(const std::string& faction) const
{
    if(mFactionDefaultWorkerClass.count(faction) == 0)
        return EMPTY_STRING;

    return mFactionDefaultWorkerClass.at(faction);
}

CreatureMoodLevel ConfigManager::getCreatureMoodLevel(int32_t moodModifiersPoints) const
{
    int32_t mood = mCreatureBaseMood + moodModifiersPoints;
    if(mood >= mCreatureMoodHappy)
        return CreatureMoodLevel::Happy;

    if(mood >= mCreatureMoodUpset)
        return CreatureMoodLevel::Neutral;

    if(mood >= mCreatureMoodAngry)
        return CreatureMoodLevel::Upset;

    if(mood >= mCreatureMoodFurious)
        return CreatureMoodLevel::Angry;

    return CreatureMoodLevel::Furious;
}

const TileSet* ConfigManager::getTileSet(const std::string& tileSetName) const
{
    if(tileSetName.empty())
        return mTileSets.at(DEFAULT_TILESET_NAME);

    if(mTileSets.count(tileSetName) > 0)
        return mTileSets.at(tileSetName);

    OD_LOG_ERR("Cannot find requested tileset name=" + tileSetName);
    // We return the default tileset
    return mTileSets.at(DEFAULT_TILESET_NAME);
}

bool ConfigManager::initVideoConfig(Ogre::Root& ogreRoot)
{
    std::string rendererName = getVideoValue(Config::RENDERER);

    Ogre::RenderSystem* renderSystem = ogreRoot.getRenderSystemByName(rendererName);
    bool sameRenderer = true;
    if (renderSystem == nullptr)
    {
        const Ogre::RenderSystemList& renderers = ogreRoot.getAvailableRenderers();
        if(renderers.empty())
        {
            OD_LOG_ERR("No valid renderer found. Exiting...");
            return false;
        }
        renderSystem = *renderers.begin();
        OD_LOG_INF("No OpenGL renderer found. Using the first available: " + renderSystem->getName());
        sameRenderer = false;
    }

    ogreRoot.setRenderSystem(renderSystem);
    Ogre::ConfigOptionMap& options = renderSystem->getConfigOptions();

    // If the renderer was changed, we need to reset the video options.
    if (sameRenderer == false)
    {
        mVideoUserConfig.clear();

        for (std::pair<Ogre::String, Ogre::ConfigOption> option : options)
        {
            std::string optionName = option.first;
            Ogre::ConfigOption& values = option.second;
            // We don't store options that are immutable or empty
            if (values.immutable || values.possibleValues.empty())
                continue;

            setVideoValue(optionName, values.currentValue);
        }
    }
    else {
        std::vector<std::string> optionsToRemove;

        // The renderer system was initialized. Let's load its new option values.
        for (std::pair<std::string, std::string> setting : mVideoUserConfig)
        {
            // Check the option exists.
            if (options.find(setting.first) == options.end())
            {
                optionsToRemove.push_back(setting.first);
                continue;
            }

            // Check the desired option value exists.
            Ogre::ConfigOption& values = options.find(setting.first)->second;
            bool valueIsPossible = false;
            for (std::string value : values.possibleValues)
            {
                if (setting.second == value)
                {
                    valueIsPossible = true;
                    break;
                }
            }
            if (!valueIsPossible)
            {
                optionsToRemove.push_back(setting.first);
                continue;
            }

            renderSystem->setConfigOption(setting.first, setting.second);
        }

        // Removes now invalid options from the video options.
        for (std::string option : optionsToRemove)
            mVideoUserConfig.erase(option);
    }

    return true;
}
