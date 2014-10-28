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

#include "entities/CreatureDefinition.h"
#include "entities/Weapon.h"

#include "utils/ConfigManager.h"
#include "utils/ResourceManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

template<> ConfigManager* Ogre::Singleton<ConfigManager>::msSingleton = 0;

ConfigManager::ConfigManager() :
    mNetworkPort(0)
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
    }

    if(filesOk != 0x03)
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
    if (nextParam != "[EquipmentDefinitions]")
    {
        OD_ASSERT_TRUE_MSG(false, "Invalid Creature classes start format. Line was " + nextParam);
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

Ogre::ColourValue ConfigManager::getColorFromId(const std::string& id) const
{
    if(mSeatColors.count(id) > 0)
        return mSeatColors.at(id);

    return Ogre::ColourValue();
}
