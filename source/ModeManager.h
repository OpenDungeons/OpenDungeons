#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <stack>


using std::stack;

class AbstractApplicationMode;
class ModeContext;
class GameMap;
class MiniMap;
class Console;

class ModeManager
{



public:

enum ModeType {
    MENU,
    GAME,
    EDITOR,
    CONSOLE,
    PREV
    };



ModeManager(GameMap* ,MiniMap*, Console* cn );
~ModeManager();

AbstractApplicationMode*  getCurrentMode();


AbstractApplicationMode*  progressMode(ModeType);
AbstractApplicationMode*  regressMode();

int lookForNewMode();




private:
Console *cn;
AbstractApplicationMode* modesArray[4];
stack<ModeType> modesStack;
ModeContext *mc;



};

#endif // MODEMANAGER_H
