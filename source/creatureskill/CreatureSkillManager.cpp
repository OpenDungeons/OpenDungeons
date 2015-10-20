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

#include "creatureskill/CreatureSkillManager.h"

#include "creatureskill/CreatureSkill.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <vector>

namespace
{
    static std::vector<const CreatureSkillFactory*>& getFactories()
    {
        static std::vector<const CreatureSkillFactory*> factory;
        return factory;
    }
}

void CreatureSkillManager::registerFactory(const CreatureSkillFactory* factory)
{
    std::vector<const CreatureSkillFactory*>& factories = getFactories();
    factories.push_back(factory);
}

void CreatureSkillManager::unregisterFactory(const CreatureSkillFactory* factory)
{
    std::vector<const CreatureSkillFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getCreatureSkillName());
        return;
    }
    factories.erase(it);
}

CreatureSkill* CreatureSkillManager::clone(const CreatureSkill* skill)
{
    return skill->clone();
}

CreatureSkill* CreatureSkillManager::load(std::istream& is)
{
    if(!is.good())
        return nullptr;

    std::vector<const CreatureSkillFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(is >> nextParam);
    const CreatureSkillFactory* factoryToUse = nullptr;
    for(const CreatureSkillFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getCreatureSkillName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown Skill=" + nextParam);
        return nullptr;
    }

    CreatureSkill* skill = factoryToUse->createCreatureSkill();
    if(!skill->importFromStream(is))
    {
        OD_LOG_ERR("Couldn't load creature Skill=" + nextParam);
        delete skill;
        return nullptr;
    }

    return skill;
}

void CreatureSkillManager::dispose(const CreatureSkill* skill)
{
    delete skill;
}

void CreatureSkillManager::write(const CreatureSkill& skill, std::ostream& os)
{
    os << skill.getSkillName();
    skill.exportToStream(os);
}

void CreatureSkillManager::getFormatString(const CreatureSkill& skill, std::string& format)
{
    format = "# SkillName";
    skill.getFormatString(format);
}

bool CreatureSkillManager::areEqual(const CreatureSkill& skill1, const CreatureSkill& skill2)
{
    return skill1.isEqual(skill2);
}
