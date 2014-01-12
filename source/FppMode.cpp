#include "FppMode.h"
#include "Gui.h"
#include "Socket.h"

FppMode::FppMode(ModeContext *modeContext):AbstractApplicationMode(modeContext)
{


}

FppMode::~FppMode(){



}


bool FppMode::mouseMoved     (const OIS::MouseEvent &arg){
    // CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
    // if (arg.state.X.abs == 0)
    // 	mc->frameListener->cm->move(CameraManager::moveLeft);
    // else
    // 	mc->frameListener->cm->move(CameraManager::stopLeft);

    // if (arg.state.X.abs ==  arg.state.width)
    // 	mc->frameListener->cm->move(CameraManager::moveRight);
    // else
    // 	mc->frameListener->cm->move(CameraManager::stopRight);

    // if (arg.state.Y.abs == 0)
    // 	mc->frameListener->cm->move(CameraManager::moveForward);
    // else
    // 	mc->frameListener->cm->move(CameraManager::stopForward);

    // if (arg.state.Y.abs ==  arg.state.height)
    // 	mc->frameListener->cm->move(CameraManager::moveBackward);
    // else
    // 	mc->frameListener->cm->move(CameraManager::stopBackward);

}

bool FppMode::mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id){


    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
    Gui::getSingletonPtr()->convertButton(id));

}
bool FppMode::mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id){

    //CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->injectMouseButtonUp(
    //    Gui::getSingletonPtr()->convertButton(id));
    CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
                Gui::getSingletonPtr()->convertButton(id));

}
bool FppMode::keyPressed     (const OIS::KeyEvent &arg){

    switch (arg.key)
    {

	case OIS::KC_ESCAPE:
	    regressMode();
            Gui::getSingletonPtr()->switchGuiMode();
	    break;
    }

}
bool FppMode::keyReleased    (const OIS::KeyEvent &arg){


}


void FppMode::handleHotkeys  (OIS::KeyCode keycode){





}
bool FppMode::isInGame(){
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;


}
    
void FppMode::giveFocus(){

    mc->mMouse->setEventCallback(this);
    mc->mKeyboard->setEventCallback(this);
}
