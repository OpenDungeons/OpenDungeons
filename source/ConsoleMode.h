#ifndef CONSOLEMODE_H
#define CONSOLEMODE_H


#include "AbstractApplicationMode.h"
#include "ModeContext.h"
#include <list>
#include <string>

using std::string; using std::list;


class ModeContext;
class PrefixTreeLL;

class  ConsoleMode: public AbstractApplicationMode  {


 public:
 

    ConsoleMode(ModeContext*, Console* );

    virtual ~ConsoleMode();

    inline virtual OIS::Mouse*      getMouse()         {return mc->mMouse;}
    inline virtual OIS::Keyboard*   getKeyboard()      {return mc->mKeyboard;}

    virtual bool mouseMoved     (const OIS::MouseEvent &arg);
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id);
    virtual bool keyPressed     (const OIS::KeyEvent &arg);
    virtual bool keyReleased    (const OIS::KeyEvent &arg);
    virtual void handleHotkeys  (OIS::KeyCode keycode);
    virtual bool isInGame();
    virtual void giveFocus();

  private:
    Console* cn;
    PrefixTreeLL* pt;
    list<string>* ll ; 
    string prefix;     ;    
    bool nonTagKeyPressed;
    list<string>::iterator it;





};

#endif // CONSOLEMODE_H
