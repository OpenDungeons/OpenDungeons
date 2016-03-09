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

#include "renderscene/RenderSceneSyncWait.h"

#include "camera/CameraManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSyncWaitName = "SyncWait";

namespace
{
class RenderSceneSyncWaitFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneSyncWait; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSyncWaitName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSyncWaitFactory);
}

const std::string& RenderSceneSyncWait::getModifierName() const
{
    return RenderSceneSyncWaitName;
}

bool RenderSceneSyncWait::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mIsWaiting = true;
    return false;
}

bool RenderSceneSyncWait::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    if(mIsWaiting)
        return false;

    mIsWaiting = true;
    return true;
}

void RenderSceneSyncWait::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    // Nothing to do. We don't care about setting the camera back to initial state
}

void RenderSceneSyncWait::notifySyncPost(const std::string& event)
{
    if(event.compare(mEvent) != 0)
        return;

    mIsWaiting = false;
}

bool RenderSceneSyncWait::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mEvent))
        return false;

    return true;
}
