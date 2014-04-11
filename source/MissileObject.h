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

#ifndef MISSILEOBJECT_H
#define MISSILEOBJECT_H

#include "MovableGameEntity.h"

#include <deque>
#include <string>

#include <OgreVector3.h>
#include <semaphore.h>

class GameMap;

//! \brief This class implements missile object launched by traps when they're triggered.
class MissileObject: public MovableGameEntity
{
public:
    MissileObject(const std::string& nMeshName, const Ogre::Vector3& nPosition, GameMap* gameMap);

    /*! \brief Changes the missile's position to a new position.
     *  Moves the creature to a new location in 3d space.  This function is
     *  responsible for informing OGRE anything it needs to know.
     */
    void setPosition(const Ogre::Vector3& v);

    std::string getOgreNamePrefix()
    {
        return "";
    }
    
    virtual bool doUpkeep();
    
    //! \brief The missile reach the end of the travel and is destroyed.
    virtual void stopWalking();

    //TODO: implement those functions
    void recieveExp(double experience)
    {}

    void takeDamage(double damage, Tile* tileTakingDamage)
    {}

    double getDefense() const
    { return 0.0; }

    double getHP(Tile* tile)
    { return 0; }

    std::vector<Tile*> getCoveredTiles()
    { return std::vector<Tile*>(); }

    static sem_t mMissileObjectUniqueNumberLockSemaphore;

private:
    //! \brief Position lock semaphore
    sem_t mPositionLockSemaphore;
};

#endif // MISSILEOBJECT_H

