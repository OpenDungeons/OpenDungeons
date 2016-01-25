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

#include "renderscene/RenderSceneAnimationOnce.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"
#include "utils/LogManager.h"

static const std::string RenderSceneSetAnimationName = "AnimationOnce";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneAnimationOnce; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderSceneAnimationOnce::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderSceneAnimationOnce::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mAnimState = renderManager.setMenuEntityAnimation(mName, mAnimation, false);
    if(mAnimState == nullptr)
        return true;

    return false;
}

bool RenderSceneAnimationOnce::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    if(mAnimState == nullptr)
    {
        OD_LOG_WRN("No animation state for entity=" + mName + ", anim=" + mAnimation);
        return true;
    }

    return renderManager.updateMenuEntityAnimation(mAnimState, timeSinceLastFrame);
}

void RenderSceneAnimationOnce::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    // No need to free the animation state. It will be freed by the entity
    mAnimState = nullptr;
}

bool RenderSceneAnimationOnce::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mAnimation))
        return false;

    return true;
}
