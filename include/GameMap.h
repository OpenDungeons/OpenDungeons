#ifndef GAMEMAP_H
#define GAMEMAP_H

#include "Tile.h"
#include "Creature.h"

typedef map< pair<int,int>, Tile*> TileMap_t;

class GameMap
{
	public:
		void createNewMap(int xSize, int ySize);
		void clearAll();
		void clearTiles();
		void clearCreatures();
		void clearClasses();
		void createAllEntities();
		Tile* getTile(int x, int y);
		TileMap_t::iterator firstTile();
		TileMap_t::iterator lastTile();
		Creature* getClassDescription(int index);
		unsigned int numTiles();
		void addTile(Tile *t);
		void addCreature(Creature *c);
		void addClassDescription(Creature c);
		void addClassDescription(Creature *c);
		Creature* getClass(string query);
		unsigned int numCreatures();
		unsigned int numClassDescriptions();
		Creature* getCreature(int index);
		Creature* getCreature(string cName);
		void doTurn();

		list<Tile*> path(int x1, int y1, int x2, int y2);
		vector<Tile*> neighborTiles(int x, int y);

		unsigned int numCreaturesInHand();
		Creature *getCreatureInHand(int i);
		void addCreatureToHand(Creature *c);
		void removeCreatureFromHand(int i);

	private:
		map< pair<int,int>, Tile*> tiles;
		vector<Creature*> classDescriptions;
		vector<Creature*> creatures;
		vector<Creature*> creaturesInHand;
};

class astarEntry
{
	public:
		Tile *tile;
		astarEntry *parent;
		double g, h;
		double fCost()	{return g+h;}
};

#endif

