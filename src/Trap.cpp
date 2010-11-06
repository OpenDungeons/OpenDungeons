#include "Trap.h"

Trap::Trap()
{
	controllingSeat = NULL;
	meshExists = false;
}

Trap* Trap::createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles)
{
	Trap *tempTrap = NULL;

	switch(nType)
	{
		case cannon:         tempTrap = new TrapCannon();         break;
	}

	if(tempTrap == NULL)
	{
		cerr << "\n\n\nERROR: Trying to create a trap of unknown type, bailing out.\n";
		cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
		exit(1);
	}

	tempTrap->meshName = getMeshNameFromTrapType(nType);
	tempTrap->type = nType;

	static int uniqueNumber = -1;
	std::stringstream tempSS;

	tempSS.str("");
	tempSS << tempTrap->meshName << "_" << uniqueNumber;
	uniqueNumber--;
	tempTrap->name = tempSS.str();

	for(unsigned int i = 0; i < nCoveredTiles.size(); i++)
		tempTrap->addCoveredTile(nCoveredTiles[i]);

	return tempTrap;
}

Trap::TrapType Trap::getType()
{
	return type;
}

string Trap::getMeshNameFromTrapType(TrapType t)
{
	switch(t)
	{
		case nullTrapType:      return "NullTrapType";      break;
		case cannon:            return "Cannon";            break;
	}

	return "UnknownTrapType";
}

Trap::TrapType Trap::getTrapTypeFromMeshName(string s)
{
	if(s.compare("Cannon") == 0)
		return cannon;
	else
	{
		cerr << "\n\n\nERROR:  Trying to get trap type from unknown mesh name, bailing out.\n";
		cerr << "Sourcefile: " << __FILE__ << "\tLine: " << __LINE__ << "\n\n\n";
		exit(1);
	}
}

