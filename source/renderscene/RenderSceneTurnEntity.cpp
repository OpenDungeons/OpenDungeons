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

#include "renderscene/RenderSceneTurnEntity.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSetAnimationName = "TurnEntity";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneTurnEntity; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderSceneTurnEntity::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderSceneTurnEntity::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    Ogre::Vector3 pos;
    mSceneNode = renderManager.getMenuEntityNode(mName, pos);
    if(mSceneNode == nullptr)
        return true;

    if(mTimeToTurn <= 0)
    {
        renderManager.orientMenuEntityPosition(mSceneNode, mDirectionFinal);
        return true;
    }

    if(mAnimation.compare("none") == 0)
        mAnimState = nullptr;
    else
        mAnimState = renderManager.setMenuEntityAnimation(mName, mAnimation, true);

    mAngleSrc = renderManager.getNodeOrientation(mSceneNode);
    Ogre::Vector3 direction = mAngleSrc * Ogre::Vector3::NEGATIVE_UNIT_Y;
    mAngleDest = direction.getRotationTo(mDirectionFinal) * mAngleSrc;
    mTime = 0;
    return false;
}

bool RenderSceneTurnEntity::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    mTime += timeSinceLastFrame;
    if(mTime >= mTimeToTurn)
    {
        renderManager.orientMenuEntityPosition(mSceneNode, mDirectionFinal);
        return true;
    }

    if(mAnimState != nullptr)
        renderManager.updateMenuEntityAnimation(mAnimState, timeSinceLastFrame);

    Ogre::Real progress = mTime / mTimeToTurn;
    renderManager.setProgressiveNodeOrientation(mSceneNode, progress, mAngleSrc, mAngleDest);
    return false;
}

void RenderSceneTurnEntity::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    mSceneNode = nullptr;
    mAnimState = nullptr;
}

bool RenderSceneTurnEntity::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mAnimation))
        return false;
    if(!(is >> mDirectionFinal.x))
        return false;
    if(!(is >> mDirectionFinal.y))
        return false;
    if(!(is >> mDirectionFinal.z))
        return false;
    if(!(is >> mTimeToTurn))
        return false;

    return true;
}
