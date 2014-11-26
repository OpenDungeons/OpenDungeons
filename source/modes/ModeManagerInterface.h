#ifndef MODEMANAGERINTERFACE_H
#define MODEMANAGERINTERFACE_H

#include <boost/noncopyable.hpp>

enum class ModeType
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
    ALL
};


class ModeManagerInterface : private boost::noncopyable
{
public:
    ModeManagerInterface()
    {}
    virtual ~ModeManagerInterface()
    {}
};

#endif // MODEMANAGERINTERFACE_H
