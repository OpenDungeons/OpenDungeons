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


#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <OgreSingleton.h>
#include <OgreSharedPtr.h>
#include <string>

#include "CreatureClass.h"
class GameMap;


namespace MapLoader
{
    //TODO - rename variable back to gameMap once we have gotten rid of the gameMap in globals.h
    bool readGameMapFromFile(const std::string& fileName, GameMap& gameMap_b);
    //TODO: make gameMap_b const
    void writeGameMapToFile(const std::string& fileName, GameMap& gameMap_b);
    Ogre::SharedPtr<CreatureClass> loadCreatureDefinition(std::string& filename);
};

#endif // MAPLOADER_H
