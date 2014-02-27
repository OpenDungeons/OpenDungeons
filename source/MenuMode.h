#ifndef MENUMODE_H
#define MENUMODE_H


#include "AbstractApplicationMode.h"
#include "ModeContext.h"

class ModeContext;

class  MenuMode: public AbstractApplicationMode  {


 public:
 

    MenuMode(ModeContext*);

    virtual ~MenuMode();

    inline virtual OIS::Mouse*      getMouse()         {return mMc->mMouse;}
    inline virtual OIS::Keyboard*   getKeyboard()      {return mMc->mKeyboard;}

    virtual bool mouseMoved     (const OIS::MouseEvent &arg);
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed     (const OIS::KeyEvent &arg);
    virtual bool keyReleased    (const OIS::KeyEvent &arg);
    virtual void handleHotkeys  (OIS::KeyCode keycode);
    virtual bool isInGame();
    virtual void giveFocus();

};

#endif // MENUMODE_H
