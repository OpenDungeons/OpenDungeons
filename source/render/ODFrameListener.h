/*!
 * \file   ODFrameListener.h
 * \date   09 April 2011
 * \auth	(require 'ecb)or Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 *
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

#ifndef __ODFRAMELISTENER_H__
#define __ODFRAMELISTENER_H__

#include "camera/CameraManager.h"

#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <OgreSingleton.h>
#include <OgreSceneQuery.h>
#include <CEGUI/EventArgs.h>

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OISMouse.h>

#include <deque>
#include <memory>
#include <SFML/System/Clock.hpp>

class ChatMessage;
class GameMap;
class Gui;
class ModeManager;
class RenderManager;

namespace Ogre
{
    class OverlaySystem;
}

/*! \brief The main OGRE rendering class.
 *
 * This class provides the rendering framework for the OGRE subsystem, as well
 * as processing keyboard and mouse input from the user.  It loads and
 * initializes the meshes for creatures and tiles, moves the camera, and
 * displays the terminal and chat messages on the game screen.
 */
class ODFrameListener :
        public Ogre::Singleton<ODFrameListener>,
        public Ogre::FrameListener,
        public Ogre::WindowEventListener
{

friend class ODClient;

public:
    // Constructor takes a RenderWindow because it uses that to determine input context
    ODFrameListener(Ogre::RenderWindow* renderWindow, Ogre::OverlaySystem* overLaySystem, Gui* gui);
    virtual ~ODFrameListener();

    void requestExit();

    inline unsigned int getChatMaxMessages() const
    { return mChatMaxMessages; }

    inline void setChatMaxMessages(unsigned int nM)
    { mChatMaxMessages = nM; }

    inline float getChatMaxTimeDisplay() const
    { return mChatMaxTimeDisplay; }

    inline void setChatMaxTimeDisplay(float chatMaxTimeDisplay)
    { mChatMaxTimeDisplay = chatMaxTimeDisplay; }

    //! \brief Toggle the display of debug info (default key used to do that: F11)
    void toggleDebugInfo()
    { mShowDebugInfo = !mShowDebugInfo; }

    inline Ogre::RaySceneQuery* getRaySceneQuery()
    { return mRaySceneQuery; }

    //! \brief Adjust mouse clipping area
    virtual void windowResized(Ogre::RenderWindow* rw);

    /*! \brief The main function for the OGRE 3d environment.
     *
     * This function is triggered by Ogre 3D once all the rendering has been bound to GPU,
     * giving time to the CPU to handle updates. Hence, we're placing our update logic here
     * to use the CPU while it's less busy and earn performance.
     */
    bool frameRenderingQueued(const Ogre::FrameEvent& evt);

    //! \brief Triggered once a frame rendering has ended.
    bool frameEnded(const Ogre::FrameEvent& evt);

    //! \brief Exit the game.
    bool quit(const CEGUI::EventArgs &e);

    //! \brief Setup the ray scene query, use CEGUI's mouse position
    //! This permits to get the mouse world coordinates and other entities present below it.
    Ogre::RaySceneQueryResult& doRaySceneQuery(const OIS::MouseEvent &arg);

    /*! \brief Print a string in the upper left corner of the screen.
     * Displays the given text on the screen starting in the upper-left corner.
     */
    void printText(const std::string& text);

    inline GameMap* getClientGameMap()
    { return mGameMap.get(); }

    inline const GameMap* getClientGameMap() const
    { return mGameMap.get(); }

    inline ModeManager* getModeManager()
    { return mModeManager.get(); }

    inline Ogre::RenderWindow* getRenderWindow()
    { return mWindow; }

    void addChatMessage(ChatMessage* message);

    void notifyChatInputMode(bool isChatInputMode, bool sendChatMsg = false);
    void notifyChatChar(int text);
    void notifyChatCharDel();

    //! \brief Accessors for camera manager
    void setCameraPosition(const Ogre::Vector3& position);
    void moveCamera(CameraManager::Direction direction);

    void setActiveCameraNearClipDistance(Ogre::Real value);
    Ogre::Real getActiveCameraNearClipDistance();
    void setActiveCameraFarClipDistance(Ogre::Real value);
    Ogre::Real getActiveCameraFarClipDistance();
    Ogre::Vector3 getCameraViewTarget();
    void cameraFlyTo(const Ogre::Vector3& destination);

    CameraManager* getCameraManager()
    {
        return &mCameraManager;
    }

    //! \brief Set max framerate
    inline void setMaxFPS(sf::Int32 fps)
    {
        mFrameDelay = 1000 / fps;
    }

    inline sf::Int32 getMaxFPS()
    {

        return mFrameDelay / 1000;
    }

private:
    //! \brief Tells whether the frame listener is initialized.
    bool mInitialized;

    //! \brief The Ogre render window reference. Don't delete it.
    Ogre::RenderWindow* mWindow;

    //! \brief Foreign reference to gui.
    Gui*                 mGui;
    std::unique_ptr<RenderManager> mRenderManager;
    std::unique_ptr<GameMap>       mGameMap;
    std::unique_ptr<ModeManager>   mModeManager;

    bool                 mShowDebugInfo;
    bool                 mContinue;
    int                  mTerminalWordWrap;
    unsigned int         mChatMaxMessages;
    float                mChatMaxTimeDisplay;

    //! \brief Reference to the Ogre ray scene query handler. Don't delete it.
    Ogre::RaySceneQuery* mRaySceneQuery;

    //! \brief To see if the frameListener wants to exit
    bool mExitRequested;

    std::deque<ChatMessage*> mChatMessages;

    //! \brief The Camera manager
    CameraManager mCameraManager;

    //! \brief The chat string for the local player
    std::string mChatString;
    bool mIsChatInputMode;

    //! \brief Clock to keep track of frame time
    sf::Clock mFpsClock;
    //! \brief The current minimum frame time.
    //! If less time elapes between frames, sleep.
    sf::Int32 mFrameDelay;

    //! \brief Actually exit application
    void exitApplication();

    //! \brief Updates server-turn independant creature animation, audio, and overall rendering.
    void updateAnimations(Ogre::Real timeSinceLastFrame);

    void refreshChat();

    void refreshPlayerDisplay(const std::string& goalsDisplayString);
};

#endif // __ODFRAMELISTENER_H__
