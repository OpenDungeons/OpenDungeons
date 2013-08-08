#include "FppMode.h"
#include "Gui.h"
#include "Socket.h"

FppMode::FppMode(ModeContext *modeContext):AbstractApplicationMode(modeContext)
{



}

FppMode::~FppMode(){



}


bool FppMode::mouseMoved     (const OIS::MouseEvent &arg){
    CEGUI::System::getSingleton().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);


}

bool FppMode::FppMode   (const OIS::MouseEvent &arg, OIS::MouseButtonID id){


    CEGUI::System::getSingleton().injectMouseButtonDown(
    Gui::getSingletonPtr()->convertButton(id));



}
bool FppMode::mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id){

    CEGUI::System::getSingleton().injectMouseButtonUp(
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
