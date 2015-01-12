#ifndef TESTMODEMANAGER_H
#define TESTMODEMANAGER_H

#include "modes/AbstractModeManager.h"

class TestModeManager: public AbstractModeManager
{
public:
    TestModeManager(ModeType mode)
        : mode(mode)
    {
    }

    ModeType getCurrentModeTypeExceptConsole() const
    {
        return mode;
    }

    ModeType getCurrentModeType()
    {
        return mode;
    }
private:
    ModeType mode;
};

#endif // TESTMODEMANAGER_H
