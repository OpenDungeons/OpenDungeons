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

/*! \brief The class which stores the entire game state on the server and a subset of this on each client.
 *
 * This class is one of the key classes in the OpenDungeons game.  The map
 * itself, consisting of tiles and rooms, as well as any creatures and items in
 * the game are managed by an instance of the game map.  The game map can also
 * be queried by other subroutines to answer basic questions like "what is the
 * sortest path between two tiles" or "what creatures are in some particular
 * tile".
 */
class GameMap : public TileContainer
{

friend class MiniMap;
friend class RenderManager;

public:
    GameMap();
    ~GameMap();

    //! \brief Load a level file (Part of the resource paths)
    //! \returns whether the file loaded correctly
    bool LoadLevel(const std::string& levelFilepath);

    //! \brief Setup the map memory to fit the given size.
    //! This methods also puts default (dirt) tiles on the new map.
    //! \returns whether the map could be created.
    bool createNewMap(int sizeX, int sizeY);

    //! \brief Set every tiles fullness and neighbors list
    //! Used when loading a map to setup the initial tile state.
    void setAllFullnessAndNeighbors();

    //! \brief Creates meshes for all the tiles, creatures, rooms, traps and lights stored in this GameMap.
    void createAllEntities();

    //! \brief Destroyes meshes for all the tiles, creatures, rooms, traps and lights stored in this GameMap.
    void destroyAllEntities();

    //! \brief Clears the mesh and deletes the data structure for all the tiles, creatures, classes, and players in the GameMap.
    void clearAll();

    MiniMap* getMiniMap()
    { return miniMap; }

    //! \brief Clears the mesh and deletes the data structure for all the creatures in the GameMap.
    void clearCreatures();

    //! \brief Tells whether the entity name already exists.
    //! \note Ogre entity names must be unique.
    bool doesCreatureNameExist(const std::string& entity_name);

    //! \brief Adds the address of a new creature to be stored in this GameMap.
    void addCreature(Creature *c);

    //! \brief Removes the creature from the game map but does not delete its data structure.
    void removeCreature(Creature *c);

    /** \brief Adds the given creature to the queue of creatures to be deleted in a future turn
     * when it is safe to do so after all references to the creature have been cleared.
     */
    void queueCreatureForDeletion(Creature *c);

    //! \brief Returns a pointer to the creature whose name matches name or number.
    Creature* getCreature(int index);
    const Creature* getCreature(int index) const;
    Creature* getCreature(const std::string& cName);
    const Creature* getCreature(const std::string& cName) const;

    //! \brief Returns the total number of creatures stored in this game map.
    unsigned int numCreatures() const;

    //! \brief Returns a vector containing all the creatures controlled by the given seat.
    std::vector<Creature*> getCreaturesByColor(int color);

    //! \brief Animated objects related functions.
    void clearAnimatedObjects();
    void addAnimatedObject(MovableGameEntity *a);
    void removeAnimatedObject(MovableGameEntity *a);
    MovableGameEntity* getAnimatedObject(int index);
    MovableGameEntity* getAnimatedObject(std::string name);
    unsigned int numAnimatedObjects();

    void addActiveObject(GameEntity* a);
    void removeActiveObject(GameEntity* a);

    //! \brief Deletes the data structure for all the creature classes in the GameMap.
    void clearClasses();

    /*! \brief Adds the address of a new class description to be stored in this GameMap.
     *
     * The class descriptions take the form of a creature data structure with most
     * of the data members filled in.  This class structure is then copied into the
     * data structure of new creatures that are added who are of this class.  The
     * copied members include things like HP, mana, etc, that are the same for all
     * members of that class.  Creature specific things like location, etc. are
     * then filled out for the individual creature.
     */
    void addClassDescription(CreatureDefinition c);
    void addClassDescription(CreatureDefinition *c);

    //! \brief Returns a pointer to the first class description whose 'name' or index parameter matches the query.
    CreatureDefinition* getClassDescription(int index);
    CreatureDefinition* getClassDescription(std::string query);

    //! \brief Returns the total number of class descriptions stored in this game map.
    unsigned int numClassDescriptions();

    //! \brief Calls the deleteYourself() method on each of the rooms in the game map as well as clearing the vector of stored rooms.
    void clearRooms();

    //! \brief A simple mutator method to add the given Room to the GameMap.
    void addRoom(Room *r);

    void removeRoom(Room *r);

    //! \brief A simple accessor method to return the given Room.
    Room* getRoom(int index);

    //! \brief A simple accessor method to return the number of Rooms stored in the GameMap.
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

    //! \brief Traps related functions.
    void clearTraps();
    void addTrap(Trap *t);
    void removeTrap(Trap *t);
    Trap* getTrap(int index);
    unsigned int numTraps();

    void clearMissileObjects();
    void addMissileObject(MissileObject *m);
    void removeMissileObject(MissileObject *m);
    MissileObject* getMissileObject(int index);
    unsigned int numMissileObjects();

