/*!
 * \file   Building.h
 * \date:  22 March 2011
 * \author StefanP.MUC
 * \brief  Provides common methods and members for buildable objects, like rooms and traps
 *
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

#ifndef BUILDING_H_
#define BUILDING_H_

#include "GameEntity.h"
#include "Seat.h"

class GameMap;

/*! \class GameEntity GameEntity.h
 *  \brief This class holds elements that are common to every object placed in the game
 *
 * Functions and properties that are common to every buildable object like rooms and traps
 * should be placed into this classand initialised with a good default value in the default
 * constructor. Member variables are private and only accessed through getters and setters.
 */
class Building : public GameEntity
{
public:
    //! \brief Default constructor with default values
    Building(GameMap* gameMap) :
        GameEntity(gameMap),
        mControllingSeat(NULL)
    {}

    virtual ~Building() {}

    inline Seat* getControllingSeat() const
    { return mControllingSeat; }

    inline void setControllingSeat(Seat* nCS) {
        mControllingSeat = nCS;
        setColor(nCS->getColor());
    }

private:
    //! \brief The Seat controlling this building
    Seat* mControllingSeat;
};

#endif // BUILDING_H_
