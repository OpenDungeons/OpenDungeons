#ifndef DIRECTIONAL_TRAP_H
#define DIRECTIONAL_TRAP_H

#include <utility>
#include "Trap.h"

class DirectionalTrap : public Trap
{
    public:
        DirectionalTrap(int, int);

    protected:
        std::pair<int, int> dir;
        std::pair<int, int> projection_on_border(int, int);
};

#endif