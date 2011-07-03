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
#include "ProtectedObject.h"

class Socket;
class RenderManager;
class InputManager;
class CameraManager;
class SoundEffectsHelper;
class ChatMessage;
class GameMap;

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
    public:
        // Constructor takes a RenderWindow because it uses that to determine input context
        ODFrameListener(Ogre::RenderWindow* win);
        virtual ~ODFrameListener();
        void requestExit();
        bool getThreadStopRequested();
        void setThreadStopRequested(bool value);
        void requestStopThreads();

        inline const bool& isTerminalActive() const{return terminalActive;}
        inline void setTerminalActive(const bool& active){terminalActive = active;}

        inline Ogre::SceneNode* getCreatureSceneNode() const{return creatureSceneNode;};

        static ODFrameListener& getSingleton();
        static ODFrameListener* getSingletonPtr();

        //Adjust mouse clipping area
        virtual void windowResized(Ogre::RenderWindow* rw);

        //Unattach OIS before window shutdown (very important under Linux)
        virtual void windowClosed(Ogre::RenderWindow* rw);

        // Override frameStarted event to process that (don't care about frameEnded)
        bool frameStarted(const Ogre::FrameEvent& evt);
        bool frameEnded(const Ogre::FrameEvent& evt);

        //CEGUI Functions
        bool quit(const CEGUI::EventArgs &e);
        Ogre::RaySceneQueryResult& doRaySceneQuery(const OIS::MouseEvent &arg);

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

    protected:
        Ogre::RenderWindow* mWindow;

        double frameDelay;

        // Mouse query stuff
        Ogre::RaySceneQuery* mRaySceneQuery; // The ray scene query pointer

        RenderManager* renderManager;
        CameraManager* cameraManager;
        InputManager* inputManager;

        void exitApplication();

    private:
        ODFrameListener(const ODFrameListener&);
        bool isInGame();

        bool terminalActive;
        int terminalWordWrap;

        Ogre::SceneNode* creatureSceneNode;
        Ogre::SceneNode* roomSceneNode;
        Ogre::SceneNode* fieldSceneNode;
        Ogre::SceneNode* lightSceneNode;

        std::vector<Ogre::ColourValue> playerColourValues;

        SoundEffectsHelper* sfxHelper;

        Ogre::Timer statsDisplayTimer;
        long int lastTurnDisplayUpdated;

        bool mContinue;
        
        //To see if the frameListener wants to exit
        ProtectedObject<bool> threadStopRequested;
        ProtectedObject<bool> exitRequested;
};

#endif
