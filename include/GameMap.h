#ifndef GAMEMAP_H
#define GAMEMAP_H

#include <semaphore.h>

#include "Tile.h"
#include "Creature.h"
#include "Player.h"
#include "Room.h"
#include "Seat.h"
#include "MapLight.h"
#include "ProtectedObject.h"

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
		GameMap();

		void createNewMap(int xSize, int ySize);
		void createAllEntities();
		void clearAll();

		// Game state methods
		void clearTiles();
		void addTile(Tile *t);
		Tile* getTile(int x, int y);
		TileMap_t::iterator firstTile();
		TileMap_t::iterator lastTile();
		unsigned int numTiles();
		vector<Tile*> rectangularRegion(int x1, int y1, int x2, int y2);
		vector<Tile*> circularRegion(int x, int y, double radius);
		vector<Tile*> tilesBorderedByRegion(const vector<Tile*> &region);

		void clearCreatures();
		void addCreature(Creature *c);
		void removeCreature(Creature *c);
		void queueCreatureForDeletion(Creature *c);
		Creature* getCreature(int index);
		Creature* getCreature(string cName);
		unsigned int numCreatures();

		void clearClasses();
		void addClassDescription(CreatureClass c);
		void addClassDescription(CreatureClass *c);
		CreatureClass* getClassDescription(int index);
		CreatureClass* getClassDescription(string query);
		unsigned int numClassDescriptions();

		void clearPlayers();
		bool addPlayer(Player *p);
		Player* getPlayer(int index);
		Player* getPlayer(string cName);
		Player* getPlayerByColour(int colour);
		unsigned int numPlayers();

		void clearRooms();
		void addRoom(Room *r);
		Room* getRoom(int index);
		unsigned int numRooms();

		void clearMapLights();
		void clearMapLightIndicators();
		void addMapLight(MapLight *m);
		MapLight* getMapLight(int index);
		MapLight* getMapLight(string name);
		unsigned int numMapLights();

		void clearEmptySeats();
		void addEmptySeat(Seat *s);
		Seat* getEmptySeat(int index);
		Seat* popEmptySeat();
		unsigned int numEmptySeats();

		void clearFilledSeats();
		void addFilledSeat(Seat *s);
		Seat* getFilledSeat(int index);
		Seat* popFilledSeat();
		unsigned int numFilledSeats();

		Seat* getSeatByColor(int color);

		void addWinningSeat(Seat *s);
		Seat* getWinningSeat(unsigned int index);
		unsigned int getNumWinningSeats();
		bool seatIsAWinner(Seat *s);

		void addGoalForAllSeats(Goal *g);
		Goal* getGoalForAllSeats(unsigned int i);
		unsigned int numGoalsForAllSeats();
		void clearGoalsForAllSeats();

		// AI Methods
		void doTurn();

		bool walkablePathExists(int x1, int y1, int x2, int y2);
		list<Tile*> path(int x1, int y1, int x2, int y2, Tile::TileClearType passability);
		vector<Tile*> neighborTiles(int x, int y);
		vector<Tile*> neighborTiles(Tile *t);
		list<Tile*> lineOfSight(int x1, int y1, int x2, int y2);
		bool pathIsClear(list<Tile*> path, Tile::TileClearType passability);
		void cutCorners(list<Tile*> &path, Tile::TileClearType passability);
		double crowDistance(int x1, int x2, int y1, int y2);
		//double manhattanDistance(int x1, int x2, int y1, int y2);

		int uniqueFloodFillColor();
		unsigned int doFloodFill(int startX, int startY, Tile::TileClearType passability = Tile::walkableTile, int color = -1);
		void disableFloodFill();
		void enableFloodFill();

		Player *me;
		string nextLevel;
		bool loadNextLevel;
		double averageAILeftoverTime;

		// Overloaded method declarations (these just make it easier to call the above functions)
		list<Tile*> path(Creature *c1, Creature *c2, Tile::TileClearType passability);
		list<Tile*> path(Tile *t1, Tile *t2, Tile::TileClearType passability);
		double crowDistance(Creature *c1, Creature *c2);
		deque<double> previousLeftoverTimes;

		void threadLockForTurn(long int turn);
		void threadUnlockForTurn(long int turn);

	private:
		// Private functions
		void processDeletionQueues();

		// Private datamembers
		map< pair<int,int>, Tile*> tiles;
		sem_t tilesLockSemaphore;
		vector<CreatureClass*> classDescriptions;
		vector<Creature*> creatures;
		sem_t creaturesLockSemaphore;  //TODO: Most of these other vectors should also probably have semaphore locks on them.
		vector<Player*> players;
		vector<Room*> rooms;
		vector<MapLight*> mapLights;
		vector<Seat*> emptySeats;
		vector<Seat*> filledSeats;
		vector<Seat*> winningSeats;
		vector<Goal*> goalsForAllSeats;
		int nextUniqueFloodFillColor;
		bool floodFillEnabled;

		map<long int, ProtectedObject<unsigned int> > threadReferenceCount;
		map<long int, vector<Creature*> > creaturesToDelete;
		sem_t threadReferenceCountLockSemaphore;

		unsigned int numCallsTo_path;
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
		void setHeuristic(int x1, int y1, int x2, int y2)
		{
			h = fabs((double)(x2-x1)) + fabs((double)(y2-y1));
		}
};

#endif

