#include "GameMap.h"

void GameMap::createNewMap(int xSize, int ySize)
{
	Tile *tempTile;
	char tempString[255];

	clearTiles();

	for(int j = 0; j < ySize; j++)
	{
		for(int i = 0; i < xSize; i++)
		{
			tempTile = new Tile;
			tempTile->setType(Tile::dirt);
			tempTile->setFullness(100);
			tempTile->x = i;
			tempTile->y = j;

			sprintf(tempString, "Level_%3i_%3i", i, j);
			tempTile->name = tempString;
			tempTile->createMesh();
			tiles.push_back(tempTile);
		}
	}
}

Tile* GameMap::getTile(int x, int y)
{
	for(unsigned int i = 0; i < tiles.size(); i++)
	{
		if(tiles[i]->x == x && tiles[i]->y == y)
		{
			return tiles[i];
		}
	}

	cerr << "Error: Tile not found: (" << x << ", " << y << ")\n\n\n";
	return NULL;
}

Tile* GameMap::getTile(int index)
{
	return tiles[index];
}

void GameMap::clearAll()
{
	clearTiles();
	clearCreatures();
	clearClasses();
}

void GameMap::clearTiles()
{
	for(int i = 0; i < numTiles(); i++)
	{
		tiles[i]->destroyMesh();
		delete tiles[i];
	}

	tiles.clear();
}

void GameMap::clearCreatures()
{
	for(int i = 0; i < numCreatures(); i++)
	{
		creatures[i]->destroyMesh();
		delete creatures[i];
	}

	creatures.clear();

}

void GameMap::clearClasses()
{
	for(int i = 0; i < numClassDescriptions(); i++)
	{
		delete classDescriptions[i];
	}

	classDescriptions.clear();
}

int GameMap::numTiles()
{
	return tiles.size();
}

void GameMap::addTile(Tile *t)
{
	tiles.push_back(t);
}

void GameMap::addClassDescription(Creature *c)
{
	classDescriptions.push_back( c );
}

void GameMap::addClassDescription(Creature c)
{
	classDescriptions.push_back( new Creature(c) );
}

void GameMap::addCreature(Creature *c)
{
	creatures.push_back(c);
}

Creature* GameMap::getClass(string query)
{
	for(unsigned int i = 0; i < classDescriptions.size(); i++)
	{
		if(classDescriptions[i]->className.compare(query) == 0)
			return classDescriptions[i];
	}

	return NULL;
}

int GameMap::numCreatures()
{
	return creatures.size();
}

int GameMap::numClassDescriptions()
{
	return classDescriptions.size();
}

Creature* GameMap::getCreature(int index)
{
	return creatures[index];
}

Creature* GameMap::getClassDescription(int index)
{
	return classDescriptions[index];
}


void GameMap::createAllEntities()
{
	// Create OGRE entities for map tiles
	for(int i = 0; i < numTiles(); i++)
	{
		getTile(i)->createMesh();
	}

	// Create OGRE entities for the creatures
	for(int i = 0; i < numCreatures(); i++)
	{
		Creature *currentCreature = getCreature(i);

		currentCreature->createMesh();
	}
}

Creature* GameMap::getCreature(string cName)
{
	for(int i = 0; i < numCreatures(); i++)
	{
		if(creatures[i]->name.compare(cName) == 0)
		{
			return creatures[i];
		}
	}

	return NULL;
}

