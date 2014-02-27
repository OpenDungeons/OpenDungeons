#ifndef FPPMODE_H
#define FPPMODE_H


#include "AbstractApplicationMode.h"
#include "ModeContext.h"

class ModeContext;

class  FppMode: public AbstractApplicationMode  {


 public:
 

    FppMode(ModeContext*);

    virtual ~FppMode();

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

#endif // FPPMODE_H
