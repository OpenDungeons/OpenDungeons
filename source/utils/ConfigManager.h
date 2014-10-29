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

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <OgreSingleton.h>
#include <OgreColourValue.h>

class CreatureDefinition;
class Weapon;

//! \brief This class is used to manage global configuration such as network configuration, global creature stats, ...
//! It should NOT be used to load level specific stuff. For that, there if GameMap.
class ConfigManager : public Ogre::Singleton<ConfigManager>
{
public:
    ConfigManager();
    ~ConfigManager();

    Ogre::ColourValue getColorFromId(const std::string& id) const;
    const std::vector<const CreatureDefinition*>& getCreatureDefinitions() const
    { return mCreatureDefs; }
    const CreatureDefinition* getCreatureDefinition(const std::string& name) const;

    const std::vector<const Weapon*>& getWeapons() const
    { return mWeapons; }
    const Weapon* getWeapon(const std::string& name) const;

    uint32_t getNetworkPort() const
    { return mNetworkPort; }

private:
    //! \brief Function used to load the global configuration. They should return true if the configuration
    //! is ok and false if a mandatory parameter is missing
    bool loadGlobalConfig();
    bool loadGlobalConfigSeatColors(std::stringstream& configFile);
    bool loadGlobalConfigDefinitionFiles(std::stringstream& configFile);
    bool loadGlobalGameConfig(std::stringstream& configFile);
    bool loadCreatureDefinitions(const std::string& fileName);
    bool loadEquipements(const std::string& fileName);

    std::map<std::string, Ogre::ColourValue> mSeatColors;
    std::vector<const CreatureDefinition*> mCreatureDefs;
    std::vector<const Weapon*> mWeapons;
    std::string mFilenameCreatureDefinition;
    std::string mFilenameEquipmentDefinition;
    uint32_t mNetworkPort;
};

#endif //CONFIGMANAGER_H