    //! \brief Map Lights related functions.
    void clearMapLights();
    void clearMapLightIndicators();
    void addMapLight(MapLight *m);
    void removeMapLight(MapLight *m);
    MapLight* getMapLight(int index);
    MapLight* getMapLight(std::string name);
    unsigned int numMapLights();

    //! \brief Deletes the data structure for all the players in the GameMap.
    void clearPlayers();

    //! \brief Adds a pointer to a player structure to the players stored by this GameMap.
    bool addPlayer(Player *p);

    //! \brief Assigns an ai to the chosen player
    bool assignAI(Player& player, const std::string& aiType, const std::string& parameters = std::string());

    //! \brief Returns a pointer to the i'th player structure stored by this GameMap.
    Player* getPlayer(int index);
    const Player* getPlayer(int index) const;

    //! \brief Returns a pointer to the player structure stored by this GameMap whose name matches pName.
    Player* getPlayer(const std::string& cName);
    const Player* getPlayer(const std::string& cName) const;

    //! \brief Returns the number of player structures stored in this GameMap.
    unsigned int numPlayers() const;

    //! \brief A simple mutator method to clear the vector of empty Seats stored in the GameMap.
    void clearEmptySeats();

    //! \brief A simple mutator method to add another empty Seat to the GameMap.
    void addEmptySeat(Seat* s);

    //! \brief A simple accessor method to return the given Seat.
    Seat* getEmptySeat(int index);
    const Seat* getEmptySeat(int index) const;

    //! \brief Removes the first empty Seat from the GameMap and returns a pointer to it,
    //! this is used when a Player "sits down" at the GameMap.
    Seat* popEmptySeat();

    //! \brief A simple accessor method to return the number of empty Seats on the GameMap.
    unsigned int numEmptySeats() const;

    void clearFilledSeats();
    void addFilledSeat(Seat *s);

    //! \brief A simple accessor method to return the given filled Seat.
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

    int getTotalGoldForColor(int color);
    int withdrawFromTreasuries(int gold, int color);

    inline const unsigned int getMaxAIThreads() const
    { return maxAIThreads; }

    inline void setMaxAIThreads(const unsigned int maxThreads)
    { maxAIThreads = maxThreads; }

    inline void setCullingManger(CullingManager* tempCulm)
    { culm = tempCulm; }

    inline const std::string& getLevelFileName() const
    { return levelFileName; }

    inline void setLevelFileName(const std::string& nlevelFileName)
    { levelFileName = nlevelFileName; }

    inline const std::string& getCreatureDefinitionFileName() const
    { return creatureDefinitionFilename; }

    inline void setCreatureDefinitionFileName(const std::string& defFileName)
    { creatureDefinitionFilename = defFileName; }

    void createTilesMeshes(void);
    void hideAllTiles(void);

    //! \brief Loops over all the creatures and calls their individual doTurn methods,
    //! also check goals and do the upkeep.
    void doTurn();

    void doPlayerAITurn(double frameTime);

    //! \brief Tells whether a path exists between two corrdinate points with the given passability.
    bool pathExists(int x1, int y1, int x2, int y2, Tile::TileClearType passability, int color = 0);

    /*! \brief Calculates the walkable path between tiles (x1, y1) and (x2, y2).
     *
     * The search is carried out using the A-star search algorithm.
     * The path returned contains both the starting and ending tiles, and consists
     * entirely of tiles which satify the 'passability' criterion specified in the
     * search.  The returned tiles are also a "manhattan path" meaning that every
     * successive tile is one of the 4 nearest neighbors of the previous tile in
     * the path.  In most cases you will want to call GameMap::cutCorners on the
     * returned path to shorten the number of steps on the path, as well as the
     * actual walking distance along the path.
     * \param color The color param is used when searching a diggable path to know
     * what tile actually diggable for the given team color.
     */
    std::list<Tile*> path(int x1, int y1, int x2, int y2,
                          Tile::TileClearType passability, int color = 0);
    std::list<Tile*> path(Creature *c1, Creature *c2, Tile::TileClearType passability, int color = 0);
    std::list<Tile*> path(Tile *t1, Tile *t2, Tile::TileClearType passability, int color = 0);

    /*! \brief Returns a list of valid tiles along a straight line from (x1, y1) to (x2, y2), NOTE: in spite of
     * the name, you do not need to be able to see through the tiles returned by this method.
     *
     * This algorithm is from
     * http://en.wikipedia.org/w/index.php?title=Bresenham%27s_line_algorithm&oldid=295047020
     * A more detailed description of how it works can be found there.
     */
    std::list<Tile*> lineOfSight(int x1, int y1, int x2, int y2);

    //! \brief Returns the tiles visible from the given start tile out to the specified sight radius.
    std::vector<Tile*> visibleTiles(Tile *startTile, double sightRadius);

