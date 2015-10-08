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

#include "creaturebehaviour/CreatureBehaviourManager.h"

#include "creaturebehaviour/CreatureBehaviour.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <vector>

namespace
{
    static std::vector<const CreatureBehaviourFactory*>& getFactories()
    {
        static std::vector<const CreatureBehaviourFactory*> factory;
        return factory;
    }
}

void CreatureBehaviourManager::registerFactory(const CreatureBehaviourFactory* factory)
{
    std::vector<const CreatureBehaviourFactory*>& factories = getFactories();
    factories.push_back(factory);
}

void CreatureBehaviourManager::unregisterFactory(const CreatureBehaviourFactory* factory)
{
    std::vector<const CreatureBehaviourFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getCreatureBehaviourName());
        return;
    }
    factories.erase(it);
}

CreatureBehaviour* CreatureBehaviourManager::clone(const CreatureBehaviour* behaviour)
{
    return behaviour->clone();
}

CreatureBehaviour* CreatureBehaviourManager::load(std::istream& is)
{
    if(!is.good())
        return nullptr;

    std::vector<const CreatureBehaviourFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(is >> nextParam);
    const CreatureBehaviourFactory* factoryToUse = nullptr;
    for(const CreatureBehaviourFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getCreatureBehaviourName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown behaviour modifier=" + nextParam);
        return nullptr;
    }

    CreatureBehaviour* behaviour = factoryToUse->createCreatureBehaviour();
    if(!behaviour->importFromStream(is))
    {
        OD_LOG_ERR("Couldn't load creature behaviour modifier=" + nextParam);
        delete behaviour;
        return nullptr;
    }

    return behaviour;
}

void CreatureBehaviourManager::dispose(const CreatureBehaviour* behaviour)
{
    delete behaviour;
}

void CreatureBehaviourManager::write(const CreatureBehaviour& behaviour, std::ostream& os)
{
    os << behaviour.getName();
    behaviour.exportToStream(os);
    os << std::endl;
}

void CreatureBehaviourManager::getFormatString(const CreatureBehaviour& behaviour, std::string& format)
{
    format = "# BehaviourName";
    behaviour.getFormatString(format);
}

bool CreatureBehaviourManager::areEqual(const CreatureBehaviour& behaviour1, const CreatureBehaviour& behaviour2)
{
    return behaviour1.isEqual(behaviour2);
}
