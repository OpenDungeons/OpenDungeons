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

#include "entities/SkillEntity.h"

#include "entities/Creature.h"
#include "entities/GameEntityType.h"
#include "entities/Tile.h"
#include "network/ODPacket.h"
#include "game/Skill.h"
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

SkillEntity::SkillEntity(GameMap* gameMap, bool isOnServerMap, const std::string& libraryName, int32_t skillPoints) :
    RenderedMovableEntity(gameMap, isOnServerMap, libraryName, "Grimoire", 0.0f, false, 1.0f),
    mSkillPoints(skillPoints)
{
    mPrevAnimationState = "Loop";
    mPrevAnimationStateLoop = true;
}

SkillEntity::SkillEntity(GameMap* gameMap, bool isOnServerMap) :
    RenderedMovableEntity(gameMap, isOnServerMap)
{
}

GameEntityType SkillEntity::getObjectType() const
{
    return GameEntityType::skillEntity;
}

const Ogre::Vector3& SkillEntity::getScale() const
{
    return SCALE;
}

void SkillEntity::notifyEntityCarryOn(Creature* carrier)
{
    removeEntityFromPositionTile();
    setSeat(carrier->getSeat());
}

void SkillEntity::notifyEntityCarryOff(const Ogre::Vector3& position)
{
    mPosition = position;
    addEntityToPositionTile();
}

SkillEntity* SkillEntity::getSkillEntityFromStream(GameMap* gameMap, std::istream& is)
{
    SkillEntity* obj = new SkillEntity(gameMap, true);
    obj->importFromStream(is);
    return obj;
}

SkillEntity* SkillEntity::getSkillEntityFromPacket(GameMap* gameMap, ODPacket& is)
{
    SkillEntity* obj = new SkillEntity(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void SkillEntity::exportToStream(std::ostream& os) const
{
    RenderedMovableEntity::exportToStream(os);
    os << mSkillPoints << "\t";
    os << mPosition.x << "\t" << mPosition.y << "\t" << mPosition.z << "\t";
}

bool SkillEntity::importFromStream(std::istream& is)
{
    if(!RenderedMovableEntity::importFromStream(is))
        return false;
    if(!(is >> mSkillPoints))
        return false;
    if(!(is >> mPosition.x >> mPosition.y >> mPosition.z))
        return false;

    return true;
}

std::string SkillEntity::getSkillEntityStreamFormat()
{
    std::string format = RenderedMovableEntity::getRenderedMovableEntityStreamFormat();
    if(!format.empty())
        format += "\t";

    format += "skillPoints\tPosX\tPosY\tPosZ";

    return format;
}
