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

#include "renderscene/RenderSceneAddEntity.h"

#include "camera/CameraManager.h"
#include "render/RenderManager.h"
#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneAddEntityName = "AddEntity";

namespace
{
class RenderSceneAddEntityFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneAddEntity; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneAddEntityName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneAddEntityFactory);
}

const std::string& RenderSceneAddEntity::getModifierName() const
{
    return RenderSceneAddEntityName;
}

bool RenderSceneAddEntity::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    if(mEntity != nullptr)
        return true;

    mEntity = renderManager.addEntityMenu(mMesh, mName, mScale, mPosition);
    return true;
}

void RenderSceneAddEntity::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    renderManager.removeEntityMenu(mEntity);
    mEntity = nullptr;
}

bool RenderSceneAddEntity::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mName))
        return false;
    if(!(is >> mMesh))
        return false;
    if(!(is >> mScale.x))
        return false;
    if(!(is >> mScale.y))
        return false;
    if(!(is >> mScale.z))
        return false;
    if(!(is >> mPosition.x))
        return false;
    if(!(is >> mPosition.y))
        return false;
    if(!(is >> mPosition.z))
        return false;

    return true;
}
