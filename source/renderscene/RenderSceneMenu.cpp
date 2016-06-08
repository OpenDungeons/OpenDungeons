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

#include "renderscene/RenderSceneMenu.h"

#include "renderscene/RenderScene.h"
#include "renderscene/RenderSceneGroup.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

RenderSceneMenu::RenderSceneMenu()
{
}

RenderSceneMenu::~RenderSceneMenu()
{
    for(RenderSceneGroup* sceneGroup : mSceneGroups)
        delete sceneGroup;

    mSceneGroups.clear();
}

void RenderSceneMenu::dispatchSyncPost(const std::string& event)
{
    for(RenderSceneGroup* sceneGroup : mSceneGroups)
        sceneGroup->notifySyncPost(event);
}

void RenderSceneMenu::resetMenu(CameraManager& cameraManager, RenderManager& renderManager)
{
    for(RenderSceneGroup* sceneGroup : mSceneGroups)
        sceneGroup->reset(cameraManager, renderManager);
}

void RenderSceneMenu::freeMenu(CameraManager& cameraManager, RenderManager& renderManager)
{
    for(RenderSceneGroup* sceneGroup : mSceneGroups)
        sceneGroup->freeGroup(cameraManager, renderManager);
}

void RenderSceneMenu::updateMenu(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    for(RenderSceneGroup* sceneGroup : mSceneGroups)
        sceneGroup->update(cameraManager, renderManager, timeSinceLastFrame);
}

void RenderSceneMenu::readSceneMenu(const std::string& fileName)
{
    if(!mSceneGroups.empty())
    {
        OD_LOG_ERR("Scene already loaded. Cannot read " + fileName);
        return;
    }

    std::stringstream defFile;
    if(!Helper::readFileWithoutComments(fileName, defFile))
    {
        OD_LOG_ERR("Couldn't read " + fileName);
        return;
    }

    std::string nextParam;
    while(true)
    {
        if(!defFile.good())
            break;

        RenderSceneGroup* group = RenderSceneGroup::load(defFile);
        if(group == nullptr)
            return;

        group->setRenderSceneListener(this);
        mSceneGroups.emplace_back(group);
    }
}
