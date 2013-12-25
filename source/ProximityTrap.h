#ifndef PROXIMITY_TRAP_H
#define PROXIMITY_TRAP_H

#include "Trap.h"

class ProximityTrap : public Trap
{
public:
    ProximityTrap();
    
protected:
    double range;
};

#endif
