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

#include "creatureskill/CreatureSkill.h"

#include "creatureskill/CreatureSkillHealSelf.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>

std::string CreatureSkill::toString(CreatureSkillType type)
{
    switch(type)
    {
        case CreatureSkillType::HealSelf:
            return "HealSelf";
        default:
            OD_LOG_ERR("type=" + Helper::toString(static_cast<int>(type)));
            return "";
    }
}

CreatureSkill* CreatureSkill::load(std::istream& defFile)
{
    if(!defFile.good())
        return nullptr;

    std::string nextParam;
    if(!(defFile >> nextParam))
        return nullptr;

    CreatureSkillType typeCreat = CreatureSkillType::nb;
    for(uint32_t i = 0; i < static_cast<uint32_t>(CreatureSkillType::nb); ++i)
    {
        CreatureSkillType type = static_cast<CreatureSkillType>(i);
        if(nextParam != toString(type))
            continue;

        typeCreat = type;
        break;
    }

    CreatureSkill* skill = nullptr;
    switch(typeCreat)
    {
        case CreatureSkillType::HealSelf:
        {
            skill = new CreatureSkillHealSelf;
            break;
        }
        case CreatureSkillType::nb:
        default:
        {
            break;
        }
    }

    if(skill == nullptr)
    {
        OD_LOG_ERR("Couldn't find creature skill=" + nextParam);
        return nullptr;
    }

    if(!skill->importFromStream(defFile))
    {
        delete skill;
        return nullptr;
    }

    return skill;
}

void CreatureSkill::write(const CreatureSkill* skill, std::ostream& defFile)
{
    skill->exportToStream(defFile);
    defFile << std::endl;
}

void CreatureSkill::getFormatString(std::string& format) const
{
    if(!format.empty())
        format += "\t";

    format += "# SkillName\tCooldownNbTurns\tWarmupNbTurns";
}

void CreatureSkill::exportToStream(std::ostream& os) const
{
    os << toString(getCreatureSkillType());
    os << "\t" << mCooldownNbTurns;
    os << "\t" << mWarmupNbTurns;
}

bool CreatureSkill::importFromStream(std::istream& is)
{
    if(!(is >> mCooldownNbTurns))
        return false;

    if(!(is >> mWarmupNbTurns))
        return false;

    return true;
}

bool CreatureSkill::isEqual(const CreatureSkill& creatureSkill) const
{
    if(typeid(*this) != typeid(creatureSkill))
        return false;

    if(mCooldownNbTurns != creatureSkill.mCooldownNbTurns)
        return false;
    if(mWarmupNbTurns != creatureSkill.mWarmupNbTurns)
        return false;

    return true;
}
