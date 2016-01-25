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

#include "renderscene/RenderSceneAddParticleEffect.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"
#include "utils/Helper.h"

static const std::string RenderSceneAddParticleEffectName = "AddParticleEffect";

namespace
{
class RenderSceneAddParticleEffectFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneAddParticleEffect; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneAddParticleEffectName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneAddParticleEffectFactory);
}

uint32_t RenderSceneAddParticleEffect::mParticleUniqueNumber = 0;

const std::string& RenderSceneAddParticleEffect::getModifierName() const
{
    return RenderSceneAddParticleEffectName;
}

bool RenderSceneAddParticleEffect::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    if(mParticleSystem != nullptr)
        return true;

    Ogre::Vector3 pos;
    mSceneNode = renderManager.getMenuEntityNode(mName, pos);
    std::string particleName = mName + "-" + mParticleScript + Helper::toString(mParticleUniqueNumber);
    ++mParticleUniqueNumber;
    mParticleSystem = renderManager.addEntityParticleEffectMenu(mSceneNode, particleName, mParticleScript);
    return true;
}

void RenderSceneAddParticleEffect::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    renderManager.removeEntityParticleEffectMenu(mSceneNode, mParticleSystem);
    mSceneNode = nullptr;
    mParticleSystem = nullptr;
}

bool RenderSceneAddParticleEffect::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mParticleScript))
        return false;

    return true;
}
