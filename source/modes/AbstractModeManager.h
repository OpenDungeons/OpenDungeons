#ifndef ABSTRACTMODEMANAGER_H
#define ABSTRACTMODEMANAGER_H

class AbstractModeManager
{
public:
    AbstractModeManager()
    {};
    //Enum for describing mode type
    enum ModeType
    {
        NONE = 0, // No change requested
        MENU = 1,
        MENU_SKIRMISH,
        MENU_MULTIPLAYER_CLIENT,
        MENU_MULTIPLAYER_SERVER,
        MENU_EDITOR,
        MENU_CONFIGURE_SEATS,
        MENU_REPLAY,
        GAME,
        EDITOR,
        CONSOLE,
        FPP,
        PREV, // Parent game mode requested
        ALL, //Used for console commands working in all modes
        NUM_ELEMS //Number of types
    };
    virtual ~AbstractModeManager()
    {}
    virtual ModeType getCurrentModeTypeExceptConsole() const = 0;
private:
    AbstractModeManager(const AbstractModeManager&) = delete;
    AbstractModeManager& operator=(const AbstractModeManager&) = delete;

};

#endif // ABSTRACTMODEMANAGER_H
