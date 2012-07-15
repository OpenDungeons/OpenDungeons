#ifndef MODEMANAGER_H
#define MODEMANAGER_H

#include <stack>


using std::stack;

class AbstractApplicationMode;
class ModeContext;
class GameMap;
class MiniMap;


class ModeManager
{



public:

enum ModeType {
    MENU,
    GAME,
    EDITOR
    };



ModeManager(GameMap* ,MiniMap* );
~ModeManager();
AbstractApplicationMode*  getCurrentMode();


AbstractApplicationMode*  progressMode(ModeType);
AbstractApplicationMode*  regressMode();






private:
AbstractApplicationMode* modesArray[3];
stack<ModeType> modesStack;
ModeContext *mc;



};

#endif // MODEMANAGER_H
