#ifndef ROOM_H
#define ROOM_H

#include <Ogre.h>
#include <string>
#include <deque>
using namespace std;

#include "Tile.h"

class Room
{
	public:
		// Constructors and operators
		Room();
		friend ostream& operator<<(ostream& os, Room *r);
		friend istream& operator>>(istream& is, Room *r);

		void addCoveredTile(Tile* t);
		void removeCoveredTile(Tile* t);
		Tile* getCoveredTile(int index);
		unsigned int numCoveredTiles();
		void clearCoveredTiles();

		void createMeshes();
		void destroyMeshes();
		void deleteYourself();

		int HP;
		Player *controllingPlayer;
		string name, meshName;
		int color;
		sem_t meshCreationFinishedSemaphore;
		sem_t meshDestructionFinishedSemaphore;
		
	private:
		vector<Tile*> coveredTiles;
};

#endif

