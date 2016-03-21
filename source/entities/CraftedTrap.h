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

#ifndef CRAFTEDTRAP_H
#define CRAFTEDTRAP_H

#include "entities/RenderedMovableEntity.h"

#include <string>
#include <iosfwd>

class Creature;
class Room;
class GameMap;
class Tile;
class ODPacket;

enum class TrapType;

class CraftedTrap: public RenderedMovableEntity
{
public:
    CraftedTrap(GameMap* gameMap, const std::string& workshopName, TrapType trapType);
    CraftedTrap(GameMap* gameMap);

    virtual GameEntityType getObjectType() const override;

    virtual const Ogre::Vector3& getScale() const;

    TrapType getTrapType() const
    { return mTrapType; }

    virtual EntityCarryType getEntityCarryType(Creature* carrier) override
    { return EntityCarryType::craftedTrap; }

    virtual void notifyEntityCarryOn(Creature* carrier) override;
    virtual void notifyEntityCarryOff(const Ogre::Vector3& position) override;

    static CraftedTrap* getCraftedTrapFromStream(GameMap* gameMap, std::istream& is);
    static CraftedTrap* getCraftedTrapFromPacket(GameMap* gameMap, ODPacket& is);
    static std::string getCraftedTrapStreamFormat();
protected:
    void exportToStream(std::ostream& os) const override;
    bool importFromStream(std::istream& is) override;

private:
    TrapType mTrapType;
};

#endif // CRAFTEDTRAP_H
