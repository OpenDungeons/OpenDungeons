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

#include "entities/CreatureDefinition.h"
#include "entities/Weapon.h"

#include "creaturemood/CreatureMood.h"

#include "spawnconditions/SpawnCondition.h"

#include "utils/ConfigManager.h"
#include "utils/ResourceManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

const std::vector<std::string> EMPTY_SPAWNPOOL;
const std::string EMPTY_STRING;

template<> ConfigManager* Ogre::Singleton<ConfigManager>::msSingleton = 0;

ConfigManager::ConfigManager() :
    mNetworkPort(0),
    mBaseSpawnPoint(10),
    mCreatureDeathCounter(10),
    mMaxCreaturesPerSeat(15),
    mCreatureBaseMood(1000),
    mCreatureMoodHappy(1200),
    mCreatureMoodUpset(0),
    mCreatureMoodAngry(-1000),
    mCreatureMoodFurious(-2000),
    mSlapDamagePercent(15),
    mTimePayDay(300),
    mNbTurnsFuriousMax(120),
    mMaxManaPerSeat(250000.0)
{
    if(!loadGlobalConfig())
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    std::string fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameCreatureDefinition;
    if(!loadCreatureDefinitions(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameEquipmentDefinition;
    if(!loadEquipements(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameSpawnConditions;
    if(!loadSpawnConditions(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameFactions;
    if(!loadFactions(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameRooms;
    if(!loadRooms(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameTraps;
    if(!loadTraps(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameSpells;
    if(!loadSpellConfig(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
    fileName = ResourceManager::getSingleton().getConfigPath() + mFilenameCreaturesMood;
    if(!loadCreaturesMood(fileName))
    {
        OD_ASSERT_TRUE(false);
        exit(1);
    }
}

ConfigManager::~ConfigManager()
{
    for(const CreatureDefinition* def : mCreatureDefs)
    {
        delete def;
    }
    mCreatureDefs.clear();
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
}

bool ConfigManager::loadGlobalConfig()
{
    std::stringstream configFile;
    std::string fileName = ResourceManager::getSingleton().getConfigPath() + "global.cfg";
    if(!Helper::readFileWithoutComments(fileName, configFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
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
        OD_ASSERT_TRUE_MSG(false, "Not enough parameters for config file paramsOk=" + Ogre::StringConverter::toString(paramsOk));
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
            OD_ASSERT_TRUE_MSG(false, "Wrong parameter read nextParam=" + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Missing parameters paramsOk=" + Ogre::StringConverter::toString(paramsOk));
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
    }

    if(filesOk != 0xFF)
    {
        OD_ASSERT_TRUE_MSG(false, "Missing parameter file filesOk=" + Ogre::StringConverter::toString(filesOk));
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
            OD_ASSERT_TRUE_MSG(false, "Wrong parameter read nextParam=" + nextParam);
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
                    OD_ASSERT_TRUE_MSG(false, "Wrong parameter read nextParam=" + nextParam);
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
                    OD_ASSERT_TRUE_MSG(false, "Wrong parameter read nextParam=" + nextParam);
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
                    OD_ASSERT_TRUE_MSG(false, "Wrong parameter read nextParam=" + nextParam);
                    return false;
                }
                colourValue.b = v;
                paramsOk |= 0x08;
                continue;
            }
        }

        if(paramsOk != 0x0F)
        {
            OD_ASSERT_TRUE_MSG(false, "Missing parameters paramsOk=" + Ogre::StringConverter::toString(paramsOk));
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

        if(nextParam == "CreatureDeathCounter")
        {
            configFile >> nextParam;
            mCreatureDeathCounter = Helper::toUInt32(nextParam);
            // Not mandatory
        }

        if(nextParam == "MaxCreaturesPerSeat")
        {
            configFile >> nextParam;
            mMaxCreaturesPerSeat = Helper::toUInt32(nextParam);
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
    }

    if(paramsOk != 0x01)
    {
        OD_ASSERT_TRUE_MSG(false, "Missing parameters paramsOk=" + Ogre::StringConverter::toString(paramsOk));
        return false;
    }

    return true;
}

bool ConfigManager::loadCreatureDefinitions(const std::string& fileName)
{
    LogManager::getSingleton().logMessage("Load creature definition file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[CreatureDefinitions]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid Creature classes start format. Line was " + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Invalid Creature classes start format. Line was " + nextParam);
            return false;
        }

        // Load the creature definition until a [/Creature] tag is found
        CreatureDefinition* creatureDef = CreatureDefinition::load(defFile);
        if (creatureDef == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "Invalid Creature classes start format");
            return false;
        }
        mCreatureDefs.push_back(creatureDef);
    }

    return true;
}

bool ConfigManager::loadEquipements(const std::string& fileName)
{
    LogManager::getSingleton().logMessage("Load weapon definition file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[EquipmentDefinitions]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid weapon start format. Line was " + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Invalid Weapon definition format. Line was " + nextParam);
            return false;
        }

        // Load the definition
        Weapon* weapon = Weapon::load(defFile);
        if (weapon == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "Invalid Weapon definition format");
            return false;
        }
        mWeapons.push_back(weapon);
    }

    return true;
}

bool ConfigManager::loadSpawnConditions(const std::string& fileName)
{
    LogManager::getSingleton().logMessage("Load creature spawn conditions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[SpawnConditions]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition start format. Line was " + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition format. Line was " + nextParam);
            return false;
        }

        if(!(defFile >> nextParam))
                break;
        if (nextParam != "CreatureClass")
        {
            OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition format. Line was " + nextParam);
            return false;
        }
        defFile >> nextParam;
        const CreatureDefinition* creatureDefinition = getCreatureDefinition(nextParam);
        if(creatureDefinition == nullptr)
        {
            OD_ASSERT_TRUE_MSG(false, "nextParam=" + nextParam);
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
                OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition format. nextParam=" + nextParam);
                return false;
            }

            // Load the definition
            SpawnCondition* def = SpawnCondition::load(defFile);
            if (def == nullptr)
            {
                OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition format");
                return false;
            }
            mCreatureSpawnConditions[creatureDefinition].push_back(def);
        }
    }

    return true;
}

bool ConfigManager::loadFactions(const std::string& fileName)
{
    LogManager::getSingleton().logMessage("Load factions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Factions]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid factions start format. Line was " + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Invalid faction. Line was " + nextParam);
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
                OD_ASSERT_TRUE_MSG(false, "Empty or missing faction name is not allowed");
                return false;
            }
            mFactions.push_back(factionName);

            if (nextParam == "WorkerClass")
            {
                defFile >> workerClass;
                continue;
            }
            if(workerClass.empty())
            {
                OD_ASSERT_TRUE_MSG(false, "Empty or missing WorkerClass name is not allowed");
                return false;
            }
            mFactionDefaultWorkerClass[factionName] = workerClass;

            if (nextParam != "[SpawnPool]")
            {
                OD_ASSERT_TRUE_MSG(false, "Invalid faction. Line was " + nextParam);
                return false;
            }

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
                OD_ASSERT_TRUE_MSG(creatureDefinition != nullptr, "factionName=" + factionName + ", class=" + nextParam);
                if(creatureDefinition == nullptr)
                    continue;

                mFactionSpawnPool[factionName].push_back(nextParam);
            }
        }
    }

    return true;
}

bool ConfigManager::loadRooms(const std::string& fileName)
{
    LogManager::getSingleton().logMessage("Load Rooms file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Rooms]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid factions start format. Line was " + nextParam);
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
    LogManager::getSingleton().logMessage("Load traps file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Traps]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid Traps start format. Line was " + nextParam);
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
    LogManager::getSingleton().logMessage("Load Spell config file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[Spells]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid Spells start format. Line was " + nextParam);
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
    LogManager::getSingleton().logMessage("Load creature spawn conditions file: " + fileName);
    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_ASSERT_TRUE_MSG(false, "Couldn't read " + fileName);
        return false;
    }

    std::string nextParam;
    // Read in the creature class descriptions
    defFile >> nextParam;
    if (nextParam != "[CreaturesMood]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid creature spawn condition start format. Line was " + nextParam);
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
            OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood format. Line was " + nextParam);
            return false;
        }

        if(!(defFile >> nextParam))
                break;
        if (nextParam != "CreatureMoodName")
        {
            OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMoodName format. Line was " + nextParam);
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
                OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood MoodModifier format. nextParam=" + nextParam);
                return false;
            }

            // Load the definition
            CreatureMood* def = CreatureMood::load(defFile);
            if (def == nullptr)
            {
                OD_ASSERT_TRUE_MSG(false, "Invalid CreatureMood MoodModifier definition");
                return false;
            }
            moodModifiers.push_back(def);
        }
    }

    return true;
}

const std::string& ConfigManager::getRoomConfigString(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mRoomsConfig.at(param);
}

uint32_t ConfigManager::getRoomConfigUInt32(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mRoomsConfig.at(param));
}

int32_t ConfigManager::getRoomConfigInt32(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mRoomsConfig.at(param));
}

