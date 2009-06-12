#include <ctype.h>

#include "Functions.h"
#include "Creature.h"

void readGameMapFromFile(string fileName)
{
	ifstream levelFile(fileName.c_str(), ifstream::in);
	if( !levelFile.good() )
	{
		cout << "Error: File not found:  " << fileName << "\n\n\n";
		exit(1);
	}

	gameMap.clearAll();

	Tile *tempTile;
	string tempString, tempString2;
	Creature tempCreature;
	int objectsToLoad;

	levelFile >> objectsToLoad;

	// Read in the map tiles from disk
	for(int i = 0; i < objectsToLoad; i++)
	{
		
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempTile = new Tile;
		levelFile >> tempTile;

		gameMap.addTile(tempTile);
	}

	// Loop over all the tiles and force them to examine their
	// neighbors.  This allows them to switch to a mesh with fewer
	// polygons if some are hidden by the neighbors.
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		itr->second->setFullness( itr->second->getFullness() );
		itr++;
	}

	// Read in the creature class descriptions
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and writeGameMapToFile.
		//Changes to this code should be reflected in that code as well
		double tempX, tempY, tempZ, tempSightRadius, tempDigRate;
		int tempHP, tempMana;
		levelFile >> tempString >> tempString2 >> tempX >> tempY >> tempZ;

		levelFile >> tempHP >> tempMana;
		levelFile >> tempSightRadius >> tempDigRate;

		Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ), tempHP, tempMana, tempSightRadius, tempDigRate);
		cout << "\nnew dig rate:  " << p->digRate;
		cout.flush();
		gameMap.addClassDescription(p);
	}

	// Read in the actual creatures themselves
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{

		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and writeGameMapToFile.
		//Changes to this code should be reflected in that code as well
		Creature *newCreature = new Creature;

		levelFile >> newCreature;

		gameMap.addCreature(newCreature);
	}

	levelFile.close();
}

void writeGameMapToFile(string fileName)
{
	ofstream levelFile(fileName.c_str(), ifstream::out);
	Creature *tempCreature;
	Tile *tempTile;

	// Write out the tiles to the file
	levelFile << gameMap.numTiles() << endl;
	TileMap_t::iterator itr = gameMap.firstTile();
	while(itr != gameMap.lastTile())
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempTile = itr->second;
		levelFile << tempTile->x << "\t" << tempTile->y << "\t";
		levelFile << tempTile->getType() << "\t" << tempTile->getFullness();

		levelFile << endl;

		itr++;
	}

	// Write out the creature descriptions to the file
	levelFile << "\n" << gameMap.numClassDescriptions() << "\n";
	for(unsigned int i = 0; i < gameMap.numClassDescriptions(); i++)
	{
		
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		tempCreature = gameMap.getClassDescription(i);
		levelFile << tempCreature->className << "\t" << tempCreature->meshName << "\t" << tempCreature->scale.x << "\t" << tempCreature->scale.y << "\t" << tempCreature->scale.z << "\t";
		levelFile << tempCreature->hp << "\t" << tempCreature->mana << "\t";
		levelFile << tempCreature->sightRadius << "\t" << tempCreature->digRate << "\n";
	}

	// Write out the individual creatures to the file
	levelFile << "\n" << gameMap.numCreatures() << "\n";
	for(unsigned int i = 0; i < gameMap.numCreatures(); i++)
	{
		//NOTE: This code is duplicated in the client side method
		//"addclass" defined in src/Client.cpp and readGameMapFromFile.
		//Changes to this code should be reflected in that code as well
		levelFile << gameMap.getCreature(i);
	}

	levelFile << endl;

	levelFile.close();
}

string forceLowercase(string s)
{
	string tempString;
	
	for(unsigned int i = 0; i < s.size(); i++)
	{
		tempString += tolower(s[i]);
	}

	return tempString;
}

/************************************************
*randomDouble() returns a double between the 
*lower number entered and the higher number 
*entered.  One or both numbers can be negative
************************************************/
double randomDouble(double min,double max)
{
	if(min > max)
	{
	double temp = min;
	min = max;
	max = temp;
	}
	
double r = -1*((double)rand()/(double)(RAND_MAX+1));
double value = min;
value += r*(max-min);

return value;
}

//returns a gaussian distributed random double value in [-1,1]
double gaussianRandomDouble()
{
	double temp1 = randomDouble(0.0, 1.0);
	double temp2 = randomDouble(0.0, 1.0);

	double val1;
	//double val2;

	val1 = sqrt(-2.0*log(temp1)) * cos(2.0*M_PI*temp2);
	// val2 = sqrt(-2.0*log(temp1)) * sin(2.0*M_PI*temp2);
	
	return val1;
}

void seedRandomNumberGenerator()
{
	srand(time(0));
}

void swap(int &a, int &b)
{
	int temp = a;
	a = b;
	b = temp;
}

