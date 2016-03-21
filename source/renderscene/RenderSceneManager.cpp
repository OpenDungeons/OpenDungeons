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

#include "renderscene/RenderSceneManager.h"

#include "renderscene/RenderScene.h"
#include "utils/LogManager.h"

#include <istream>
#include <vector>

namespace
{
    static std::vector<const RenderSceneFactory*>& getFactories()
    {
        static std::vector<const RenderSceneFactory*> factory;
        return factory;
    }
}

void RenderSceneManager::registerFactory(const RenderSceneFactory* factory)
{
    std::vector<const RenderSceneFactory*>& factories = getFactories();
    factories.push_back(factory);
}

void RenderSceneManager::unregisterFactory(const RenderSceneFactory* factory)
{
    std::vector<const RenderSceneFactory*>& factories = getFactories();
    auto it = std::find(factories.begin(), factories.end(), factory);
    if(it == factories.end())
    {
        OD_LOG_ERR("Trying to unregister unknown factory=" + factory->getRenderSceneName());
        return;
    }
    factories.erase(it);
}

RenderScene* RenderSceneManager::load(std::istream& def)
{
    if(!def.good())
        return nullptr;

    std::vector<const RenderSceneFactory*>& factories = getFactories();
    std::string nextParam;
    OD_ASSERT_TRUE(def >> nextParam);
    const RenderSceneFactory* factoryToUse = nullptr;
    for(const RenderSceneFactory* factory : factories)
    {
        if(factory == nullptr)
            continue;

        if(factory->getRenderSceneName().compare(nextParam) != 0)
            continue;

        factoryToUse = factory;
        break;
    }

    if(factoryToUse == nullptr)
    {
        OD_LOG_ERR("Unknown scene modifier=" + nextParam);
        return nullptr;
    }

    RenderScene* scene = factoryToUse->createRenderScene();
    if(!scene->importFromStream(def))
    {
        OD_LOG_ERR("Couldn't load creature scene modifier=" + nextParam);
        delete scene;
        return nullptr;
    }

    return scene;
}

void RenderSceneManager::dispose(const RenderScene* scene)
{
    delete scene;
}
