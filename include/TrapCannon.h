#ifndef TRAPCANNON_H
#define TRAPCANNON_H

#include "ProximityTrap.h"

class TrapCannon : public ProximityTrap
{
	public:
		TrapCannon();
        //~ bool doUpkeep();
	private:
		double cannonHeight;

};

#endif

