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

#include "entities/ResearchEntity.h"

#include "entities/Creature.h"
#include "entities/Tile.h"

#include "network/ODPacket.h"

#include "game/Research.h"

#include "gamemap/GameMap.h"

#include "traps/Trap.h"
#include "traps/TrapBoulder.h"
#include "traps/TrapCannon.h"
#include "traps/TrapSpike.h"

#include "utils/Random.h"
#include "utils/LogManager.h"

#include <iostream>

const std::string EMPTY_STRING;

const Ogre::Vector3 SCALE(0.5,0.5,0.5);

ResearchEntity::ResearchEntity(GameMap* gameMap, const std::string& libraryName, ResearchType researchType) :
    RenderedMovableEntity(gameMap, libraryName, "Grimoire", 0.0f, false, 1.0f, "Loop"),
    mResearchType(researchType)
{
}

ResearchEntity::ResearchEntity(GameMap* gameMap) :
    RenderedMovableEntity(gameMap)
{
}

const Ogre::Vector3& ResearchEntity::getScale() const
{
    return SCALE;
}

void ResearchEntity::notifyEntityCarryOn(Creature* carrier)
{
    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    setIsOnMap(false);
    setSeat(carrier->getSeat());
    myTile->removeEntity(this);
}

void ResearchEntity::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    setIsOnMap(true);

    Tile* myTile = getPositionTile();
    OD_ASSERT_TRUE_MSG(myTile != nullptr, "name=" + getName());
    if(myTile == nullptr)
        return;

    myTile->addEntity(this);
}

ResearchEntity* ResearchEntity::getResearchEntityFromStream(GameMap* gameMap, std::istream& is)
{
    ResearchEntity* obj = new ResearchEntity(gameMap);
    return obj;
}

ResearchEntity* ResearchEntity::getResearchEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    ResearchEntity* obj = new ResearchEntity(gameMap);
    return obj;
}

void ResearchEntity::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mResearchType << "\t";
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

void ResearchEntity::importFromStream(std::istream& is)
{
    RenderedMovableEntity::importFromStream(is);
    OD_ASSERT_TRUE(is >> mResearchType);
    OD_ASSERT_TRUE(is >> mPosition.x >> mPosition.y >> mPosition.z);
}

std::string ResearchEntity::getResearchEntityStreamFormat()
{
    return RenderedMovableEntity::getRenderedMovableEntityStreamFormat()
        + "researchType\tPosX\tPosY\tPosZ\t";
}
