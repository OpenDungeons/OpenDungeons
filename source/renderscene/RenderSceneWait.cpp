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

#include "renderscene/RenderSceneWait.h"

#include "renderscene/RenderSceneManager.h"

static const std::string RenderSceneWaitName = "Wait";

namespace
{
class RenderSceneWaitFactory : public RenderSceneFactory
{
    RenderScene* createRenderScene() const override
    { return new RenderSceneWait; }

    const std::string& getRenderSceneName() const override
    {
        return RenderSceneWaitName;
    }
};

// Register the factory
static RenderSceneRegister reg(new RenderSceneWaitFactory);
}

const std::string& RenderSceneWait::getModifierName() const
{
    return RenderSceneWaitName;
}

bool RenderSceneWait::activate(CameraManager& cameraManager, RenderManager& renderManager)
{
    mTimeCurrent = 0;
    if(mTimeWait < 0)
        return true;

    return false;
}

bool RenderSceneWait::update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
{
    mTimeCurrent += timeSinceLastFrame;
    if(mTimeCurrent >= mTimeWait)
        return true;

    return false;
}

void RenderSceneWait::freeRessources(CameraManager& cameraManager, RenderManager& renderManager)
{
    mTimeCurrent = 0;
}

bool RenderSceneWait::importFromStream(std::istream& is)
{
    if(!RenderScene::importFromStream(is))
        return false;

    if(!(is >> mTimeWait))
        return false;

    return true;
}
