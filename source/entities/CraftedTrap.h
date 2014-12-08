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

#ifndef CRAFTEDTRAP_H
#define CRAFTEDTRAP_H

#include "entities/RenderedMovableEntity.h"

#include "traps/Trap.h"

#include <string>
#include <istream>
#include <ostream>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

class CraftedTrap: public RenderedMovableEntity
{
public:
    CraftedTrap(GameMap* gameMap, const std::string& forgeName, Trap::TrapType trapType);
    CraftedTrap(GameMap* gameMap);

    virtual RenderedMovableEntityType getRenderedMovableEntityType()
    { return RenderedMovableEntityType::craftedTrap; }

    virtual Ogre::Vector3 getScale() const
    { return Ogre::Vector3(0.5,0.5,0.5); }


    virtual void setPosition(const Ogre::Vector3& v);

    Trap::TrapType getTrapType() const
    { return mTrapType; }

    void notifyEntityCarried(bool isCarried);

    static CraftedTrap* getCraftedTrapFromStream(GameMap* gameMap, std::istream& is);
    static CraftedTrap* getCraftedTrapFromPacket(GameMap* gameMap, ODPacket& is);
    static const char* getFormat();
private:
    Trap::TrapType mTrapType;

    const std::string& getMeshFromTrapType(Trap::TrapType trapType);
};

#endif // CRAFTEDTRAP_H
