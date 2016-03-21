/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef INPUTCOMMAND_H
#define INPUTCOMMAND_H

#include <string>
#include <vector>

namespace Ogre
{
class ColourValue;
}

class Tile;

//! Abstract class used for spell manager clients (on client side)
class InputCommand
{
public:
    //! \brief Notify the InputCommand that we want to display tiles within given square
    //! as selected for the local player
    virtual void selectSquaredTiles(int tileX1, int tileY1, int tileX2, int tileY2) = 0;
    virtual void selectTiles(const std::vector<Tile*> tiles) = 0;
    virtual void unselectAllTiles() = 0;
    //! \brief Notify the InputCommand that we want to display the given text to the local player
    virtual void displayText(const Ogre::ColourValue& txtColour, const std::string& txt) = 0;
};


#endif // INPUTCOMMAND_H
