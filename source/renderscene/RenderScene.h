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

#ifndef RENDERSCENE_H
#define RENDERSCENE_H

#include <OgrePrerequisites.h>
#include <OgreVector3.h>

#include <cstdint>
#include <iosfwd>
#include <string>

class CameraManager;
class RenderManager;

class RenderScene
{
public:
    // Constructors
    RenderScene()
    {}

    virtual ~RenderScene()
    {}

    //! \brief Function called when the RenderScene is activated.If it returns true, the next RenderScene
    //! of the RenderScenePart will be executed.(activated). Otherwise,
    //! this RenderScene will continue to be updated.
    virtual bool activate(CameraManager& cameraManager, RenderManager& renderManager) = 0;

    //! \brief Function called for each new frame. If it returns true, the next RenderScene
    //! of the RenderScenePart will be executed.(activated). Otherwise,
    //! this RenderScene will continue to be updated.
    virtual bool update(CameraManager& cameraManager, RenderManager& renderManager,
        Ogre::Real timeSinceLastFrame)
    { return true; }

    //! \brief Function called when the scene should not be displayed anymore. It should deallocate
    //! every used ressource. There should be no call to update after calling freeRessources
    //! Note that this function will only be called when the scene is destroyed
    //! Note that calling freeRessources on a non activated scene or calling it several times
    //! should have no effect
    virtual void freeRessources(CameraManager& cameraManager, RenderManager& renderManager) = 0;

    //! \brief Returns the name of the RenderScene (must be unique amongst the RenderScenes)
    virtual const std::string& getModifierName() const = 0;

    //! \brief Can be overriden to read additional parameters from the stream
    virtual bool importFromStream(std::istream& file)
    { return true; }
};

#endif // RENDERSCENE_H
