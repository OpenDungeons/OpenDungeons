#include "MenuMode.h"
#include "Gui.h"
#include "Socket.h"

MenuMode::MenuMode(ModeContext *modeContext):AbstractApplicationMode(modeContext)
{

}

MenuMode::~MenuMode() {

}


bool MenuMode::mouseMoved(const OIS::MouseEvent &arg) {
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition(arg.state.X.abs, arg.state.Y.abs);
}

bool MenuMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id) {
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id) {
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::keyPressed(const OIS::KeyEvent &arg) {
    switch (arg.key)
    {

    case OIS::KC_ESCAPE:
        regressMode();
        Gui::getSingletonPtr()->switchGuiMode();
        break;
    default:
        break;
    }
    return true;
}

bool MenuMode::keyReleased(const OIS::KeyEvent &arg) {
    return true;
}

void MenuMode::handleHotkeys(OIS::KeyCode keycode) {

}

bool MenuMode::isInGame(){
    //TODO: this exact function is also in ODFrameListener, replace it too after GameState works
    //TODO - we should use a bool or something, not the sockets for this.
    return (Socket::serverSocket != NULL || Socket::clientSocket != NULL);
    //return GameState::getSingletonPtr()->getApplicationState() == GameState::ApplicationState::GAME;
}

void MenuMode::giveFocus(){

    mMc->mMouse->setEventCallback(this);
    mMc->mKeyboard->setEventCallback(this);
}
