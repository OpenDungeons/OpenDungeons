/*!
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#ifndef _GAMEMAP_H_
#define _GAMEMAP_H_

#include "gamemap/TileContainer.h"

#include "ai/AIManager.h"

#ifdef __MINGW32__
#ifndef mode_t
#include <sys/types.h>
#endif //mode_t
#endif //mingw32

#include <map>
#include <string>
#include <cstdint>

class Building;
class Tile;
class Creature;
class Player;
class Trap;
class Seat;
class GameEntity;
class Goal;
class MapLight;
class MovableGameEntity;
class CreatureDefinition;
class Weapon;
class CreatureMood;
class RenderedMovableEntity;
class Room;
class Spell;

enum class GameEntityType;
enum class FloodFillType;
enum class RoomType;

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

friend class RenderManager;
friend class ODServer;

public:
    GameMap(bool isServerGameMap);
    ~GameMap();

    inline const std::string serverStr()
    { return std::string( mIsServerGameMap ? "SERVER - " : "CLIENT - "); }

    //! \brief Tells whether the game map is currently used for the map editor mode
    //! or for a standard game session.
    //! \note This function has got the noticeable role to keep the separation between the client
    //! and the server game maps clean, by not calling client related code when acting
    //! as a server game and vice versa.
    bool isInEditorMode() const;

    //! \brief Load a level file (Part of the resource paths)
    //! \returns whether the file loaded correctly
    bool loadLevel(const std::string& levelFilepath);

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

    //! \brief Clears the mesh and deletes the data structure for all the creatures in the GameMap.
    void clearCreatures();

    //! \brief Adds the address of a new creature to be stored in this GameMap.
    void addCreature(Creature *c);

    //! \brief Removes the creature from the game map but does not delete its data structure.
    void removeCreature(Creature *c);

    /** \brief Adds the given entity to the queue to be deleted at the end of the turn. */
    void queueEntityForDeletion(GameEntity *ge);

    /** \brief Adds the given map light to the queue to be deleted at the end of the turn. */
    void queueMapLightForDeletion(MapLight *ml);

    //! \brief Returns a pointer to the creature whose name matches name or
    //! nullptr if it is not found
    Creature* getCreature(const std::string& cName) const;

    inline bool getIsFOWActivated() const
    { return mIsFOWActivated; }

    //! \brief Returns a vector containing all the creatures controlled by the given seat.
    std::vector<Creature*> getCreaturesByAlliedSeat(Seat* seat);
    std::vector<Creature*> getCreaturesBySeat(Seat* seat);

    inline const std::vector<Creature*>& getCreatures() const
    { return mCreatures; }

    Creature* getWorkerToPickupBySeat(Seat* seat);
    Creature* getFighterToPickupBySeat(Seat* seat);

    //! \brief Animated objects related functions.
    void clearAnimatedObjects();
    void addAnimatedObject(MovableGameEntity *a);
    void removeAnimatedObject(MovableGameEntity *a);
    MovableGameEntity* getAnimatedObject(const std::string& name) const;

    void addActiveObject(GameEntity* a);
    void removeActiveObject(GameEntity* a);

    //! \brief Deletes the data structure for all the creature classes in the GameMap.
    void clearClasses();

    //! \brief Deletes the data structure for all the weapons in the GameMap.
    void clearWeapons();

    /*! \brief Adds the address of a new class description to be stored in this GameMap.
     *
     * The class descriptions take the form of a creature data structure with most
     * of the data members filled in.  This class structure is then copied into the
     * data structure of new creatures that are added who are of this class.  The
     * copied members include things like HP, mana, etc, that are the same for all
     * members of that class.  Creature specific things like location, etc. are
     * then filled out for the individual creature.
     */
    void addClassDescription(const CreatureDefinition *c);

    //! \brief Returns a pointer to the first class description whose 'name' or index parameter matches the query.
    const CreatureDefinition* getClassDescription(int index);
    const CreatureDefinition* getClassDescription(const std::string& className);
    CreatureDefinition* getClassDescriptionForTuning(const std::string& name);

    //! \brief Returns the total number of class descriptions stored in this game map.
    unsigned int numClassDescriptions();

    void saveLevelClassDescriptions(std::ofstream& levelFile);

    void addWeapon(const Weapon* weapon);
    const Weapon* getWeapon(int index);
    const Weapon* getWeapon(const std::string& name);
    Weapon* getWeaponForTuning(const std::string& name);
    uint32_t numWeapons();
    void saveLevelEquipments(std::ofstream& levelFile);

    void clearCreatureMoodModifiers();
    bool addCreatureMoodModifiers(const std::string& name,
        const std::vector<CreatureMood*>& moodModifiers);
    int32_t computeCreatureMoodModifiers(const Creature* creature) const;

    std::vector<GameEntity*> getNaturalEnemiesInList(const Creature* creature, const std::vector<GameEntity*>& reachableAlliedObjects) const;

    //! \brief Calls the deleteYourself() method on each of the rooms in the game map as well as clearing the vector of stored rooms.
    void clearRooms();

    //! \brief Simple mutators method to add/remove the given Room to the GameMap.
    void addRoom(Room *r);
    void removeRoom(Room *r);

    //! \brief A simple accessor method to return the given Room.
    Room* getRoom(int index);

    //! \brief A simple accessor method to return the number of Rooms stored in the GameMap.
    inline const std::vector<Room*>& getRooms() const
    { return mRooms; }

    std::vector<Room*> getRoomsByType(RoomType type);
    std::vector<Room*> getRoomsByTypeAndSeat(RoomType type,
                        Seat* seat);
    std::vector<const Room*> getRoomsByTypeAndSeat(RoomType type,
                          Seat* seat) const;
    unsigned int numRoomsByTypeAndSeat(RoomType type,
                      Seat* seat) const;
    std::vector<Room*> getReachableRooms(const std::vector<Room*> &vec,
                       Tile *startTile, const Creature* creature);
    std::vector<Building*> getReachableBuildingsPerSeat(Seat* seat,
        Tile *startTile, const Creature* creature);
    Room* getRoomByName(const std::string& name);
    Trap* getTrapByName(const std::string& name);

    //! \brief Traps related functions.
    void clearTraps();
    void addTrap(Trap *t);
    void removeTrap(Trap *t);
    inline const std::vector<Trap*>& getTraps() const
    { return mTraps; }

    //! \brief Map Lights related functions.
    void clearMapLights();
    void addMapLight(MapLight *m);
    void removeMapLight(MapLight *m);
    MapLight* getMapLight(const std::string& name) const;
    inline const std::vector<MapLight*>& getMapLights() const
    { return mMapLights; }

    //! \brief Deletes the data structure for all the players in the GameMap.
    void clearPlayers();

    //! \brief Adds a pointer to a player structure to the players stored by this GameMap
    bool addPlayer(Player *player);

    //! \brief Assigns an ai to the chosen player
    bool assignAI(Player& player, const std::string& aiType, const std::string& parameters = std::string());

    //! \brief Returns a pointer to the player structure stored by this GameMap whose name matches pName.
    Player* getPlayer(const std::string& cName) const;

    inline const std::vector<Player*>& getPlayers() const
    { return mPlayers; }

    inline const std::vector<Seat*>& getSeats() const
    { return mSeats; }

    //! \brief Returns a pointer to the player structure stored by this GameMap whose seat id matches seatId.
    Player* getPlayerBySeatId(int seatId) const;
    Player* getPlayerBySeat(Seat* seat) const;

    //! \brief A simple mutator method to clear the vector of Seats stored in the GameMap.
    void clearSeats();

    //! \brief A simple mutator method to add another Seat to the GameMap. Checks if a seat with the same
    //! id is already in the list before inserting. Returns true if the seat was inserted and false
    //! otherwise
    bool addSeat(Seat* s);

    int nextSeatId(int SeatId);

    void clearFilledSeats();
    void clearAiManager();

    Seat* getSeatById(int id) const;

    inline Seat* getSeatRogue() const
    { return getSeatById(0); }

    void addWinningSeat(Seat *s);
    bool seatIsAWinner(Seat *s) const;

    void addGoalForAllSeats(Goal *g);
    inline const std::vector<Goal*>& getGoalsForAllSeats() const
    { return mGoalsForAllSeats; }
    void clearGoalsForAllSeats();

    int getTotalGoldForSeat(Seat* seat);
    bool withdrawFromTreasuries(int gold, Seat* seat);

    inline const std::string& getLevelFileName() const
    { return mLevelFileName; }

    inline void setLevelFileName(const std::string& levelFileName)
    { mLevelFileName = levelFileName; }

    inline const std::string& getLevelName() const
    { return mMapInfoName; }

    inline void setLevelName(const std::string& levelName)
    { mMapInfoName = levelName; }

    inline const std::string& getLevelDescription() const
    { return mMapInfoDescription; }

    inline void setLevelDescription(const std::string& levelDescription)
    { mMapInfoDescription = levelDescription; }

    inline const std::string& getLevelMusicFile() const
    { return mMapInfoMusicFile; }

    inline void setLevelMusicFile(const std::string& levelMusicFile)
    { mMapInfoMusicFile = levelMusicFile; }

    inline const std::string& getLevelFightMusicFile() const
    { return mMapInfoFightMusicFile; }

    inline void setLevelFightMusicFile(const std::string& levelFightMusicFile)
    { mMapInfoFightMusicFile = levelFightMusicFile; }

    void createTilesMeshes(void);
    void hideAllTiles(void);

    std::string getGoalsStringForPlayer(Player* player);

    //! \brief Loops over all the creatures and calls their individual doTurn methods,
    //! also check goals and do the upkeep.
    void doTurn();

    void doPlayerAITurn(double frameTime);

    //! \brief Tells whether a path exists between two tiles for the given creature.
    bool pathExists(const Creature* creature, Tile* tileStart, Tile* tileEnd);

    /*! \brief Calculates the walkable path between tileStart and one of the possibleDests. This function
     * will choose the closest tile in possibleDests and return the path between tileStart and it.
     * If a path is found, it is returned and chosenTile is set to the chosen tile. If no path is found,
     * an empty list will be returned and chosenTile will be set to nullptr
     */
    std::list<Tile*> findBestPath(const Creature* creature, Tile* tileStart, const std::vector<Tile*> possibleDests,
        Tile*& chosenTile);

    /*! \brief Calculates the walkable path between tiles (x1, y1) and (x2, y2).
     *
     * The search is carried out using the A-star search algorithm.
     * The path returned contains both the starting and ending tiles, and consists
     * entirely of tiles which satify the passability criterion specified by the creature
     * definition.  A "manhattan path" is used what means that successive tile is one of
     * the 4 nearest neighbors of the previous tile in the path.
     * When building the path, we check if a diagonal can be used. We consider it can
     * if the creature can go through the 4 tiles.
     * \param seat The seat is used when searching a diggable path to know
     * what tile actually diggable for the given team.
     */
    std::list<Tile*> path(int x1, int y1, int x2, int y2, const Creature* creature, Seat* seat, bool throughDiggableTiles = false);
    std::list<Tile*> path(Creature *c1, Creature *c2, const Creature* creature, Seat* seat, bool throughDiggableTiles = false);
    std::list<Tile*> path(Tile *t1, Tile *t2, const Creature* creature, Seat* seat, bool throughDiggableTiles = false);
    //! \note Returns a path for the given creature to the given destination.
    std::list<Tile*> path(const Creature* creature, Tile* destination, bool throughDiggableTiles = false);

    //! \brief Loops over the visibleTiles and returns any creature/room/trap in those tiles allied with the given seat
    //! (or if enemyForce is true, is not allied)
    std::vector<GameEntity*> getVisibleForce(const std::vector<Tile*>& visibleTiles, Seat* seat, bool enemyForce);

    //! \brief Loops over the visibleTiles and returns any creature in those tiles allied with the given seat.
    //! (or if enemyCreatures is true, is not allied)
    std::vector<GameEntity*> getVisibleCreatures(const std::vector<Tile*>& visibleTiles, Seat* seat, bool enemyCreatures);

    //! \brief Loops over the visibleTiles and returns any carryable entity in those tiles
    std::vector<GameEntity*> getVisibleCarryableEntities(const std::vector<Tile*>& visibleTiles);

    /** \brief Returns the as the crow flies distance between tiles located at the two coordinates given.
     * If tiles do not exist at these locations the function returns -1.0.
     */
    Ogre::Real crowDistance(int x1, int x2, int y1, int y2);
    Ogre::Real crowDistance(Tile *t1, Tile *t2);
    Ogre::Real crowDistance(Creature *c1, Creature *c2);

    //! \brief Returns the squared distance between 2 tiles
    Ogre::Real squaredCrowDistance(Tile *t1, Tile *t2) const;

    //! \brief Checks the neighboor tiles to see if the floodfill can be used. Floodfill consists on tagging all contiguous tiles
    //! to be able to know before computing it if a path exists between 2 tiles. We do that to avoid computing paths when we
    //! already know that no path exists.
    bool doFloodFill(Tile* tile);
    void refreshFloodFill(Tile* tile);

    //! \brief Temporarily disables the flood fill computations on this game map.
    void disableFloodFill()
    { mFloodFillEnabled = false; }

    /** \brief Re-enables the flood filling on the game map, also recomputes the painting on the
     * whole map since the passabilities may have changed since the flood filling was disabled.
     */
    void enableFloodFill();

    inline void setLocalPlayer(Player* player)
    { mLocalPlayer = player; }

    inline Player* getLocalPlayer()
    { return mLocalPlayer; }

    inline const Player* getLocalPlayer() const
    { return mLocalPlayer; }

    inline const std::string& getLocalPlayerNick()
    { return mLocalPlayerNick; }

    inline void setLocalPlayerNick(const std::string& nick)
    { mLocalPlayerNick = nick; }

    //! \brief Updates the different entities animations.
    void updateAnimations(Ogre::Real timeSinceLastFrame);

    inline int64_t getTurnNumber() const
    { return mTurnNumber; }

    inline void setTurnNumber(int64_t turnNumber)
    { mTurnNumber = turnNumber; }

    inline bool isServerGameMap() const
    { return mIsServerGameMap; }

    inline bool getGamePaused() const
    { return mIsPaused; }

    inline void setGamePaused(bool paused)
    { mIsPaused = paused; }

    //! \brief Refresh the tiles borders based a recent change on the map
    void refreshBorderingTilesOf(const std::vector<Tile*>& affectedTiles);

    std::vector<Tile*> getDiggableTilesForPlayerInArea(int x1, int y1, int x2, int y2,
        Player* player);
    std::vector<Tile*> getBuildableTilesForPlayerInArea(int x1, int y1, int x2, int y2,
        Player* player);
    void markTilesForPlayer(std::vector<Tile*>& tiles, bool isDigSet, Player* player);

    int addGoldToSeat(int gold, int seatId);

    //! \brief Returns the number of workers the given seat controls
    int getNbWorkersForSeat(Seat* seat);

    //! \brief Searches for a worker owned by the seat for path finding
    Creature* getWorkerForPathFinding(Seat* seat);

    //! \brief Finds a path for the creature to the best tile within range from target
    //! Returns true and path will contain the path if a path is found.
    //! Returns false otherwise
    bool pathToBestFightingPosition(std::list<Tile*>& pathToTarget, Creature* attackingCreature, Tile* attackedTile);

    //! \brief Returns the closest tile from origin where a GameEntity in listObjects is
    GameEntity* getClosestTileWhereGameEntityFromList(std::vector<GameEntity*> listObjects, Tile* origin, Tile*& attackedTile);

    void logFloodFileTiles();
    void consoleSetCreatureDestination(const std::string& creatureName, int x, int y);
    void consoleDisplayCreatureVisualDebug(const std::string& creatureName, bool enable);
    void consoleDisplaySeatVisualDebug(int seatId, bool enable);
    void consoleSetLevelCreature(const std::string& creatureName, uint32_t level);
    void consoleAskToggleFOW();

    //! \brief This functions create unique names. They check that there
    //! is no entity with the same name before returning
    std::string nextUniqueNameCreature(const std::string& className);
    std::string nextUniqueNameMissileObj(const std::string& baseName);
    std::string nextUniqueNameRoom(const std::string& meshName);
    std::string nextUniqueNameRenderedMovableEntity(const std::string& baseName);
    std::string nextUniqueNameTrap(const std::string& meshName);
    std::string nextUniqueNameMapLight();

    void addRenderedMovableEntity(RenderedMovableEntity *obj);
    void removeRenderedMovableEntity(RenderedMovableEntity *obj);
    RenderedMovableEntity* getRenderedMovableEntity(const std::string& name);
    void clearRenderedMovableEntities();
    void clearActiveObjects();
    GameEntity* getEntityFromTypeAndName(GameEntityType entityType,
        const std::string& entityName);

    //! brief Functions to add/remove/get Spells
    inline const std::vector<Spell*>& getSpells() const
    { return mSpells; }
    void addSpell(Spell *spell);
    void removeSpell(Spell *spell);
    Spell* getSpell(const std::string& name) const;
    void clearSpells();
    std::vector<Spell*> getSpellsBySeatAndType(Seat* seat, SpellType type) const;

    //! \brief Tells the game map a given player is attacking or under attack.
    //! Used on the server game map only. tile represents the tile where the fight is
    //! happening
    void playerIsFighting(Player* player, Tile* tile);

    //! \brief Deletes the entities that have been marked to delete. This function should only be called once the
    //! RenderManager has finished to render every object inside.
    void processDeletionQueues();

    void fillBuildableTilesAndPriceForPlayerInArea(int x1, int y1, int x2, int y2,
        Player* player, RoomType type, std::vector<Tile*>& tiles, int& goldRequired);

    void updateVisibleEntities();

    inline const std::vector<RenderedMovableEntity*>& getRenderedMovableEntities() const
    { return mRenderedMovableEntities; }

    inline void setTileSetName(const std::string& tileSetName)
    { mTileSetName = tileSetName; }

    inline const std::string& getTileSetName() const
    { return mTileSetName; }

    const TileSetValue& getMeshForTile(const Tile* tile) const;
    inline const Ogre::Vector3& getTileSetScale() const
    { return mTileSet->getScale(); }

