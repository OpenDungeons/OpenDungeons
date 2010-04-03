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
		// When room types are added to this enum they also need to be added to the switch statements in Room.cpp.
		enum RoomType {nullRoomType = 0, dungeonTemple, vein, quarters};

		// Constructors and operators
		Room();
		static Room* CreateRoom(RoomType nType, const vector<Tile*> &nCoveredTiles, int nColor);

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

		// Public data members
		int HP;
		Player *controllingPlayer;
		string name, meshName;
		int color;

		// Functions which can be overridden by child classes.
		virtual void doUpkeep();
		
	private:
		vector<Tile*> coveredTiles;
};

#include "RoomQuarters.h"

#endif

