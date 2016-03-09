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

#include "renderscene/RenderSceneSyncPost.h"

#include "camera/CameraManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSyncPostName = "SyncPost";

namespace
{
class RenderSceneSyncPostFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneSyncPost; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSyncPostName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSyncPostFactory);
}

const std::string& RenderSceneSyncPost::getModifierName() const
{
    return RenderSceneSyncPostName;
}

bool RenderSceneSyncPost::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    fireSyncPost(mEvent);
    return true;
}

void RenderSceneSyncPost::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    // Nothing to do. We don't care about setting the camera back to initial state
}

bool RenderSceneSyncPost::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mEvent))
        return false;

    return true;
}
