#ifndef GAMEODE_H
#define GAMEODE_H


#include "AbstractApplicationMode.h"


class  GameMode: public AbstractApplicationMode  {


 public:
 

    GameMode(GameMap* ,MiniMap*);
    GameMode( AbstractApplicationMode const& );
    virtual ~GameMode();

    inline virtual OIS::Mouse*      getMouse()      const   {return mMouse;}
    inline virtual OIS::Keyboard*   getKeyboard()   const   {return mKeyboard;}

    virtual bool mouseMoved     (const OIS::MouseEvent &arg);
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed     (const OIS::KeyEvent &arg);
    virtual bool keyReleased    (const OIS::KeyEvent &arg);
    virtual void handleHotkeys  (OIS::KeyCode keycode);

protected:
    virtual bool isInGame();
    

};

#endif // GAMEMODE_H
