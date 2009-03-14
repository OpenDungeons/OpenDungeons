#ifndef GAMEMAP_H
#define GAMEMAP_H

#include "Tile.h"
#include "Creature.h"

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
		Tile* getTile(int index);
		Creature* getClassDescription(int index);
		int numTiles();
		void addTile(Tile *t);
		void addCreature(Creature *c);
		void addClassDescription(Creature c);
		void addClassDescription(Creature *c);
		Creature* getClass(string query);
		int numCreatures();
		int numClassDescriptions();
		Creature* getCreature(int index);
		Creature* getCreature(string cName);
		void doTurn();

		list<Tile*> path(int x1, int y1, int x2, int y2);

		int numCreaturesInHand();
		Creature *getCreatureInHand(int i);
		void addCreatureToHand(Creature *c);
		void removeCreatureFromHand(int i);

	private:
		vector<Tile*> tiles;
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

