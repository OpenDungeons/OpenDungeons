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

#include "renderscene/RenderSceneCameraMove.h"

#include "camera/CameraManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneCameraMoveName = "CameraMove";

namespace
{
class RenderSceneCameraMoveFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneCameraMove; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneCameraMoveName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneCameraMoveFactory);
}

const std::string& RenderSceneCameraMove::getModifierName() const
{
    return RenderSceneCameraMoveName;
}

bool RenderSceneCameraMove::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    cameraManager.resetCamera(mPosition, mRotation);
    return true;
}

void RenderSceneCameraMove::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    // Nothing to do. We don't care about setting the camera back to initial state
}

bool RenderSceneCameraMove::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mPosition.x))
        return false;
    if(!(is >> mPosition.y))
        return false;
    if(!(is >> mPosition.z))
        return false;
    if(!(is >> mRotation.x))
        return false;
    if(!(is >> mRotation.y))
        return false;
    if(!(is >> mRotation.z))
        return false;

    return true;
}
