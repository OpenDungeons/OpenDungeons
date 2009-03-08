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
	int tempInt;
	Creature tempCreature;
	char tempCellName[255];
	int xLocation, yLocation, zLocation;
	int objectsToLoad;

	levelFile >> objectsToLoad;

	// Read in the map tiles from disk
	for(int i = 0; i < objectsToLoad; i++)
	{
		
		tempTile = new Tile;
		levelFile >> xLocation >> yLocation;
		tempTile->location = Ogre::Vector3(xLocation, yLocation, 0);
		sprintf(tempCellName, "Level_%3i_%3i", xLocation, yLocation);
		tempTile->name = tempCellName;
		tempTile->x = xLocation;
		tempTile->y = yLocation;

		levelFile >> tempInt;
		tempTile->setType( (Tile::TileType) tempInt );

		levelFile >> tempInt;
		tempTile->setFullness(tempInt);

		gameMap.addTile(tempTile);
	}

	// Read in the creature class descriptions
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{
		double tempX, tempY, tempZ;
		//levelFile >> tempCreature.className >> tempCreature.meshName >> tempX >> tempY >> tempZ;
		//tempCreature.scale = Ogre::Vector3(tempX, tempY, tempZ);
	

		levelFile >> tempString >> tempString2 >> tempX >> tempY >> tempZ;
		Creature *p = new Creature(tempString, tempString2, Ogre::Vector3(tempX, tempY, tempZ));
		gameMap.addClassDescription(p);

		//gameMap.addClassDescription(tempCreature);
	}

	// Read in the actual creatures themselves
	levelFile >> objectsToLoad;
	for(int i = 0; i < objectsToLoad; i++)
	{

		Creature *newCreature = new Creature;

		levelFile >> newCreature;

		newCreature->meshName = gameMap.getClass(newCreature->className)->meshName;
		newCreature->scale = gameMap.getClass(newCreature->className)->scale;

		gameMap.addCreature(newCreature);
	}

	levelFile.close();
}

void writeGameMapToFile(string fileName)
{
	ofstream levelFile(fileName.c_str(), ifstream::out);
	Creature *tempCreature;

	// Write out the tiles to the file
	levelFile << gameMap.numTiles() << endl;
	for(int i = 0; i < gameMap.numTiles(); i++)
	{
		levelFile << gameMap.getTile(i)->x << "\t" << gameMap.getTile(i)->y << "\t";
		levelFile << gameMap.getTile(i)->getType() << "\t" << gameMap.getTile(i)->getFullness();

		levelFile << endl;
	}

	// Write out the creature descriptions to the file
	levelFile << "\n" << gameMap.numClassDescriptions() << "\n";
	for(int i = 0; i < gameMap.numClassDescriptions(); i++)
	{
		
		tempCreature = gameMap.getClassDescription(i);
		levelFile << tempCreature->className << "\t" << tempCreature->meshName << "\t" << tempCreature->scale.x << "\t" << tempCreature->scale.y << "\t" << tempCreature->scale.z << "\n";
	}

	// Write out the individual creatures to the file
	levelFile << "\n" << gameMap.numCreatures() << "\n";
	for(int i = 0; i < gameMap.numCreatures(); i++)
	{
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

