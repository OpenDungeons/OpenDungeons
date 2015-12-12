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

#include "entities/CraftedTrap.h"

#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "gamemap/GameMap.h"
#include "traps/Trap.h"
#include "traps/TrapManager.h"
#include "traps/TrapType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <iostream>

const Ogre::Vector3 SCALE(0.5,0.5,0.5);

CraftedTrap::CraftedTrap(GameMap* gameMap, bool isOnServerMap, const std::string& workshopName, TrapType trapType) :
    RenderedMovableEntity(gameMap, isOnServerMap, workshopName, TrapManager::getMeshFromTrapType(trapType), 0.0f, false),
    mTrapType(trapType)
{
}

CraftedTrap::CraftedTrap(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap)
{
}

GameEntityType CraftedTrap::getObjectType() const
{
    return GameEntityType::craftedTrap;
}


const Ogre::Vector3& CraftedTrap::getScale() const
{
    return SCALE;
}

void CraftedTrap::notifyEntityCarryOn(Creature* carrier)
{
    removeEntityFromPositionTile();
    setSeat(carrier->getSeat());
}

void CraftedTrap::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    addEntityToPositionTile();
}

CraftedTrap* CraftedTrap::getCraftedTrapFromStream(GameMap* gameMap, std::istream& is)
{
    CraftedTrap* obj = new CraftedTrap(gameMap, true);
    obj->importFromStream(is);
    return obj;
}

CraftedTrap* CraftedTrap::getCraftedTrapFromPacket(GameMap* gameMap, ODPacket& is)
{
    CraftedTrap* obj = new CraftedTrap(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void CraftedTrap::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mTrapType << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

bool CraftedTrap::importFromStream(std::istream& is)
{
    if(!RenderedMovableEntity::importFromStream(is))
        return false;
    if(!(is >> mTrapType >> mPosition.x >> mPosition.y >> mPosition.z))
        return false;

    return true;
}

std::string CraftedTrap::getCraftedTrapStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "trapType\tPosX\tPosY\tPosZ";

    return format;
}
