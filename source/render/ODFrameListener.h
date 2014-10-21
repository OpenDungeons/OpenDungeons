/*!
 * \file   ODFrameListener.h
 * \date   09 April 2011
 * \auth	(require 'ecb)or Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 *
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

#ifndef __ODFRAMELISTENER_H__
#define __ODFRAMELISTENER_H__

#include "camera/CameraManager.h"

#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <OgreSingleton.h>
#include <Overlay/OgreOverlaySystem.h>
#include <OgreSceneQuery.h>
#include <OgreTimer.h>
#include <CEGUI/EventArgs.h>

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OISMouse.h>

#include <deque>

class RenderManager;
class AbstractApplicationMode;
class ModeManager;
class CameraManager;
class ChatMessage;
class GameMap;
class MiniMap;
class CameraManager;

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

friend class Console;
friend class ODClient;

public:
    // Constructor takes a RenderWindow because it uses that to determine input context
    ODFrameListener(Ogre::RenderWindow* renderWindow, Ogre::OverlaySystem* overLaySystem);
    virtual ~ODFrameListener();

    void requestExit();

    inline bool isTerminalActive () const
    { return mTerminalActive; }

    inline void setTerminalActive(bool active)
    { mTerminalActive = active; }

    inline unsigned int getChatMaxMessages() const
    { return mChatMaxMessages; }

    inline void setChatMaxMessages(unsigned int nM)
    { mChatMaxMessages = nM; }

    inline unsigned int getChatMaxTimeDisplay() const
    { return mChatMaxTimeDisplay; }

    inline void setChatMaxTimeDisplay(unsigned int nT)
    { mChatMaxTimeDisplay = nT; }

    //! \brief Toggle the display of debug info (default key used to do that: F11)
    void toggleDebugInfo()
    { mShowDebugInfo = !mShowDebugInfo; }

    inline Ogre::RaySceneQuery* getRaySceneQuery()
    { return mRaySceneQuery; }

    //! \brief Adjust mouse clipping area
    virtual void windowResized(Ogre::RenderWindow* rw);

    //! \brief Unattach OIS before window shutdown (very important under Linux)
    virtual void windowClosed(Ogre::RenderWindow* rw);

    /*! \brief The main rendering function for the OGRE 3d environment.
     *
     * This function is the one which actually carries out all of the rendering in
     * the OGRE 3d system.  Since all the rendering must happen here, one of the
     * operations performed by this function is the processing of a request queue
     * full of RenderRequest structures.
     */
    bool frameStarted (const Ogre::FrameEvent& evt);
    bool frameEnded (const Ogre::FrameEvent& evt);

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
    { return mGameMap; }

    inline const GameMap* getClientGameMap() const
    { return mGameMap; }

    inline ModeManager* getModeManager() const
    { return mModeManager; }

    inline Ogre::RenderWindow* getRenderWindow()
    { return mWindow; }

    void addChatMessage(ChatMessage* message);

    //! \brief The console output
    void setConsoleCommandOutput(const std::string& output)
    {
        mCommandOutput = output;
    }

    MiniMap* getMiniMap() {return mMiniMap;}

    void notifyChatInputMode(bool isChatInputMode, bool sendChatMsg = false);
    void notifyChatChar(int text);
    void notifyChatCharDel();

    //! \brief Accessors for camera manager
    void setCameraPosition(const Ogre::Vector3& position);
    void moveCamera(CameraManager::Direction direction);
    void cameraStopZoom();
    void setCameraRotateSpeed(Ogre::Real value);
    Ogre::Real getCameraRotateSpeedInDegrees();
    void setActiveCameraNearClipDistance(Ogre::Real value);
    Ogre::Real getActiveCameraNearClipDistance();
    void setActiveCameraFarClipDistance(Ogre::Real value);
    Ogre::Real getActiveCameraFarClipDistance();
    const Ogre::Vector3 getCameraViewTarget();
    void cameraFlyTo(const Ogre::Vector3& destination);
    void onMiniMapClick(int xPos, int yPos);
    CameraManager* getCameraManager()
    {
        return mCameraManager;
    }

private:
    //! \brief Tells whether the frame listener is initialized.
    bool mInitialized;

    //! \brief The Ogre render window reference. Don't delete it.
    Ogre::RenderWindow* mWindow;

    ModeManager*            mModeManager;

    bool                    mShowDebugInfo;
    bool                    mContinue;
    bool                    mTerminalActive;
    int                     mTerminalWordWrap;
    unsigned int            mChatMaxMessages;
    float                   mChatMaxTimeDisplay;

    //! \brief Reference to the Ogre ray scene query handler. Don't delete it.
    Ogre::RaySceneQuery*    mRaySceneQuery;

    Ogre::Timer             mStatsDisplayTimer;
    GameMap*                mGameMap;

    //! \brief The minimap corresponding to the GameMap.
    MiniMap*                mMiniMap;

    //! \brief To see if the frameListener wants to exit
    bool mExitRequested;

    // Console variables
    std::string mCommand, mArguments, mPrompt;

    //! \brief The console output
    std::string mCommandOutput;

    std::deque<ChatMessage*> mChatMessages;
    std::string mPromptCommand;

    //! \brief The Camera manager
    CameraManager* mCameraManager;

    //! \brief The chat string for the local player
    std::string mChatString;
    bool mIsChatInputMode;

    //! \brief Actually exit application
    void exitApplication();

    //! \brief Tells whether the client and or server are up and running.
    bool isConnected();

    //! \brief Tells whether a server is running.
    bool isServer();

    //! \brief Tells whether a client socket is active.
    bool isClient();

    //! \brief Updates server-turn independant creature animation, audio, and overall rendering.
    void updateAnimations(Ogre::Real timeSinceLastFrame);

    void refreshChat();

    void refreshPlayerDisplay(const std::string& goalsDisplayString);
};

#endif // __ODFRAMELISTENER_H__
