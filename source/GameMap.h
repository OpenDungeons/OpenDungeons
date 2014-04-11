/*!
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GAMEMAP_H
#define GAMEMAP_H

#include "MiniMap.h"
#include "AIManager.h"
#include "Room.h"
#include "TileContainer.h"
#include "TileContainersModificator.h"

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32

#include <boost/shared_ptr.hpp>

#include <semaphore.h>
#include <map>
#include <string>

class Tile;
class TileCoordinateMap;
class Creature;
class Player;
class Trap;
class Seat;
class Goal;
class MapLight;
template<typename T> class ProtectedObject;
class MissileObject;
class MovableGameEntity;

class MiniMap;
class CullingManager;

typedef Tile** TileMap_t;
/* typedef std::map<std::pair<int, int> , Tile*> TileMap_t; */

/*! \brief The class which stores the entire game state on the server and a subset of this on each client.
 *
 * This class is one of the key classes in the OpenDungeons game.  The map
 * itself, consisting of tiles and rooms, as well as any creatures and items in
 * the game are managed by an instance of the game map.  The game map can also
 * be queried by other subroutines to answer basic questions like "what is the
 * sortest path between two tiles" or "what creatures are in some particular
 * tile".
 */
class GameMap : public TileContainer,  public TileContainersModificator
{
friend class MiniMap;
friend class RenderManager;
 public:
  GameMap();
  ~GameMap();

  int setAllNeighbors();

  //! \brief Adds the missing tiles in a map to fit its dimensions.
  void createNewMap();

  //! \brief Load a level file (Part of the resource paths)
  //! \returns whether the file loaded correctly
  bool LoadLevel(const std::string& levelFilepath);

  void createAllEntities();
  void destroyAllEntities();
  void clearAll();
  MiniMap* getMiniMap(){ return miniMap; }


  void clearCreatures();
  void addCreature(Creature *c);
  void removeCreature(Creature *c);
  void queueCreatureForDeletion(Creature *c);
  Creature* getCreature(int index);
  const Creature* getCreature(int index) const;
  Creature* getCreature(std::string cName);
  const Creature* getCreature(std::string cName) const;

  unsigned int numCreatures() const;
  std::vector<Creature*> getCreaturesByColor(int color);

  void clearAnimatedObjects();
  void addAnimatedObject(MovableGameEntity *a);
  void removeAnimatedObject(MovableGameEntity *a);
  MovableGameEntity* getAnimatedObject(int index);
  MovableGameEntity* getAnimatedObject(std::string name);
  unsigned int numAnimatedObjects();

  void addActiveObject(GameEntity* a);
  void removeActiveObject(GameEntity* a);

  void clearClasses();
  void addClassDescription(CreatureDefinition c);
  void addClassDescription(CreatureDefinition *c);
  CreatureDefinition* getClassDescription(int index);
  CreatureDefinition* getClassDescription(std::string query);
  unsigned int numClassDescriptions();

  void clearPlayers();
  bool addPlayer(Player *p);
  bool assignAI(Player& player, const std::string& aiType, const std::string& parameters = "");
  Player* getPlayer(int index);
  const Player* getPlayer(int index) const;
  Player* getPlayer(const std::string& cName);
  const Player* getPlayer(const std::string& cName) const;
  unsigned int numPlayers() const;

  void clearRooms();
  void addRoom(Room *r);
  void removeRoom(Room *r);
  Room* getRoom(int index);
  unsigned int numRooms();
  std::vector<Room*> getRoomsByType(Room::RoomType type);
  std::vector<Room*> getRoomsByTypeAndColor(Room::RoomType type,
					    int color);
  std::vector<const Room*> getRoomsByTypeAndColor(Room::RoomType type,
						  int color) const;
  unsigned int numRoomsByTypeAndColor(Room::RoomType type,
				      int color) const;
  std::vector<Room*> getReachableRooms(const std::vector<Room*> &vec,
				       Tile *startTile, Tile::TileClearType passability);

  void clearTraps();
  void addTrap(Trap *t);
  void removeTrap(Trap *t);
  Trap* getTrap(int index);
  unsigned int numTraps();

