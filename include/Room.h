#ifndef ROOM_H
#define ROOM_H

#include <Ogre.h>
#include <string>
#include <deque>
#include <iostream>
using namespace std;

#include "Tile.h"
#include "AttackableObject.h"

class Room : public AttackableObject
{
	public:
		// When room types are added to this enum they also need to be added to the switch statements in Room.cpp.
		enum RoomType {nullRoomType = 0, dungeonTemple, vein, quarters, treasury, portal};

		// Constructors and operators
		Room();
		static Room* createRoom(RoomType nType, const vector<Tile*> &nCoveredTiles, int nColor);
		static Room* createRoomFromStream(istream &is);
		virtual void absorbRoom(Room *r);

		static string getFormat();
		friend ostream& operator<<(ostream& os, Room *r);
		friend istream& operator>>(istream& is, Room *r);

		void createMeshes();
		void destroyMeshes();
		void deleteYourself();
		RoomType getType();

		static string getMeshNameFromRoomType(RoomType t);
		static RoomType getRoomTypeFromMeshName(string s);

		static int costPerTile(RoomType t);

		// Public data members
		Player *controllingPlayer;  //TODO:  This should be a controlling seat rather than a player.
		string name, meshName;
		int color;

		// Functions which can be overridden by child classes.
		virtual void doUpkeep(Room *r);
		
		virtual void addCoveredTile(Tile* t, double nHP = defaultTileHP);
		virtual void removeCoveredTile(Tile* t);
		virtual Tile* getCoveredTile(int index);
		vector<Tile*> getCoveredTiles();
		virtual unsigned int numCoveredTiles();
		virtual void clearCoveredTiles();

		// Methods inherited from AttackableObject.
		//TODO:  Sort these into the proper places in the rest of the file.
		double getHP(Tile *tile);
		double getDefense();
		void takeDamage(double damage, Tile *tileTakingDamage);
		void recieveExp(double experience);
		bool isMobile();
		int getLevel();
		int getColor();
		string getName();
		AttackableObject::AttackableObjectType getAttackableObjectType();

	protected:
		const static double defaultTileHP = 10.0;

		vector<Tile*> coveredTiles;
		map<Tile*,double> tileHP;
		RoomType type;
		bool meshExists;
};

#include "RoomQuarters.h"
#include "RoomTreasury.h"
#include "RoomPortal.h"

#endif