private:
    void replaceFloodFill(FloodFillType floodFillType, int colorOld, int colorNew);

    //! \brief Tells whether this game map instance is used as a reference by the server-side,
    //! or as a standard client game map.
    bool mIsServerGameMap;

    //! \brief the Local player reference. The local player will also be in the player list so this pointer
    //! should not be deleted as it will be handled like every other in the list.
    Player* mLocalPlayer;

    std::string mLocalPlayerNick;

    //! \brief The current server turn number.
    int64_t mTurnNumber;

    //! \brief Unique numbers to ensure names are unique
    int mUniqueNumberCreature;
    int mUniqueNumberMissileObj;
    int mUniqueNumberRoom;
    int mUniqueNumberRenderedMovableEntity;
    int mUniqueNumberTrap;
    int mUniqueNumberMapLight;

    //! \brief When paused, the GameMap is not updated.
    bool mIsPaused;

    Ogre::Real mTimePayDay;

    //! \brief Level related filenames.
    std::string mLevelFileName;

    //! \brief Map info
    std::string mMapInfoName;
    std::string mMapInfoDescription;
    std::string mMapInfoMusicFile;
    std::string mMapInfoFightMusicFile;

    std::vector<Creature*> mCreatures;

    //! \brief The creature definition data. We use a pair to be able to make the difference between the original
    //! data from the global creature definition file and the specific data from the level file. With this trick,
    //! we will be able to compare and write the differences in the level file.
    //! It is the same for weapons.
    std::vector<std::pair<const CreatureDefinition*,CreatureDefinition*> > mClassDescriptions;
    std::vector<std::pair<const Weapon*,Weapon*> > mWeapons;

    //Mutable to allow locking in const functions.
    std::vector<MovableGameEntity*> mAnimatedObjects;

    //! \brief Map Entities
    std::vector<Room*> mRooms;
    std::vector<Trap*> mTraps;
    std::vector<MapLight*> mMapLights;

    //! \brief Players and available game player slots (Seats)
    std::vector<Player*> mPlayers;
    std::vector<Seat*> mSeats;
    std::vector<Seat*> mWinningSeats;

    //! \brief Common player goals
    std::vector<Goal*> mGoalsForAllSeats;

    //! \brief Tells whether the map color flood filling is enabled.
    bool mFloodFillEnabled;

    //! When true, fog of war will work normally. When false, every connected client will see the whole map
    bool mIsFOWActivated;

    std::vector<GameEntity*> mActiveObjects;

    //! \brief  active objects that are created are stored here. They will be added after the miscupkeep to avoid changing the list while we use it
    std::deque<GameEntity*> mActiveObjectsToAdd;

    //! \brief  active objects that are removed are stored here. They will be removed after the miscupkeep to avoid changing the list while we use it
    std::deque<GameEntity*> mActiveObjectsToRemove;

    //! \brief Useless entities that need to be deleted. They will be deleted when processDeletionQueues is called
    std::vector<GameEntity*> mEntitiesToDelete;

    //! \brief Debug member used to know how many call to pathfinding has been made within the same turn.
    unsigned int mNumCallsTo_path;

    std::vector<RenderedMovableEntity*> mRenderedMovableEntities;

    std::vector<Spell*> mSpells;

    //! AI Handling manager
    AIManager mAiManager;

    //! Map tileset
    const TileSet* mTileSet;
    std::string mTileSetName;

    //! Creature mood modifiers. Used to compute mood. The name of the mood modifier is associated with
    //! the list of mood modifier
    std::map<const std::string, std::vector<CreatureMood*>> mCreatureMoodModifiers;

    //! \brief Updates different entities states.
    //! Updates active objects (creatures, rooms, ...), goals, count each team Workers, gold, mana and claimed tiles.
    unsigned long int doMiscUpkeep();

    //! \brief Resets the unique numbers
    void resetUniqueNumbers();

    //! \brief Updates every player's time value so they can handle timed events like fighting music
    //! Used on the server game map only.
    void updatePlayerTime(Ogre::Real timeSinceLastFrame);
};

#endif // _GAMEMAP_H_
