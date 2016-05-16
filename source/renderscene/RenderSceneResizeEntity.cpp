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

#include "renderscene/RenderSceneResizeEntity.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneSetAnimationName = "ResizeEntity";

namespace
{
class RenderSceneSetAnimationFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneResizeEntity; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneSetAnimationName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneSetAnimationFactory);
}

const std::string& RenderSceneResizeEntity::getModifierName() const
{
    return RenderSceneSetAnimationName;
}

bool RenderSceneResizeEntity::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mTime = 0;
    mSceneNode = renderManager.getMenuEntityNode(mName);
    if(mSceneNode == nullptr)
        return true;

    if(mTotalTime <= 0)
    {
        renderManager.setScaleMenuEntity(mSceneNode, mFinalSize);
        return true;
    }

    return false;
}

bool RenderSceneResizeEntity::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    mTime += timeSinceLastFrame;
    if(mTime >= mTotalTime)
    {
        renderManager.setScaleMenuEntity(mSceneNode, mFinalSize);
        return true;
    }

    // We compute the new scale
    // Scale = prevScale + (finalScale - prevScale) * coef
    Ogre::Real coef = timeSinceLastFrame / (mTotalTime - mTime);
    const Ogre::Vector3& entityScale = renderManager.getMenuEntityScale(mSceneNode);
    Ogre::Vector3 scale = entityScale;
    scale += ((mFinalSize - entityScale) * coef);
    renderManager.setScaleMenuEntity(mSceneNode, scale);
    return false;
}

void RenderSceneResizeEntity::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    mSceneNode = nullptr;
    mTime = 0;
}

bool RenderSceneResizeEntity::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    Ogre::Real finalSize;
    if(!(is >> finalSize))
        return false;
    mFinalSize = Ogre::Vector3::UNIT_SCALE * finalSize;
    if(!(is >> mTotalTime))
        return false;

    return true;
}