double ConfigManager::getRoomConfigDouble(const std::string& param) const
{
    if(mRoomsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mRoomsConfig.at(param));
}

const std::string& ConfigManager::getTrapConfigString(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mTrapsConfig.at(param);
}

uint32_t ConfigManager::getTrapConfigUInt32(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mTrapsConfig.at(param));
}

int32_t ConfigManager::getTrapConfigInt32(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mTrapsConfig.at(param));
}

double ConfigManager::getTrapConfigDouble(const std::string& param) const
{
    if(mTrapsConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mTrapsConfig.at(param));
}

const std::string& ConfigManager::getSpellConfigString(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return EMPTY_STRING;
    }

    return mSpellConfig.at(param);
}

uint32_t ConfigManager::getSpellConfigUInt32(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toUInt32(mSpellConfig.at(param));
}

int32_t ConfigManager::getSpellConfigInt32(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0;
    }

    return Helper::toInt(mSpellConfig.at(param));
}

double ConfigManager::getSpellConfigDouble(const std::string& param) const
{
    if(mSpellConfig.count(param) <= 0)
    {
        OD_ASSERT_TRUE_MSG(false, "Unknown parameter param=" + param);
        return 0.0;
    }

    return Helper::toDouble(mSpellConfig.at(param));
}

const CreatureDefinition* ConfigManager::getCreatureDefinition(const std::string& name) const
{
    for(const CreatureDefinition* def : mCreatureDefs)
    {
        if(name.compare(def->getClassName()) == 0)
            return def;
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
