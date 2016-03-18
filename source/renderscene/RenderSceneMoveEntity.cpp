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

#include "renderscene/RenderSceneMoveEntity.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSetAnimationName = "MoveEntity";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneMoveEntity; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderSceneMoveEntity::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderSceneMoveEntity::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mSceneNode = renderManager.getMenuEntityNode(mName, mPosition);
    mAnimState = renderManager.setMenuEntityAnimation(mName, mAnimation, true);
    if((mSceneNode == nullptr) || (mAnimState == nullptr))
        return true;

    // We orient the entity to its destination
    Ogre::Vector3 walkDirection = mDestination - mPosition;
    walkDirection.normalise();
    renderManager.orientMenuEntityPosition(mSceneNode, walkDirection);
    return false;
}

bool RenderSceneMoveEntity::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    renderManager.updateMenuEntityAnimation(mAnimState, timeSinceLastFrame);
    // Computes new position
    Ogre::Vector3 walkDirection = mDestination - mPosition;
    walkDirection.normalise();
    Ogre::Real moveDist = mSpeed * timeSinceLastFrame;
    Ogre::Real distToDest = mPosition.distance(mDestination);
    if(distToDest > moveDist)
    {
        mPosition = mPosition + walkDirection * moveDist;
        renderManager.updateMenuEntityPosition(mSceneNode, mPosition);
        return false;
    }

    mPosition = mDestination;
    renderManager.updateMenuEntityPosition(mSceneNode, mPosition);

    return true;
}

void RenderSceneMoveEntity::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    // No need to free the animation state. It will be freed by the entity
    mSceneNode = nullptr;
    mAnimState = nullptr;
}

bool RenderSceneMoveEntity::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mAnimation))
        return false;
    if(!(is >> mSpeed))
        return false;
    if(!(is >> mDestination.x))
        return false;
    if(!(is >> mDestination.y))
        return false;
    if(!(is >> mDestination.z))
        return false;

    return true;
}
