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

#include "entities/BuildingObject.h"

#include "network/ODPacket.h"

#include "gamemap/GameMap.h"

#include "traps/TrapBoulder.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"

#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

BuildingObject::BuildingObject(GameMap* gameMap, const std::string& buildingName, const std::string& meshName,
        const Ogre::Vector3& position, Ogre::Real rotationAngle, bool hideCoveredTile, float opacity) :
    RenderedMovableEntity(gameMap, buildingName, meshName, rotationAngle, hideCoveredTile, opacity)
{
    mPosition = position;
}

BuildingObject::BuildingObject(GameMap* gameMap) :
    RenderedMovableEntity(gameMap)
{
}

BuildingObject* BuildingObject::getBuildingObjectFromPacket(GameMap* gameMap, ODPacket& is)
{
    BuildingObject* obj = new BuildingObject(gameMap);
    return obj;
}