  int getTotalGoldForColor(int color);
  int withdrawFromTreasuries(int gold, int color);

  void clearMapLights();
  void clearMapLightIndicators();
  void addMapLight(MapLight *m);
  void removeMapLight(MapLight *m);
  MapLight* getMapLight(int index);
  MapLight* getMapLight(std::string name);
  unsigned int numMapLights();

  void clearEmptySeats();
  void addEmptySeat(Seat *s);
  Seat* getEmptySeat(int index);
  const Seat* getEmptySeat(int index) const;
  Seat* popEmptySeat();
  unsigned int numEmptySeats() const;

  void clearFilledSeats();
  void addFilledSeat(Seat *s);
  Seat* getFilledSeat(int index);
  const Seat* getFilledSeat(int index) const;
  Seat* popFilledSeat();
  unsigned int numFilledSeats() const;

  Seat* getSeatByColor(int color);

  void addWinningSeat(Seat *s);
  Seat* getWinningSeat(unsigned int index);
  unsigned int getNumWinningSeats();
  bool seatIsAWinner(Seat *s);

  void addGoalForAllSeats(Goal *g);
  Goal* getGoalForAllSeats(unsigned int i);
  const Goal* getGoalForAllSeats(unsigned int i) const;
  unsigned int numGoalsForAllSeats() const;
  void clearGoalsForAllSeats();

  void clearMissileObjects();
  void addMissileObject(MissileObject *m);
  void removeMissileObject(MissileObject *m);
  MissileObject* getMissileObject(int index);
  unsigned int numMissileObjects();

  inline const int maxX() const {return width - 1;}
  inline const int maxY() const {return length - 1;}

  inline const unsigned int getMaxAIThreads() const {return maxAIThreads;}
  inline void setMaxAIThreads(const unsigned int maxThreads) {maxAIThreads = maxThreads;}
  inline void setCullingManger(CullingManager* tempCulm){culm = tempCulm;}

  inline const std::string& getLevelFileName() const {return levelFileName;}
  inline void setLevelFileName(const std::string& nlevelFileName) {levelFileName = nlevelFileName;}
  void createTilesMeshes(void);
  void hideAllTiles(void);
  // AI Methods
  void doTurn();
  void doPlayerAITurn(double frameTime);

  bool pathExists(int x1, int y1, int x2, int y2,
		  Tile::TileClearType passability);
  std::list<Tile*> path(int x1, int y1, int x2, int y2,
			Tile::TileClearType passability);

  std::list<Tile*> lineOfSight(int x1, int y1, int x2, int y2);
  std::vector<Tile*> visibleTiles(Tile *startTile, double sightRadius);
  std::vector<GameEntity*> getVisibleForce(
					   std::vector<Tile*> visibleTiles, int color, bool invert);
  bool
    pathIsClear(std::list<Tile*> path, Tile::TileClearType passability);
  void
    cutCorners(std::list<Tile*> &path, Tile::TileClearType passability);
  Ogre::Real crowDistance(int x1, int x2, int y1, int y2);
  Ogre::Real crowDistance(Tile *t1, Tile *t2);
  //double manhattanDistance(int x1, int x2, int y1, int y2);

  int uniqueFloodFillColor();
  unsigned int doFloodFill(int startX, int startY,
			   Tile::TileClearType passability = Tile::walkableTile,
			   int color = -1);
  void disableFloodFill();
  void enableFloodFill();

  inline Player* getLocalPlayer()
  { return me; }

  inline const Player* getLocalPlayer() const
  { return me; }

  mutable sem_t creaturesLockSemaphore;
  Player *me;
  CullingManager* culm;
  std::string nextLevel;
  bool loadNextLevel;
  double averageAILeftoverTime;
  static Tile::TileType*  neighborType; // should be of 8 objects
  static bool*  neighborFullness; // should be of 8 objects

