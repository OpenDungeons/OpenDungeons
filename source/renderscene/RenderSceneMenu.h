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

#ifndef RENDERSCENEMENU_H
#define RENDERSCENEMENU_H

class CameraManager;
class RenderManager;
class RenderSceneGroup;

#include "renderscene/RenderScene.h"

#include <OgrePrerequisites.h>

#include <string>

//! \brief This class is basically used to store the needed scenes to display a menu
class RenderSceneMenu : public RenderSceneListener
{
public:
    // Constructors
    RenderSceneMenu();
    virtual ~RenderSceneMenu();

    void dispatchSyncPost(const std::string& event) override;

    void resetMenu(CameraManager& cameraManager, RenderManager& renderManager);
    void freeMenu(CameraManager& cameraManager, RenderManager& renderManager);
    void updateMenu(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame);
    void readSceneMenu(const std::string& fileName);

private:
    std::vector<RenderSceneGroup*> mSceneGroups;
    RenderSceneListener* mRenderSceneListener;
};

#endif // RENDERSCENEMENU_H
