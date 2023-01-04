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

#ifndef RENDERSCENEMANAGER_H
#define RENDERSCENEMANAGER_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <sstream>

class RenderScene;

//! \brief Factory class to register a new render scene
class RenderSceneFactory
{
public:
    virtual ~RenderSceneFactory()
    {}

    virtual RenderScene* createRenderScene() const = 0;

    virtual const std::string& getRenderSceneName() const = 0;
};

class RenderSceneManager
{
friend class RenderSceneRegister;

public:
    RenderSceneManager()
    {}

    virtual ~RenderSceneManager()
    {}

    static RenderScene* load(std::istream& def);

    static void dispose(const RenderScene* scene);

private:
    static void registerFactory(const RenderSceneFactory* factory);
    static void unregisterFactory(const RenderSceneFactory* factory);
};

class RenderSceneRegister
{
public:
    RenderSceneRegister(const RenderSceneFactory* factoryToRegister) :
        mRenderSceneFactory(factoryToRegister)
    {
        RenderSceneManager::registerFactory(mRenderSceneFactory);
    }
    ~RenderSceneRegister()
    {
        RenderSceneManager::unregisterFactory(mRenderSceneFactory);
        delete mRenderSceneFactory;
    }

private:
    const RenderSceneFactory* mRenderSceneFactory;
};

#endif // RENDERSCENEMANAGER_H
