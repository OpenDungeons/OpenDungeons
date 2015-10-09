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

#include "giftboxes/GiftBoxResearch.h"

#include "game/Research.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <iostream>

GiftBoxResearch::GiftBoxResearch(GameMap* gameMap, bool isOnServerMap, const std::string& baseName, ResearchType researchType) :
    GiftBoxEntity(gameMap, isOnServerMap, baseName, "MysteryBox", GiftBoxType::research),
    mResearchType(researchType)
{
    mPrevAnimationState = "Loop";
    mPrevAnimationStateLoop = true;
}

GiftBoxResearch::GiftBoxResearch(GameMap* gameMap, bool isOnServerMap) :
    GiftBoxEntity(gameMap, isOnServerMap),
    mResearchType(ResearchType::nullResearchType)
{
}

void GiftBoxResearch::exportToStream(std::ostream& os) const
{
    GiftBoxEntity::exportToStream(os);
    os << mResearchType << "\t";
}

bool GiftBoxResearch::importFromStream(std::istream& is)
{
    if(!GiftBoxEntity::importFromStream(is))
        return false;
    if(!(is >> mResearchType))
        return false;

    return true;
}

void GiftBoxResearch::applyEffect()
{
    if(getSeat() == nullptr)
    {
        OD_LOG_ERR("null Seat for giftbox=" + getName() + " pos=" + Helper::toString(getPosition()));
        return;
    }

    getSeat()->addResearch(mResearchType);
}
