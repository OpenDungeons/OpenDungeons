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

#ifndef RENDERSCENEGROUP_H
#define RENDERSCENEGROUP_H

#include <OgrePrerequisites.h>
#include <OgreVector3.h>

#include <cstdint>
#include <iosfwd>
#include <string>

class CameraManager;
class RenderManager;
class RenderScene;
class RenderSceneListener;

class RenderSceneGroup
{
public:
    // Constructors
    RenderSceneGroup();
    virtual ~RenderSceneGroup();

    //! \brief Called when the animation should be restarted. It will call freeGroup
    //! and then restart
    void reset(CameraManager& cameraManager, RenderManager& renderManager);

    //! \brief Called when the animation should not be displayed anymore.
    //! It will free the ressources starting from the current scene in the group
    //! until the first
    void freeGroup(CameraManager& cameraManager, RenderManager& renderManager);

    //! \brief Called at each frame
    void update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame);

    void setRenderSceneListener(RenderSceneListener* listener);

    //! \brief Called when a sync post event occurs. It should notify every current
    //! render scene.
    void notifySyncPost(const std::string& event);

    static RenderSceneGroup* load(std::istream& defFile);

private:
    std::vector<RenderScene*> mScenes;
    bool mIsRepeat;
    uint32_t mIndexSceneRepeat;
    uint32_t mIndexScene;
};

#endif // RENDERSCENEGROUP_H
