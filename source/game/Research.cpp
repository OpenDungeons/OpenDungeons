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

#include "game/Research.h"

#include "game/Player.h"
#include "network/ODPacket.h"
#include "utils/Helper.h"


Research::Research(ResearchType type, int32_t neededResearchPoints, const std::vector<const Research*>& researchDepends):
    mType(type),
    mNeededResearchPoints(neededResearchPoints),
    mResearchDepends(researchDepends)
{
}

bool Research::canBeResearched(const std::vector<ResearchType>& researchesDone) const
{
    if(mResearchDepends.empty())
        return true;

    for(const Research* research : mResearchDepends)
    {
        if(std::find(researchesDone.begin(), researchesDone.end(), research->getType()) == researchesDone.end())
            continue;

        return true;
    }
    return false;
}

std::string Research::researchTypeToString(ResearchType type)
{
    switch(type)
    {
        case ResearchType::nullResearchType:
            return "nullResearchType";
        case ResearchType::roomCrypt:
            return "roomCrypt";
        case ResearchType::roomDormitory:
            return "roomDormitory";
        case ResearchType::roomWorkshop:
            return "roomWorkshop";
        case ResearchType::roomHatchery:
            return "roomHatchery";
        case ResearchType::roomLibrary:
            return "roomLibrary";
        case ResearchType::roomTrainingHall:
            return "roomTrainingHall";
        case ResearchType::roomTreasury:
            return "roomTreasury";
        case ResearchType::spellCallToWar:
            return "spellCallToWar";
        case ResearchType::spellSummonWorker:
            return "spellSummonWorker";
        case ResearchType::spellCreatureHeal:
            return "spellCreatureHeal";
        case ResearchType::spellCreatureExplode:
            return "spellCreatureExplode";
        case ResearchType::trapBoulder:
            return "trapBoulder";
        case ResearchType::trapCannon:
            return "trapCannon";
        case ResearchType::trapSpike:
            return "trapSpike";
        default:
            return "Unknown enum value:" + Helper::toString(static_cast<int>(type));
    }
}

std::string Research::researchTypeToPlayerVisibleString(ResearchType type)
{
    switch(type)
    {
        case ResearchType::nullResearchType:
            return "No Research type";
        case ResearchType::roomCrypt:
            return "The Crypt Room";
        case ResearchType::roomDormitory:
            return "The Dormitory Room";
        case ResearchType::roomWorkshop:
            return "The Workshop Room";
        case ResearchType::roomHatchery:
            return "The Hatchery Room";
        case ResearchType::roomLibrary:
            return "The Library Room";
        case ResearchType::roomTrainingHall:
            return "The TrainingHall Room";
        case ResearchType::roomTreasury:
            return "The Treasury Room";
        case ResearchType::spellCallToWar:
            return "The 'Call to War' Spell";
        case ResearchType::spellSummonWorker:
            return "The 'Summon Worker' Spell";
        case ResearchType::trapBoulder:
            return "The Boulder Trap";
        case ResearchType::trapCannon:
            return "The Cannon Trap";
        case ResearchType::trapSpike:
            return "The Spike Trap";
        default:
            return "Unknown enum value:" + Helper::toString(static_cast<int>(type));
    }
}

ODPacket& operator<<(ODPacket& os, const ResearchType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, ResearchType& type)
{
    int32_t tmp;
    is >> tmp;
    type = static_cast<ResearchType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const ResearchType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, ResearchType& type)
{
    int32_t tmp;
    is >> tmp;
    type = static_cast<ResearchType>(tmp);
    return is;
}
