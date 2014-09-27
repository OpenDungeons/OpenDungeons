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


#ifndef MAPLOADER_H
#define MAPLOADER_H

#include "CreatureDefinition.h"

#include <OgreSingleton.h>
#include <OgreSharedPtr.h>

#include <string>

class GameMap;

namespace MapLoader
{
    bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap);

    void writeGameMapToFile(const std::string& fileName, GameMap& gameMap);

    bool loadCreatureDefinition(const std::string& fileName, GameMap& gameMap);

    // TODO: This is sub-optimal. Later we might want to create a map info struct
    //! \brief Returns the main map info as a string
    std::string getMapDescription(const std::string& fileName);

    //! \brief Returns the map name as a string
    //! or the filename if the map is invalid.
    std::string getMapName(const std::string& fileName);
};

#endif // MAPLOADER_H
