/*!
 * \file   ODFrameListener.h
 * \date   09 April 2011
 * \auth	(require 'ecb)or Ogre team, andrewbuck, oln, StefanP.MUC
 * \brief  Handles the input and rendering request
 */

#ifndef __ExampleFrameListener_H__
#define __ExampleFrameListener_H__

#include <deque>
#include <vector>

#include <pthread.h>
#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <OgreSingleton.h>
#include <OgreSceneQuery.h>
#include <OgreTimer.h>
#include <CEGUIEventArgs.h>



//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OISMouse.h>

#include "ProtectedObject.h"

class Socket;
class RenderManager;
class AbstractApplicationMode;
class ModeManager;
class GameMode;
class CameraManager;
class SoundEffectsHelper;
class ChatMessage;
class GameMap;
class MiniMap;
class GameContext;
class EditorContext;

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
    public:
        // Constructor takes a RenderWindow because it uses that to determine input context
        ODFrameListener(Ogre::RenderWindow* win);
        virtual ~ODFrameListener();

	void makeEditorContext();
	void makeGameContext();
        void requestExit();
        bool getThreadStopRequested();
        void setThreadStopRequested(bool value);
        void requestStopThreads();

        inline bool         isTerminalActive () const               {return terminalActive;}
        inline void         setTerminalActive(bool active)          {terminalActive = active;}

        inline unsigned int getChatMaxMessages() const              {return chatMaxMessages;}
        inline void         setChatMaxMessages(unsigned int nM)     {chatMaxMessages = nM;}

        inline unsigned int getChatMaxTimeDisplay() const           {return chatMaxTimeDisplay;}
        inline void         setChatMaxTimeDisplay(unsigned int nT)  {chatMaxTimeDisplay = nT;}

        inline Ogre::SceneNode*     getCreatureSceneNode() const    {return creatureSceneNode;}
        inline Ogre::RaySceneQuery* getRaySceneQuery    ()          {return mRaySceneQuery;}

        //Adjust mouse clipping area
        virtual void windowResized(Ogre::RenderWindow* rw);

        //Unattach OIS before window shutdown (very important under Linux)
        virtual void windowClosed(Ogre::RenderWindow* rw);

        // Override frameStarted event to process that (don't care about frameEnded)
        bool frameStarted   (const Ogre::FrameEvent& evt);
        bool frameEnded     (const Ogre::FrameEvent& evt);

        //CEGUI Functions
        bool quit(const CEGUI::EventArgs &e);
        Ogre::RaySceneQueryResult& doRaySceneQuery(const OIS::MouseEvent &arg);

	void printText(const std::string& text);
	    
        //NOTE - we should generally avoid using this function
        inline GameMap* getGameMap() {return gameMap;}
        
        inline const GameMap* getGameMap() const {return gameMap;}

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

    private:
        ODFrameListener(const ODFrameListener&);

        Ogre::RenderWindow*     mWindow;
        Ogre::RaySceneQuery*    mRaySceneQuery;
        RenderManager*          renderManager;
        CameraManager*          cameraManager;
        ModeManager*            inputManager;
        SoundEffectsHelper*     sfxHelper;
        bool                    mContinue;
        bool                    terminalActive;
        int                     terminalWordWrap;
        unsigned int            chatMaxMessages;
        unsigned int            chatMaxTimeDisplay;
        double                  frameDelay;
        long int                previousTurn;
        Ogre::SceneNode*        creatureSceneNode;
        Ogre::SceneNode*        roomSceneNode;
        Ogre::SceneNode*        fieldSceneNode;
        Ogre::SceneNode*        lightSceneNode;
        Ogre::SceneNode*        rockSceneNode;
        Ogre::Timer             statsDisplayTimer;
        GameMap*                gameMap;
	MiniMap*                miniMap;


	GameContext*   gc;
	EditorContext*   ed;
        std::vector<Ogre::ColourValue> playerColourValues;

        //To see if the frameListener wants to exit
        ProtectedObject<bool>   threadStopRequested;
        ProtectedObject<bool>   exitRequested;
        
        void exitApplication();
        bool isInGame       ();
};

#endif
