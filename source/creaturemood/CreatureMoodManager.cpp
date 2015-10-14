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

#include "creaturemood/CreatureMoodManager.h"

#include "creaturemood/CreatureMood.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <vector>

namespace
{
    static std::vector<const CreatureMoodFactory*>& getFactories()
    {
        static std::vector<const CreatureMoodFactory*> factory;
        return factory;
    }
}

void CreatureMoodManager::registerFactory(const CreatureMoodFactory* factory)
{
    std::vector<const CreatureMoodFactory*>& factories = getFactories();
    factories.push_back(factory);
}

void CreatureMoodManager::unregisterFactory(const CreatureMoodFactory* factory)
{
    std::vector<const CreatureMoodFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getCreatureMoodName());
        return;
    }
    factories.erase(it);
}

CreatureMood* CreatureMoodManager::load(std::istream& defFile)
{
    if(!defFile.good())
        return nullptr;

    std::vector<const CreatureMoodFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(defFile >> nextParam);
    const CreatureMoodFactory* factoryToUse = nullptr;
    for(const CreatureMoodFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getCreatureMoodName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown mood modifier=" + nextParam);
        return nullptr;
    }

    CreatureMood* mood = factoryToUse->createCreatureMood();
    if(!mood->importFromStream(defFile))
    {
        OD_LOG_ERR("Couldn't load creature mood modifier=" + nextParam);
        delete mood;
        return nullptr;
    }

    return mood;
}
