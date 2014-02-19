/*!
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

#ifndef ABSTRACTGAMESTATE_H
#define ABSTRACTGAMESTATE_H

#include <OIS/OISMouse.h>

class GameStateManager;
namespace Ogre { struct FrameEvent; }
namespace OIS { class KeyEvent; }

class AbstractGameState
{

public:
    AbstractGameState(GameStateManager& gameStateManager):
        mGameStateManager(gameStateManager),
        mParentState(NULL)
    {}

    virtual ~AbstractGameState()
    {}

    virtual bool frameStarted   (const Ogre::FrameEvent& evt) = 0;
    virtual bool mouseMoved     (const OIS::MouseEvent &arg) = 0;
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) = 0;
    virtual bool keyPressed     (const OIS::KeyEvent &arg) = 0;
    virtual bool keyReleased    (const OIS::KeyEvent &arg) = 0;

protected:
    inline GameStateManager& getGameStateManager()
    { return mGameStateManager; }

private:
    GameStateManager& mGameStateManager;
    AbstractGameState* mParentState;
};

#endif // ABSTRACTGAMESTATE_H
