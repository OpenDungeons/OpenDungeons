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

#include "creatureeffect/CreatureEffectManager.h"

#include "creatureeffect/CreatureEffect.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <vector>

namespace
{
    static std::vector<const CreatureEffectFactory*>& getFactories()
    {
        static std::vector<const CreatureEffectFactory*> factory;
        return factory;
    }
}

void CreatureEffectManager::registerFactory(const CreatureEffectFactory* factory)
{
    std::vector<const CreatureEffectFactory*>& factories = getFactories();
    factories.push_back(factory);
}

void CreatureEffectManager::unregisterFactory(const CreatureEffectFactory* factory)
{
    std::vector<const CreatureEffectFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getCreatureEffectName());
        return;
    }
    factories.erase(it);
}

CreatureEffect* CreatureEffectManager::load(std::istream& defFile)
{
    if(!defFile.good())
        return nullptr;

    std::vector<const CreatureEffectFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(defFile >> nextParam);
    const CreatureEffectFactory* factoryToUse = nullptr;
    for(const CreatureEffectFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getCreatureEffectName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown Effect modifier=" + nextParam);
        return nullptr;
    }

    CreatureEffect* effect = factoryToUse->createCreatureEffect();
    if(!effect->importFromStream(defFile))
    {
        OD_LOG_ERR("Couldn't load creature effect modifier=" + nextParam);
        delete effect;
        return nullptr;
    }

    return effect;
}

void CreatureEffectManager::write(const CreatureEffect& effect, std::ostream& os)
{
    os << effect.getCreatureEffectType();
    effect.exportToStream(os);
}
