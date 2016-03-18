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

#ifndef RENDERSCENETURNENTITY_H
#define RENDERSCENETURNENTITY_H

#include "renderscene/RenderScene.h"

class RenderSceneTurnEntity : public RenderScene
{
public:
    RenderSceneTurnEntity() :
        mDirectionFinal(Ogre::Vector3::ZERO),
        mTimeToTurn(0),
        mSceneNode(nullptr),
        mAnimState(nullptr),
        mTime(0),
        mAngleSrc(Ogre::Quaternion::IDENTITY),
        mAngleDest(Ogre::Quaternion::IDENTITY)
    {}

    virtual ~RenderSceneTurnEntity()
    {}

    const std::string& getModifierName() const override;

    bool activate(CameraManager& cameraManager, RenderManager& renderManager) override;
    bool update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame) override;
    void freeRessources(CameraManager& cameraManager, RenderManager& renderManager) override;

    bool importFromStream(std::istream& is) override;

private:
    std::string mName;
    std::string mAnimation;
    Ogre::Vector3 mDirectionFinal;
    Ogre::Real mTimeToTurn;

    // Temporary ressources
    Ogre::SceneNode* mSceneNode;
    Ogre::AnimationState* mAnimState;
    Ogre::Real mTime;
    Ogre::Quaternion mAngleSrc;
    Ogre::Quaternion mAngleDest;
};

#endif // RENDERSCENETURNENTITY_H
