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
		Room();

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
		
	private:
		vector<Tile*> coveredTiles;
};

#endif

