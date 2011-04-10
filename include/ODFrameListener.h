/*!
 * \file   ODFrameListener.h
 * \date   09 April 2011
 * \author Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 */

#ifndef __ExampleFrameListener_H__
#define __ExampleFrameListener_H__

#include <deque>
#include <vector>

#include <pthread.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <CEGUI.h>

//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OIS.h>

#include "Tile.h"

class Socket;
class RenderManager;
class SoundEffectsHelper;
class ChatMessage;

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
        public Ogre::WindowEventListener,
        public OIS::MouseListener,
        public OIS::KeyListener
{
    protected:
        void updateStats();

    public:
        // Constructor takes a RenderWindow because it uses that to determine input context
        ODFrameListener(Ogre::RenderWindow* win, Ogre::Camera* cam,
                bool bufferedKeys, bool bufferedMouse, bool bufferedJoy);
        virtual ~ODFrameListener();

        static ODFrameListener& getSingleton();
        static ODFrameListener* getSingletonPtr();

        //Adjust mouse clipping area
        virtual void windowResized(Ogre::RenderWindow* rw);

        //Unattach OIS before window shutdown (very important under Linux)
        virtual void windowClosed(Ogre::RenderWindow* rw);

        void moveCamera(Ogre::Real frameTime);
        Ogre::Vector3 getCameraViewTarget();
        void flyTo(Ogre::Vector3 destination);

        void showDebugOverlay(bool show);

        // Override frameStarted event to process that (don't care about frameEnded)
        bool frameStarted(const Ogre::FrameEvent& evt);
        bool frameEnded(const Ogre::FrameEvent& evt);

        //CEGUI Functions
        bool quit(const CEGUI::EventArgs &e);
        bool mouseMoved(const OIS::MouseEvent &arg);
        Ogre::RaySceneQueryResult& doRaySceneQuery(const OIS::MouseEvent &arg);
        bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
        bool keyPressed(const OIS::KeyEvent &arg);
        bool keyReleased(const OIS::KeyEvent &arg);
        void handleHotkeys(int hotkeyNumber);

        // Console functions
        void printText(const std::string& text);
        void executePromptCommand(const std::string& command, std::string arguments);
        std::string getHelpText(std::string arg);

        // Console variables
        std::string command, arguments, commandOutput, prompt;
        std::deque<ChatMessage*> chatMessages;
        std::string promptCommand, chatString;

        // Multiplayer stuff
        std::vector<Socket*> clientSockets;
        pthread_t clientThread;
        pthread_t serverThread;
        pthread_t serverNotificationThread;
        pthread_t clientNotificationThread;
        std::vector<pthread_t*> clientHandlerThreads;
        pthread_t creatureThread;

        // Variables for chat messages
        unsigned int chatMaxMessages;
        unsigned int chatMaxTimeDisplay;

        bool mContinue;

    protected:
        Ogre::Camera* mCamera;
        Ogre::SceneNode* mCamNode;

        Ogre::Vector3 translateVector;
        Ogre::Vector3 translateVectorAccel;
        Ogre::Vector3 cameraFlightDestination;
        Ogre::Vector3 mRotateLocalVector;
        double zChange;
        Ogre::RenderWindow* mWindow;
        bool cameraIsFlying;
        Ogre::Real moveSpeed;
        Ogre::Real moveSpeedAccel;
        Ogre::Degree mRotateSpeed;
        Ogre::Degree swivelDegrees;
        Ogre::Real cameraFlightSpeed;
        bool hotkeyLocationIsValid[10];
        Ogre::Vector3 hotkeyLocation[10];

        std::string mDebugText;
        bool mStatsOn;

        unsigned int mNumScreenShots;
        float mZoomSpeed;
        Tile::TileType mCurrentTileType;
        int mCurrentFullness, mCurrentTileRadius;
        bool mBrushMode;
        double frameDelay;

        Ogre::Overlay* mDebugOverlay;

        //OIS Input devices
        OIS::InputManager* mInputManager;
        OIS::Mouse* mMouse;
        OIS::Keyboard* mKeyboard;

        // Mouse query stuff
        Ogre::RaySceneQuery* mRaySceneQuery; // The ray scene query pointer
        bool mLMouseDown, mRMouseDown; // True if the mouse buttons are down
        int mLStartDragX, mLStartDragY; // The start tile coordinates for a left drag
        int mRStartDragX, mRStartDragY; // The start tile coordinates for a left drag
        int xPos, yPos;
        bool digSetBool; // For server mode - hods whether to mark or unmark a tile for digging
        bool mouseDownOnCEGUIWindow;

        enum DragType
        {
            creature,
            mapLight,
            tileSelection,
            tileBrushSelection,
            addNewRoom,
            addNewTrap,
            nullDragType
        };

        RenderManager* renderManager;

    private:
        ODFrameListener(const ODFrameListener&);

        bool terminalActive;
        int terminalWordWrap;

        DragType mDragType;
        std::string draggedCreature, draggedMapLight;
        Ogre::SceneNode* creatureSceneNode;
        Ogre::SceneNode* roomSceneNode;
        Ogre::SceneNode* fieldSceneNode;
        Ogre::SceneNode* lightSceneNode;

        std::vector<Ogre::ColourValue> playerColourValues;

        SoundEffectsHelper* sfxHelper;
};

#endif