  // Overloaded method declarations (these just make it easier to call the above functions)
  std::list<Tile*> path(Creature *c1, Creature *c2,
			Tile::TileClearType passability);
  std::list<Tile*> path(Tile *t1, Tile *t2,
			Tile::TileClearType passability);
  Ogre::Real crowDistance(Creature *c1, Creature *c2);
  std::deque<double> previousLeftoverTimes;

  void threadLockForTurn(long int turn);
  void threadUnlockForTurn(long int turn);

  // Public data members
  unsigned long int miscUpkeepTime, creatureTurnsTime;


  static sem_t mCreatureAISemaphore;
  std::vector<Creature*> creatures;
  static ProtectedObject<long int> turnNumber;

 private:
  void processDeletionQueues();
  bool walkablePathExists(int x1, int y1, int x2, int y2);
  Tile* auxTilesArray;

  // THREAD - This function is meant to be called by pthread_create.
  static void *miscUpkeepThread(void *p);
  static void *creatureDoTurnThread(void *p);
  static void *creatureDoTurnHelperThread(void *p);



  class CDTHTStruct
  {
  public:
    int numCreatures;
    Creature **creatures;
  };

  unsigned long int doMiscUpkeep();
  unsigned long int doCreatureTurns();

  // Private data members
  MiniMap *miniMap;
  int iteration_doFloodFill;

  std::string levelFileName;

  std::vector<boost::shared_ptr<CreatureDefinition> > classDescriptions;

  //Mutable to allow locking in const functions.
  //TODO: Most of these other vectors should also probably have semaphore locks on them.
  std::vector<MovableGameEntity*> animatedObjects;
  sem_t animatedObjectsLockSemaphore;
  sem_t activeObjectsLockSemaphore;
  sem_t newActiveObjectsLockSemaphore;
  std::vector<Player*> players;
  std::vector<Room*> rooms;
  std::vector<Trap*> traps;
  std::vector<MapLight*> mapLights;
  std::vector<Seat*> emptySeats;
  std::vector<Seat*> filledSeats;
  std::vector<Seat*> winningSeats;
  std::vector<Goal*> goalsForAllSeats;
  std::vector<MissileObject*> missileObjects;
  int nextUniqueFloodFillColor;
  bool floodFillEnabled;
  std::vector<GameEntity*> activeObjects;
  std::queue<GameEntity*> newActiveObjects; // active objects that are created by other active object, i.e. : cannon balls

  std::map<long int, ProtectedObject<unsigned int> > threadReferenceCount;
  std::map<long int, std::vector<Creature*> > creaturesToDelete;
  sem_t threadReferenceCountLockSemaphore;

  unsigned int numCallsTo_path;
  unsigned int maxAIThreads;

  TileCoordinateMap *tileCoordinateMap;

  int length, width;

  AIManager aiManager;

  /*! \brief A helper class for the A* search in the GameMap::path function.
   *
   * This class stores the requesite information about a tile which is placed in
   * the search queue for the A-star, or A*, algorithm which is used to
   * calculate paths in the path function.
   */
  class AstarEntry
  {
  public:
  AstarEntry() :
    tile    (0),
      parent  (0),
      g       (0),
      h       (0)
	{}

  AstarEntry(Tile* tile, int x1, int y1, int x2, int y2) :
    tile    (tile),
      parent  (0),
      g       (0),
      h       (0)
	{
	  setHeuristic(x1, y1, x2, y2);
	}

    void setHeuristic(const int& x1, const int& y1, const int& x2, const int& y2)
    {
      h = fabs(static_cast<double>(x2 - x1)) + fabs(static_cast<double>(y2 - y1));
    }


    inline double           fCost       () const                {return g + h;}

    inline Tile*            getTile     () const                {return tile;}
    inline void             setTile     (Tile* newTile)         {tile = newTile;}




    inline AstarEntry*      getParent   () const                {return parent;}
    inline void             setParent   (AstarEntry* newParent) {parent = newParent;}

    inline const double&    getG        () const                {return g;}
    inline void             setG        (const double& newG)    {g = newG;}

  private:

    Tile*       tile;
    AstarEntry* parent;
    double      g;
    double      h;

  };
};

#endif

