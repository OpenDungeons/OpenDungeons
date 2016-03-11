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

#include "renderscene/RenderScenePosEntity.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSetAnimationName = "PosEntity";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderScenePosEntity; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderScenePosEntity::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderScenePosEntity::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    Ogre::Vector3 pos;
    mSceneNode = renderManager.getMenuEntityNode(mName, pos);
    if(mSceneNode == nullptr)
        return true;

    renderManager.updateMenuEntityPosition(mSceneNode, mDestination);
    renderManager.orientMenuEntityPosition(mSceneNode, mDirection);
    return true;
}

void RenderScenePosEntity::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    mSceneNode = nullptr;
}

bool RenderScenePosEntity::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mDestination.x))
        return false;
    if(!(is >> mDestination.y))
        return false;
    if(!(is >> mDestination.z))
        return false;
    if(!(is >> mDirection.x))
        return false;
    if(!(is >> mDirection.y))
        return false;
    if(!(is >> mDirection.z))
        return false;

    return true;
}
