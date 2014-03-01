/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef EDITORCONTEXT_H
#define EDITORCONTEXT_H

#include <boost/shared_ptr.hpp>

#include <OgreTimer.h>

class GameMap;
class CameraManager;
class ModeManager;
class LogManager;
class RenderManager;

namespace Ogre
{
  class SceneNode;
  class RenderWindow;
}

class EditorContext
{
public:
    EditorContext(Ogre::RenderWindow*, ModeManager*, GameMap*);
    ~EditorContext();

    inline GameMap* getGameMap()
    {
        return mGameMap;
    }

    void onFrameStarted(const Ogre::FrameEvent& evt);
    void onFrameEnded(const Ogre::FrameEvent& evt);
    void setCameraManager(CameraManager*);

private:
    GameMap* mGameMap;
    //TODO: Un-singleton this class and make it a stack object.
    RenderManager*          mRenderManager;
    CameraManager*          mCameraManager;
    ModeManager*            mModeManager;
    LogManager*             mLogManager;
    long int                mPreviousTurn;
    Ogre::SceneNode*        mCreatureSceneNode;
    Ogre::SceneNode*        mRoomSceneNode;
    Ogre::SceneNode*        mFieldSceneNode;
    Ogre::SceneNode*        mLightSceneNode;
    Ogre::Timer             mStatsDisplayTimer;
};

#endif // EDITORCONTEXT_H
