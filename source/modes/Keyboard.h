#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <OISKeyboard.h>

//! \brief A class to abstract between the SFML and OIS keyboard functionality
class Keyboard
{
public:
    bool isModifierDown(OIS::Keyboard::Modifier code);
#ifndef OD_USE_SFML_WINDOW
    Keyboard(OIS::Keyboard* kb) : mKeyboard(kb) {}

    OIS::Keyboard* getKeyboard() {
        return mKeyboard;
    }

private:
    OIS::Keyboard* mKeyboard;
#endif
};

#endif // KEYBOARD_H
