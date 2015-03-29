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


#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <string>

class GameMap;

enum class GameEntityType;

//! \brief A small structure storing level info for the player
struct LevelInfo
{
    LevelInfo()
    {}

    //! \brief The level visible name
    std::string mLevelName;

    //! \brief The level description, player's slot, size, ...
    std::string mLevelDescription;
};

namespace MapLoader
{
    bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap);

    void writeGameMapToFile(const std::string& fileName, GameMap& gameMap);

    bool readGameEntity(GameMap& gameMap, const std::string& item, GameEntityType type, std::stringstream& levelFile);

    bool loadEquipments(const std::string& fileName, GameMap& gameMap);

    bool loadCreatureDefinition(const std::string& fileName, GameMap& gameMap);

    //! \brief Reads the main user map info. Returns true if the level could be read and levelInfo is set to
    //! corresponding info. Returns false otherwise.
    bool getMapInfo(const std::string& fileName, LevelInfo& levelInfo);
};

#endif // MAPLOADER_H
