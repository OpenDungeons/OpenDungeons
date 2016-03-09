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

#ifndef RENDERSCENEADDPARTICLEEFFECTBONE_H
#define RENDERSCENEADDPARTICLEEFFECTBONE_H

#include "renderscene/RenderScene.h"

namespace Ogre
{
class SceneNode;
class ParticleSystem;
}

class RenderSceneAddParticleEffectBone : public RenderScene
{
public:
    RenderSceneAddParticleEffectBone() :
        mParticleSystem(nullptr)
    {}

    virtual ~RenderSceneAddParticleEffectBone()
    {}

    const std::string& getModifierName() const override;

    bool activate(CameraManager& cameraManager, RenderManager& renderManager) override;

    void freeRessources(CameraManager& cameraManager, RenderManager& renderManager) override;

    bool importFromStream(std::istream& is) override;

private:
    static uint32_t mParticleUniqueNumber;
    std::string mName;
    std::string mParticleScript;
    std::string mBoneName;

    // Temporary ressources
    Ogre::ParticleSystem* mParticleSystem;
};

#endif // RENDERSCENEADDPARTICLEEFFECTBONE_H
