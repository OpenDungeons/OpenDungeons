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

#include "renderscene/RenderSceneAddPointLight.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneAddPointLightName = "AddPointLight";

namespace
{
class RenderSceneAddPointLightFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneAddPointLight; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneAddPointLightName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneAddPointLightFactory);
}

const std::string& RenderSceneAddPointLight::getModifierName() const
{
    return RenderSceneAddPointLightName;
}

bool RenderSceneAddPointLight::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    if(mLight != nullptr)
        return true;

    mLight = renderManager.addPointLightMenu(mName, mPosition, mDiffuse, mSpecular, mAttenuationRange,
        mAttenuationConstant, mAttenuationLinear, mAttenuationQuadratic);
    return true;
}

void RenderSceneAddPointLight::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    renderManager.removePointLightMenu(mLight);
    mLight = nullptr;
}

bool RenderSceneAddPointLight::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mPosition.x))
        return false;
    if(!(is >> mPosition.y))
        return false;
    if(!(is >> mPosition.z))
        return false;
    mDiffuse.a = 1;
    if(!(is >> mDiffuse.r))
        return false;
    if(!(is >> mDiffuse.g))
        return false;
    if(!(is >> mDiffuse.b))
        return false;
    mSpecular.a = 1;
    if(!(is >> mSpecular.r))
        return false;
    if(!(is >> mSpecular.g))
        return false;
    if(!(is >> mSpecular.b))
        return false;
    if(!(is >> mAttenuationRange))
        return false;
    if(!(is >> mAttenuationConstant))
        return false;
    if(!(is >> mAttenuationLinear))
        return false;
    if(!(is >> mAttenuationQuadratic))
        return false;

    return true;
}