    //! \brief Loops over the visibleTiles and returns any creatures in those tiles whose color matches (or if invert is true, does not match) the given color parameter.
    std::vector<GameEntity*> getVisibleForce(std::vector<Tile*> visibleTiles, int color, bool invert);

    //! \brief Determines whether or not you can travel along a path.
    bool pathIsClear(std::list<Tile*> path, Tile::TileClearType passability);

    //! \brief Loops over a path an replaces 'manhattan' paths with 'as the crow flies' paths.
    void cutCorners(std::list<Tile*> &path, Tile::TileClearType passability);

    /** \brief Returns the as the crow flies distance between tiles located at the two coordinates given.
     * If tiles do not exist at these locations the function returns -1.0.
     */
    Ogre::Real crowDistance(int x1, int x2, int y1, int y2);
    Ogre::Real crowDistance(Tile *t1, Tile *t2);
    Ogre::Real crowDistance(Creature *c1, Creature *c2);

    //! \brief Starts at the tile at the given coordinates and paints outward over all the tiles
    //! whose passability matches the passability of the seed tile.
    //! TODO: What is this used for?
    unsigned int doFloodFill(int startX, int startY,
                             Tile::TileClearType passability = Tile::walkableTile,
                             int color = -1);

    //! \brief Temporarily disables the flood fill computations on this game map.
    void disableFloodFill()
    { floodFillEnabled = false; }

    /** \brief Re-enables the flood filling on the game map, also recomputes the painting on the
     * whole map since the passabilities may have changed since the flood filling was disabled.
     */
    void enableFloodFill();

    inline Player* getLocalPlayer()
    { return me; }

    inline const Player* getLocalPlayer() const
    { return me; }

    //! \brief Updates the different entities animations.
    void updateAnimations(Ogre::Real timeSinceLastFrame);

    //! \brief Increments a semaphore for the given turn indicating how many outstanding references
    //! to game asssets have been copied by other functions.
    void threadLockForTurn(long int turn);

    /** \brief Decrements a semaphore for the given turn indicating how many outstanding references to game asssets there are,
     * when this reaches 0 the turn can be safely retired and assets queued for deletion then can be safely deleted.
     */
    void threadUnlockForTurn(long int turn);

    mutable sem_t creaturesLockSemaphore;
    Player *me;

    CullingManager* culm;

    unsigned long int miscUpkeepTime, creatureTurnsTime;

    static sem_t mCreatureAISemaphore;
    std::vector<Creature*> creatures;
    static ProtectedObject<long int> turnNumber;

 private:
    //! \brief The corresponding minimap.
    MiniMap* miniMap;

    //! \brief Level related filenames.
    std::string levelFileName;
    std::string creatureDefinitionFilename;

    //! \brief The creature definition data
    std::vector<boost::shared_ptr<CreatureDefinition> > classDescriptions;

    //Mutable to allow locking in const functions.
    //TODO: Most of these other vectors should also probably have semaphore locks on them.
    std::vector<MovableGameEntity*> animatedObjects;
    sem_t animatedObjectsLockSemaphore;
    sem_t activeObjectsLockSemaphore;
    sem_t newActiveObjectsLockSemaphore;

    //! \brief Map Entities
    std::vector<Room*> rooms;
    std::vector<Trap*> traps;
    std::vector<MapLight*> mapLights;
    std::vector<MissileObject*> missileObjects;

    //! \brief Players and available game player slots (Seats)
    std::vector<Player*> players;
    std::vector<Seat*> emptySeats;
    std::vector<Seat*> filledSeats;
    std::vector<Seat*> winningSeats;

    //! \brief Common player goals
    std::vector<Goal*> goalsForAllSeats;

    //! \brief Tells whether the map color flood filling is enabled.
    bool floodFillEnabled;

    std::vector<GameEntity*> activeObjects;

    //! \brief  active objects that are created by other active object, i.e. : cannon balls
    std::queue<GameEntity*> newActiveObjects;

    std::map<long int, ProtectedObject<unsigned int> > threadReferenceCount;
    std::map<long int, std::vector<Creature*> > creaturesToDelete;
    sem_t threadReferenceCountLockSemaphore;

    //! \brief Debug member used to know how many call to pathfinding has been made within the same turn.
    unsigned int numCallsTo_path;

    unsigned int maxAIThreads;

    TileCoordinateMap* tileCoordinateMap;

    //! AI Handling manager
    AIManager aiManager;

    void processDeletionQueues();

    //! \brief Tells whether a path is existing between the two coordinate points.
    bool walkablePathExists(int x1, int y1, int x2, int y2);

    //! \brief Updates different entities states.
    //! Updates goals, count each team Workers, gold, mana and claimed tiles.
    unsigned long int doMiscUpkeep();

    //! \brief Updates current creature actions
    unsigned long int doCreatureTurns();

    //! THREAD - Those functions are meant to be called in pthread_create.
    //! Thus, They are different threads.
    static void *creatureDoTurnHelperThread(void *p);
};

#endif

