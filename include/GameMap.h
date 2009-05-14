#ifndef GAMEMAP_H
#define GAMEMAP_H

#include "Tile.h"
#include "Creature.h"

typedef map< pair<int,int>, Tile*> TileMap_t;

/*! \brief The class which stores the entire game state on the server and a subset of this on each client.
 *
 * This class is one of the key classes in the OpenDungeons game.  The map
 * itself, consisting of tiles and rooms, as well as any creatures and items in
 * the game are managed by an instance of the game map.  The game map can also
 * be queried by other subroutines to answer basic questions like "what is the
 * sortest path between two tiles" or "what creatures are in some particular
 * tile".
 */
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

/*! \brief A helper class for the A* search in the GameMap::path function.
 *
 * This class stores the requesite information about a tile which is placed in
 * the search queue for the A-star, or A*, algorithm which is used to
 * calculate paths in the path function.
 */
class astarEntry
{
	public:
		Tile *tile;
		astarEntry *parent;
		double g, h;
		double fCost()	{return g+h;}
};

#endif

