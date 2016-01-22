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

#include "renderscene/RenderSceneAnimationTime.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSetAnimationName = "AnimationTime";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneAnimationTime; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderSceneAnimationTime::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderSceneAnimationTime::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mAnimState = renderManager.setMenuEntityAnimation(mName, mAnimation, true);
    mLenght = 0;

    return (mTotalLenght == 0);
}

bool RenderSceneAnimationTime::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    if(mTotalLenght >= 0)
    {
        mLenght += timeSinceLastFrame;
        if(mLenght >= mTotalLenght)
            return true;
    }

    renderManager.updateMenuEntityAnimation(mAnimState, timeSinceLastFrame);
    return false;
}

void RenderSceneAnimationTime::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    mLenght = 0;
    // No need to free the animation state. It will be freed by the entity
    mAnimState = nullptr;
}

bool RenderSceneAnimationTime::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mAnimation))
        return false;
    if(!(is >> mTotalLenght))
        return false;

    return true;
}
