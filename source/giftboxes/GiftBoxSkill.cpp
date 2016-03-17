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

#include "giftboxes/GiftBoxSkill.h"

#include "game/Skill.h"
#include "game/SkillType.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <iostream>

GiftBoxSkill::GiftBoxSkill(GameMap* gameMap, const std::string& baseName, SkillType skillType) :
    GiftBoxEntity(gameMap, baseName, "MysteryBox", GiftBoxType::skill),
    mSkillType(skillType)
{
    mPrevAnimationState = "Loop";
    mPrevAnimationStateLoop = true;
}

GiftBoxSkill::GiftBoxSkill(GameMap* gameMap) :
    GiftBoxEntity(gameMap),
    mSkillType(SkillType::nullSkillType)
{
}

void GiftBoxSkill::exportToStream(std::ostream& os) const
{
    GiftBoxEntity::exportToStream(os);
    os << mSkillType << "\t";
}

bool GiftBoxSkill::importFromStream(std::istream& is)
{
    if(!GiftBoxEntity::importFromStream(is))
        return false;
    if(!(is >> mSkillType))
        return false;

    return true;
}

void GiftBoxSkill::applyEffect()
{
    if(getSeat() == nullptr)
    {
        OD_LOG_ERR("null Seat for giftbox=" + getName() + " pos=" + Helper::toString(getPosition()));
        return;
    }

    getSeat()->addSkill(mSkillType);
}
