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

#include "renderscene/RenderSceneGroup.h"

#include "renderscene/RenderScene.h"
#include "renderscene/RenderSceneManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

RenderSceneGroup::RenderSceneGroup() :
    mIsRepeat(false),
    mIndexSceneRepeat(0),
    mIndexScene(0)
{
}

RenderSceneGroup::~RenderSceneGroup()
{
    for(RenderScene* scene : mScenes)
        delete scene;

    mScenes.clear();
}

RenderSceneGroup* RenderSceneGroup::load(std::istream& defFile)
{
    RenderSceneGroup* group = new RenderSceneGroup;
    std::string nextParam;
    if(!Helper::readNextLineNotEmpty(defFile, nextParam))
    {
        OD_LOG_WRN("Couldn't read new line");
        return group;
    }

    if(nextParam != "[RenderSceneGroup]")
    {
        OD_LOG_WRN("Invalid User configuration start format. Line was " + nextParam);
        return group;
    }

    while(true)
    {
        if(!Helper::readNextLineNotEmpty(defFile, nextParam))
        {
            OD_LOG_WRN("Couldn't read new line");
            return group;
        }

        if(nextParam == "[/RenderSceneGroup]")
            return group;

        if(nextParam == "*Repeat")
        {
            group->mIsRepeat = true;
            group->mIndexSceneRepeat = group->mScenes.size();
            continue;
        }

        std::stringstream ss(nextParam);
        RenderScene* scene = RenderSceneManager::load(ss);
        if(scene == nullptr)
        {
            OD_LOG_WRN("Unexpected null scene for line=" + nextParam);
            continue;
        }
        group->mScenes.push_back(scene);
    }

    return nullptr;
}

void RenderSceneGroup::reset(CameraManager& cameraManager, RenderManager& renderManager)
{
    freeGroup(cameraManager, renderManager);
    if(mIndexScene > 0)
    {
        OD_LOG_ERR("Unexpected non zero index=" + Helper::toString(mIndexScene));
        return;
    }

    if(mIndexScene >= mScenes.size())
        return;

    while(mIndexScene < mScenes.size())
    {
        RenderScene* scene = mScenes.at(mIndexScene);
        if(!scene->activate(cameraManager, renderManager))
            return;

        ++mIndexScene;
    }
}

void RenderSceneGroup::freeGroup(CameraManager& cameraManager, RenderManager& renderManager)
{
    while(mIndexScene > 0)
    {
        --mIndexScene;
        RenderScene* scene = mScenes.at(mIndexScene);
        scene->freeRessources(cameraManager, renderManager);
    }
}

void RenderSceneGroup::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    if(mIndexScene >= mScenes.size())
    {
        if(!mIsRepeat)
            return;

        mIndexScene = mIndexSceneRepeat;
        while(mIndexScene < mScenes.size())
        {
            RenderScene* scene = mScenes.at(mIndexScene);
            if(!scene->activate(cameraManager, renderManager))
                return;

            ++mIndexScene;
        }

        return;
    }

    RenderScene* scene = mScenes.at(mIndexScene);
    if(!scene->update(cameraManager, renderManager, timeSinceLastFrame))
        return;

    ++mIndexScene;
    while(mIndexScene < mScenes.size())
    {
        scene = mScenes.at(mIndexScene);
        if(!scene->activate(cameraManager, renderManager))
            return;

        ++mIndexScene;
    }
}

void RenderSceneGroup::setRenderSceneListener(RenderSceneListener* listener)
{
    for(RenderScene* scene : mScenes)
        scene->setRenderSceneListener(listener);
}

void RenderSceneGroup::notifySyncPost(const std::string& event)
{
    for(RenderScene* scene : mScenes)
        scene->notifySyncPost(event);
}
