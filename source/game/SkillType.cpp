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

#include "game/SkillType.h"

#include "network/ODPacket.h"
#include "utils/Helper.h"

namespace Skills
{

SkillType fromString(const std::string& type)
{
    for(uint32_t i = 0; i < static_cast<uint32_t>(SkillType::countSkill); ++i)
    {
        SkillType skillType = static_cast<SkillType>(i);
        if(type != toString(skillType))
            continue;

        return skillType;
    }

    return SkillType::nullSkillType;
}

std::string toString(SkillType type)
{
    switch(type)
    {
        case SkillType::nullSkillType:
            return "nullSkillType";
        case SkillType::roomArena:
            return "roomArena";
        case SkillType::roomBridgeStone:
            return "roomBridgeStone";
        case SkillType::roomBridgeWooden:
            return "roomBridgeWooden";
        case SkillType::roomCasino:
            return "roomCasino";
        case SkillType::roomCrypt:
            return "roomCrypt";
        case SkillType::roomDormitory:
            return "roomDormitory";
        case SkillType::roomWorkshop:
            return "roomWorkshop";
        case SkillType::roomHatchery:
            return "roomHatchery";
        case SkillType::roomLibrary:
            return "roomLibrary";
        case SkillType::roomPrison:
            return "roomPrison";
        case SkillType::roomTorture:
            return "roomTorture";
        case SkillType::roomTrainingHall:
            return "roomTrainingHall";
        case SkillType::roomTreasury:
            return "roomTreasury";
        case SkillType::spellCallToWar:
            return "spellCallToWar";
        case SkillType::spellSummonWorker:
            return "spellSummonWorker";
        case SkillType::spellCreatureDefense:
            return "spellCreatureDefense";
        case SkillType::spellCreatureExplosion:
            return "spellCreatureExplosion";
        case SkillType::spellCreatureHaste:
            return "spellCreatureHaste";
        case SkillType::spellCreatureHeal:
            return "spellCreatureHeal";
        case SkillType::spellCreatureSlow:
            return "spellCreatureSlow";
        case SkillType::spellCreatureStrength:
            return "spellCreatureStrength";
        case SkillType::spellCreatureWeak:
            return "spellCreatureWeak";
        case SkillType::trapBoulder:
            return "trapBoulder";
        case SkillType::trapCannon:
            return "trapCannon";
        case SkillType::trapSpike:
            return "trapSpike";
        case SkillType::trapDoorWooden:
            return "trapDoorWooden";
        default:
            return "Unknown enum value:" + Helper::toString(static_cast<int>(type));
    }
}

std::string skillTypeToPlayerVisibleString(SkillType type)
{
    switch(type)
    {
        case SkillType::nullSkillType:
            return "No Skill type";
        case SkillType::roomArena:
            return "The Arena Room";
        case SkillType::roomBridgeStone:
            return "The Stone Bridge Room";
        case SkillType::roomBridgeWooden:
            return "The Wooden Bridge Room";
        case SkillType::roomCasino:
            return "The Casino Room";
        case SkillType::roomCrypt:
            return "The Crypt Room";
        case SkillType::roomDormitory:
            return "The Dormitory Room";
        case SkillType::roomWorkshop:
            return "The Workshop Room";
        case SkillType::roomHatchery:
            return "The Hatchery Room";
        case SkillType::roomLibrary:
            return "The Library Room";
        case SkillType::roomTrainingHall:
            return "The TrainingHall Room";
        case SkillType::roomTreasury:
            return "The Treasury Room";
        case SkillType::roomPrison:
            return "The Prison Room";
        case SkillType::roomTorture:
            return "The Torture Room";
        case SkillType::spellCallToWar:
            return "The 'Call to War' Spell";
        case SkillType::spellSummonWorker:
            return "The 'Summon Worker' Spell";
        case SkillType::spellCreatureHeal:
            return "The 'Heal' Spell";
        case SkillType::spellCreatureDefense:
            return "The 'Defense' Spell";
        case SkillType::spellCreatureExplosion:
            return "The 'Explosion' Spell";
        case SkillType::spellCreatureHaste:
            return "The 'Haste' Spell";
        case SkillType::spellCreatureSlow:
            return "The 'Slow' Spell";
        case SkillType::spellCreatureStrength:
            return "The 'Strength' Spell";
        case SkillType::spellCreatureWeak:
            return "The 'Weak' Spell";
        case SkillType::trapBoulder:
            return "The Boulder Trap";
        case SkillType::trapCannon:
            return "The Cannon Trap";
        case SkillType::trapSpike:
            return "The Spike Trap";
        case SkillType::trapDoorWooden:
            return "The Wooden Door";
        default:
            return "Unknown enum value:" + Helper::toString(static_cast<int>(type));
    }
}
}

ODPacket& operator<<(ODPacket& os, const SkillType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, SkillType& type)
{
    int32_t tmp;
    is >> tmp;
    type = static_cast<SkillType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const SkillType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, SkillType& type)
{
    int32_t tmp;
    is >> tmp;
    type = static_cast<SkillType>(tmp);
    return is;
}
