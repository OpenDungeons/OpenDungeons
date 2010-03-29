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
		enum RoomType {nullRoomType = 0, dungeonTemple, vein, quarters};

		// Constructors and operators
		Room();
		Room(RoomType nType, const vector<Tile*> &nCoveredTiles, int nColor);
		static string getFormat();
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

		static string getMeshNameFromRoomType(RoomType t);

		int HP;
		Player *controllingPlayer;
		string name, meshName;
		int color;
		sem_t meshCreationFinishedSemaphore;
		sem_t meshDestructionFinishedSemaphore;

		// Functions which can be overridden by child classes.
		virtual void doUpkeep();
		
	private:
		vector<Tile*> coveredTiles;
};

#endif

