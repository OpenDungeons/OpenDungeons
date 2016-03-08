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

#ifndef RENDERSCENESYNCWAITANIMATION_H
#define RENDERSCENESYNCWAITANIMATION_H

#include "renderscene/RenderScene.h"

class RenderSceneSyncWaitAnimation : public RenderScene
{
public:
    RenderSceneSyncWaitAnimation() :
        mLenght(0),
        mAnimState(nullptr),
        mIsWaiting(true)
    {}

    virtual ~RenderSceneSyncWaitAnimation()
    {}

    const std::string& getModifierName() const override;

    bool activate(CameraManager& cameraManager, RenderManager& renderManager) override;
    bool update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame) override;
    void freeRessources(CameraManager& cameraManager, RenderManager& renderManager) override;

    void notifySyncPost(const std::string& event) override;

    bool importFromStream(std::istream& is) override;

private:
    std::string mName;
    std::string mAnimation;
    std::string mEvent;

    // Temporary ressources
    Ogre::Real mLenght;
    Ogre::AnimationState* mAnimState;
    bool mIsWaiting;
};

#endif // RENDERSCENESYNCWAITANIMATION_H
