#ifndef ROOM_H
#define ROOM_H

#include <Ogre.h>
#include <string>
#include <deque>
#include <iostream>

#include "ActiveObject.h"
#include "Tile.h"
#include "AttackableObject.h"
#include "RoomObject.h"

class Room : public AttackableObject, public ActiveObject
{
	public:
		// When room types are added to this enum they also need to be added to the switch statements in Room.cpp.
		enum RoomType {nullRoomType = 0, dungeonTemple, vein, quarters, treasury, portal, forge};

		// Constructors and operators
		Room();
		static Room* createRoom(RoomType nType, const std::vector<Tile*> &nCoveredTiles, int nColor);
		static Room* createRoomFromStream(istream &is);
		virtual void absorbRoom(Room *r);

		static string getFormat();
		friend ostream& operator<<(ostream& os, Room *r);
		friend istream& operator>>(istream& is, Room *r);

		virtual void createMeshes();
		virtual void destroyMeshes();
		void loadRoomObject(string meshName, Tile *targetTile = NULL);
		void createRoomObjectMeshes();
		void destroyRoomObjectMeshes();
		void deleteYourself();
		RoomType getType();

		static string getMeshNameFromRoomType(RoomType t);
		static RoomType getRoomTypeFromMeshName(string s);

		static int costPerTile(RoomType t);

		// Public data members
		Player *controllingPlayer;  //TODO:  This should be a controlling seat rather than a player.
		string meshName;
		int color;

		// Functions which can be overridden by child classes.
		virtual bool doUpkeep();
		virtual bool doUpkeep(Room *r);
		
		virtual void addCoveredTile(Tile* t, double nHP = defaultTileHP);
		virtual void removeCoveredTile(Tile* t);
		virtual Tile* getCoveredTile(int index);
		std::vector<Tile*> getCoveredTiles();
		virtual unsigned int numCoveredTiles();
		virtual void clearCoveredTiles();
		virtual bool tileIsPassable(Tile *t);

		Tile* getCentralTile();

		// Methods inherited from AttackableObject.
		//TODO:  Sort these into the proper places in the rest of the file.
		double getHP(Tile *tile);
		double getDefense();
		void takeDamage(double damage, Tile *tileTakingDamage);
		void recieveExp(double experience);
		bool isMobile();
		int getLevel();
		int getColor();
		AttackableObject::AttackableObjectType getAttackableObjectType();

		// Public data members
		std::string name;
		std::string getName() {return name;}

	protected:
		const static double defaultTileHP;// = 10.0;

		std::vector<Tile*> coveredTiles;
		std::map<Tile*,double> tileHP;
		std::map<Tile*,RoomObject*> roomObjects;
		RoomType type;
		bool meshExists;
};

#include "RoomQuarters.h"
#include "RoomTreasury.h"
#include "RoomPortal.h"
#include "RoomDungeonTemple.h"
#include "RoomForge.h"

#endif

