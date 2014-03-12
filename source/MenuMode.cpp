#include "MenuMode.h"

#include "Gui.h"
#include "ModeManager.h"

MenuMode::MenuMode(ModeManager *modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU)
{
}

MenuMode::~MenuMode()
{
}

bool MenuMode::mouseMoved(const OIS::MouseEvent &arg)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMousePosition((float)arg.state.X.abs, (float)arg.state.Y.abs);
}

bool MenuMode::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonDown(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
    return CEGUI::System::getSingleton().getDefaultGUIContext().injectMouseButtonUp(
        Gui::getSingletonPtr()->convertButton(id));
}

bool MenuMode::keyPressed(const OIS::KeyEvent &arg) {
    switch (arg.key)
    {

    case OIS::KC_ESCAPE:
        regressMode();
        break;
    default:
        break;
    }
    return true;
}

bool MenuMode::keyReleased(const OIS::KeyEvent &arg)
{
    return true;
}

void MenuMode::handleHotkeys(OIS::KeyCode keycode)
{
}
