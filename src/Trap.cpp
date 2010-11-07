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

int Trap::costPerTile(TrapType t)
{
	switch(t)
	{
		case cannon:     return 500;     break;
	}

	return 100;
}

void Trap::doUpkeep()
{
	doUpkeep(this);
}

void Trap::doUpkeep(Trap *t)
{
}

void Trap::addCoveredTile(Tile* t, double nHP)
{
	coveredTiles.push_back(t);
	tileHP[t] = nHP;
}

void Trap::removeCoveredTile(Tile* t)
{
	for(unsigned int i = 0; i < coveredTiles.size(); i++)
	{
		if(t == coveredTiles[i])
		{
			coveredTiles.erase(coveredTiles.begin() + i);
			tileHP.erase(t);
			break;
		}
	}

	/*
	// Destroy the mesh for this tile.
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyTrap;
	request->p = this;
	request->p2 = t;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
	*/
}

Tile* Trap::getCoveredTile(int index)
{
	return coveredTiles[index];
}

std::vector<Tile*> Trap::getCoveredTiles()
{
	return coveredTiles;
}

unsigned int Trap::numCoveredTiles()
{
	return coveredTiles.size();
}

void Trap::clearCoveredTiles()
{
	coveredTiles.clear();
}

double Trap::getHP(Tile *tile)
{
	return tileHP[tile];
}

double Trap::getDefense()
{
	return 0;
}

void Trap::takeDamage(double damage, Tile *tileTakingDamage)
{
	tileHP[tileTakingDamage] -= damage;
}

void Trap::recieveExp(double experience)
{
	exp += experience;
}

bool Trap::isMobile()
{
	return false;
}

int Trap::getLevel()
{
	return 1;
}

int Trap::getColor()
{
	if(controllingSeat != NULL)
		return controllingSeat->color;
	else
		return 0;
}

string Trap::getName()
{
	return name;
}

AttackableObject::AttackableObjectType Trap::getAttackableObjectType()
{
	return trap;
}

