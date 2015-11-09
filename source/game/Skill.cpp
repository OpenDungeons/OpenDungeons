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

#include "game/Skill.h"

#include "game/Player.h"
#include "game/SkillType.h"
#include "network/ODPacket.h"
#include "utils/Helper.h"


Skill::Skill(SkillType type, int32_t neededSkillPoints, const std::vector<const Skill*>& skillDepends):
    mType(type),
    mNeededSkillPoints(neededSkillPoints),
    mSkillDepends(skillDepends)
{
}

bool Skill::canBeSkilled(const std::vector<SkillType>& skillsDone) const
{
    for(const Skill* skill : mSkillDepends)
    {
        if(std::find(skillsDone.begin(), skillsDone.end(), skill->getType()) != skillsDone.end())
            continue;

        return false;
    }
    return true;
}

void Skill::buildDependencies(const std::vector<SkillType>& skillsDone, std::vector<SkillType>& dependencies) const
{
    // If the current skill is already in the dependencies list, no need to process it
    if(std::find(dependencies.begin(), dependencies.end(), getType()) != dependencies.end())
        return;

    for(const Skill* skill : mSkillDepends)
    {
        SkillType resType = skill->getType();
        if(std::find(skillsDone.begin(), skillsDone.end(), resType) != skillsDone.end())
            continue;

        if(std::find(dependencies.begin(), dependencies.end(), resType) != dependencies.end())
            continue;

        skill->buildDependencies(skillsDone, dependencies);
    }

    if(std::find(dependencies.begin(), dependencies.end(), getType()) == dependencies.end())
        dependencies.push_back(getType());
}

bool Skill::dependsOn(const std::vector<SkillType>& skills) const
{
    for(SkillType resType : skills)
    {
        if(dependsOn(resType))
            return true;
    }
    return false;
}

bool Skill::dependsOn(SkillType type) const
{
    if(getType() == type)
        return true;

    for(const Skill* skill : mSkillDepends)
    {
        if(skill->getType() == type)
            return true;

        if(skill->dependsOn(type))
            return true;
    }

    return false;
}
