/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef TILE_H
#define TILE_H

#include "entities/GameEntity.h"

#include <OgreVector3.h>

#include <string>
#include <vector>
#include <iosfwd>
#include <cstdint>

class Building;
class Creature;
class GameEntity;
class Player;
class Room;
class MapLight;
class GameMap;
class CreatureDefinition;
class Trap;
class TreasuryObject;
class ChickenEntity;
class CraftedTrap;
class BuildingObject;
class PersistentObject;
class ODPacket;

enum class RoomType;
enum class SelectionEntityWanted;
enum class TrapType;

//! Tile types a tile can be
enum class TileType
{
    nullTileType = 0,
    dirt = 1,
    gold = 2,
    rock = 3,
    water = 4,
    lava = 5,
    gem = 6,
    countTileType
};

enum class TileSound
{
    ClaimGround,
    ClaimWall,
    Digged,
    BuildRoom,
    BuildTrap
};

ODPacket& operator<<(ODPacket& os, const TileType& type);
ODPacket& operator>>(ODPacket& is, TileType& type);
std::ostream& operator<<(std::ostream& os, const TileType& type);
std::istream& operator>>(std::istream& is, TileType& type);


//! Different representations a tile can have (ground or full)
enum class TileVisual
{
    nullTileVisual = 0,
    dirtGround,
    dirtFull,
    goldGround,
    goldFull,
    rockGround,
    rockFull,
    waterGround,
    lavaGround,
    claimedGround,
    claimedFull,
    gemGround,
    gemFull,
    countTileVisual
};

ODPacket& operator<<(ODPacket& os, const TileVisual& type);
ODPacket& operator>>(ODPacket& is, TileVisual& type);
std::ostream& operator<<(std::ostream& os, const TileVisual& type);
std::istream& operator>>(std::istream& is, TileVisual& type);

enum class FloodFillType
{
    ground = 0,
    groundWater,
    groundLava,
    groundWaterLava,
    nbValues
};

/*! \brief The tile class contains information about tile type and contents and is the basic level bulding block.
 *
 * A Tile is the basic building block for the GameMap.  It consists of a tile
 * type (rock, dirt, gold, etc.) as well as a fullness which indicates how much
 * the tile has been dug out.  Additionally the tile contains lists of the
 * entities located within it to aid in AI calculations.
 */
class Tile : public GameEntity
{
public:
    Tile(GameMap* gameMap, int x = 0, int y = 0, TileType type = TileType::dirt, double fullness = 100.0);

    virtual ~Tile();

    static const uint32_t NO_FLOODFILL;
    static const std::string TILE_PREFIX;
    static const std::string TILE_SCANF;

    virtual GameEntityType getObjectType() const override;

    void doUpkeep() override
    {}

    std::vector<Tile*> getCoveredTiles() override
    { return std::vector<Tile*>(); }

    Tile* getCoveredTile(int index) override
    { return nullptr; }

    uint32_t numCoveredTiles() const override
    { return 0; }

    double getHP(Tile* tile) const override
    { return 0.0; }

    double takeDamage(GameEntity* attacker, double absoluteDamage, double physicalDamage, double magicalDamage, double elementDamage,
        Tile *tileTakingDamage, bool ko) override
    { return 0.0; }

    void addToGameMap() override
    {}
    void removeFromGameMap() override
    {}

    void fireAddEntity(Seat* seat, bool async) override
    {}

    void fireRemoveEntity(Seat* seat) override
    {}

    /*! \brief Set the type (rock, claimed, etc.) of the tile.
     *
     * In addition to setting the tile type this function also reloads the new mesh
     * for the tile.
     */
    inline void setType(TileType t)
    { mType = t; }

    //! \brief Returns the tile type (rock, claimed, etc.).
    inline TileType getType() const
    { return mType; }

    //! \brief Returns the tile type (rock, claimed, etc.).
    inline TileVisual getTileVisual() const
    { return mTileVisual; }

    //! \brief Sets the tile type (rock, claimed, etc.).
    inline void setTileVisual(TileVisual tileVisual)
    { mTileVisual = tileVisual; }

    //! \brief A mutator to change how "filled in" the tile is.
    //! Additionally this function refreshes floodfill if needed (if a tile becomes walkable)
    void setFullness(double f);

    //! \brief An accessor which returns the tile's fullness which should range from 0 to 100.
    inline double getFullness() const
    { return mFullness; }

    //! \brief Tells whether a creature can see through a tile
    bool permitsVision();

    inline uint32_t getRefundPriceRoom() const
    { return mRefundPriceRoom; }

    inline uint32_t getRefundPriceTrap() const
    { return mRefundPriceTrap; }


    /*! \brief This is a helper function to scroll through the list of available fullness levels.
     *
     * This function is used in the map editor when the user presses the button to
     * select the next tile fullness level to be active in the user interface.  The
     * active fullness level is the one which is placed when the user clicks the
     * mouse button.
     */
    static int nextTileFullness(int f);

    //! \brief This function puts a message in the renderQueue to change the mesh for this tile.
    void refreshMesh();

    //! \brief Marks the tile as being selected through a mouse click or drag.
    void setSelected(bool ss, const Player* pp);

    //! \brief Returns whether or not the tile has been selected.
    bool getSelected() const
    { return mSelected; }

    inline void setLocalPlayerHasVision(bool localPlayerHasVision)
    { mLocalPlayerHasVision = localPlayerHasVision; }

    inline bool getLocalPlayerHasVision() const
    { return mLocalPlayerHasVision; }

    //! \brief Set/unset the value of the mask depending on boolean value
    void setTileCullingFlags(uint32_t mask, bool value);

    //! \brief Set the tile digging mark for the given player.
    void setMarkedForDigging(bool s, const Player* p);

    //! \brief This accessor function returns whether or not the tile has been marked to be dug out by a given Player p.
    bool getMarkedForDigging(const Player* p) const;

    //! \brief This is a simple helper function which just calls setMarkedForDigging() for everyone in the game except
    //! allied to exceptSeat. If exceptSeat is nullptr, it is called for every player
    void setMarkedForDiggingForAllPlayersExcept(bool s, Seat* exceptSeat);

    //! \brief Tells whether the tile is selected for digging by any player/AI.
    bool isMarkedForDiggingByAnySeat();

    //! \brief Add/Remove a player to the vector of players who have marked this tile for digging.
    void addPlayerMarkingTile(const Player *p);
    void removePlayerMarkingTile(const Player *p);

    //! \brief This function adds an entity to the list of entities in this tile.
    bool addEntity(GameEntity *entity);

    //! \brief This function removes an entity to the list of entities in this tile.
    void removeEntity(GameEntity *entity);

    //! \brief This function returns the count of the number of creatures in the tile.
    unsigned int numEntitiesInTile() const
    { return mEntitiesInTile.size(); }

    //! \brief This function returns the count of the number of creatures in the tile.
    const std::vector<GameEntity*>& getEntitiesInTile() const
    { return mEntitiesInTile; }

    void addNeighbor(Tile *n);
    Tile* getNeighbor(unsigned index);
    const std::vector<Tile*>& getAllNeighbors() const
    { return mNeighbors; }

    void claimForSeat(Seat* seat, double nDanceRate);
    void claimTile(Seat* seat);
    void unclaimTile();
    double digOut(double digRate);

    inline Building* getCoveringBuilding() const
    { return mCoveringBuilding; }

    //! \brief Proxy that checks if there is a covering building and if it is a room. If yes, returns
    //! a pointer to the covering room
    Room* getCoveringRoom() const;

    //! \brief Proxy that checks if there is a covering room and, if yes, that its
    //! type is the expected one. This is for convenience because it is a pretty
    //! common check
    bool checkCoveringRoomType(RoomType type) const;

    //! \brief Proxy that checks if there is a covering building and if it is a trap. If yes, returns
    //! a pointer to the covering trap
    Trap* getCoveringTrap() const;

    //! \brief Proxy that checks if there is a covering trap and, if yes, that its
    //! type is the expected one. This is for convenience because it is a pretty
    //! common check
    bool checkCoveringTrapType(TrapType type) const;

    void setCoveringBuilding(Building* building);

    //! \brief Add a tresaury object in this tile. There can be only one per tile so if there is already one, they are merged
    bool addTreasuryObject(TreasuryObject* object);

    //! \brief Tells whether the tile is diggable by dig-capable creatures.
    //! \brief The player seat.
    //! The function will check whether a tile is not already a reinforced wall owned by another team.
    bool isDiggable(const Seat* seat) const;

    //! \brief Tells whether the tile fullness is empty (ground tile) and can be claimed by the given seat.
    bool isGroundClaimable(Seat* seat) const;

    //! \brief Tells whether the tile is a wall (fullness > 1) and can be claimed for the given seat.
    //! Reinforced walls by another team and hard rocks can't be claimed.
    bool isWallClaimable(Seat* seat);

    //! \brief Tells whether the tile is claimed for the given seat.
    bool isClaimedForSeat(const Seat* seat) const;

    //! \brief Tells whether the tile is claimed for the given seat.
    bool isClaimed() const;

    //! \brief Tells whether the given tile is a claimed wall for the given seat team.
    //! Used to discover active spots for rooms.
    bool isWallClaimedForSeat(Seat* seat);

    //! \brief Tells whether a room can be built upon this tile.
    bool isBuildableUpon(Seat* seat) const;

    static std::string getFormat();

    //! \brief Loads the tile data from a level line.
    static void loadFromLine(const std::string& line, Tile *t);

    /*! \brief This is a helper function which just converts the tile type enum into a string.
     *
     * This function is used primarily in forming the mesh names to load from disk
     * for the various tile types.  The name returned by this function is
     * concatenated with a fullnessMeshNumber to form the filename, e.g.
     * Dirt104.mesh is a 4 sided dirt mesh with 100% fullness.
     */
    static std::string tileTypeToString(TileType t);

    static std::string tileVisualToString(TileVisual tileVisual);
    static TileVisual tileVisualFromString(const std::string& strTileVisual);

    inline int getX() const
    { return mX; }

    inline int getY() const
    { return mY; }

    inline double getClaimedPercentage() const
    { return mClaimedPercentage; }

    static std::string buildName(int x, int y);
    static bool checkTileName(const std::string& tileName, int& x, int& y);

    static std::string displayAsString(const Tile* tile);

    //! \brief fills the given vector with the carryable entities on this tile
    void fillWithCarryableEntities(Creature* carrier, std::vector<GameEntity*>& entities);
    uint32_t countEntitiesOnTile(GameEntityType entityType) const;

    //! \brief Returns true if the given entity is on the tile and false otherwise
    bool isEntityOnTile(GameEntity* entity) const;

    //! Fills the given vector with corresponding entities on this tile.
    void fillWithEntities(std::vector<GameEntity*>& entities, SelectionEntityWanted entityWanted, Player* player);

    //! \brief Computes the visible tiles and tags them to know which are visible
    void computeVisibleTiles();
    void clearVision();
    void notifyVision(Seat* seat);

    void setSeats(const std::vector<Seat*>& seats);
    bool hasChangedForSeat(Seat* seat) const;
    void changeNotifiedForSeat(Seat* seat);

    void notifyEntitiesSeatsWithVision();

    const std::vector<Seat*>& getSeatsWithVision()
    { return mSeatsWithVision; }

    void resetFloodFill();

    static std::string toString(FloodFillType type);

    bool isSameFloodFill(Seat* seat, FloodFillType type, Tile* tile) const;

    //! Updates the floodfill from the given tile if floodfill is not already set.
    //! returns true if the floodfill has been updated and false otherwise
    bool updateFloodFillFromTile(Seat* seat, FloodFillType type, Tile* tile);

    //! Sets the floodfill value corresponding at type to newValue
    void replaceFloodFill(Seat* seat, FloodFillType type, uint32_t newValue);

    void copyFloodFillToOtherSeats(Seat* seatToCopy);

    uint32_t getFloodFillValue(Seat* seat, FloodFillType type) const;

    void logFloodFill() const;

    bool isFloodFillFilled(Seat* seat) const;

    //! \brief Returns true if the given type can be set for the current tile
    //! depending on its type/fullness
    bool isFloodFillPossible(Seat* seat, FloodFillType type) const;

    //! Refresh the tile visual according to the tile parameters (type, claimed, ...).
    //! Used only on server side
    void computeTileVisual();

    //! Function that allows to know if the tile is full or not. Works for both
    //! server and client
    bool isFullTile() const;

    //! Sets the number of teams in this gamemap (after seat configuration). This number includes the rogue team.
    void setTeamsNumber(uint32_t nbTeams);

    //! \brief returns true if the mesh from the tileset should be displayed and false otherwise
    inline bool shouldDisplayTileMesh() const
    { return mDisplayTileMesh; }

    //! \brief returns true if the mesh from the tileset should be colored and false otherwise
    bool shouldColorTileMesh() const;

    //! \brief returns true if the custom mesh (from a building, for example) should be colored and false otherwise
    inline bool shouldColorCustomMesh() const
    { return mColorCustomMesh; }

    inline bool getHasBridge() const
    { return mHasBridge; }

    //! \brief returns true if there is a building on this tile and false otherwise.
    //! client side function
    inline bool getIsBuilding() const
    { return mIsRoom || mIsTrap; }
    inline bool getIsRoom() const
    { return mIsRoom; }
    inline bool getIsTrap() const
    { return mIsTrap; }

    void fireTileSound(TileSound sound);

    double getCreatureSpeedDefault(const Creature* creature) const;

    //! \brief Allows to lock the tile for the workers claiming it. Returns true if the worker could be added
    //! and false otherwise
    //! Note that by nature, a tile cannot be claimed for wall and ground. Because of that, we can use the same
    //! functions for both
    bool canWorkerClaim(const Creature& worker);
    bool addWorkerClaiming(const Creature& worker);
    bool removeWorkerClaiming(const Creature& worker);
    //! \brief Allows to lock the tile for the workers claiming it. Returns true if the worker could be added
    //! and false otherwise
    bool canWorkerDig(const Creature& worker);
    bool addWorkerDigging(const Creature& worker);
    bool removeWorkerDigging(const Creature& worker);

    static void exportToStream(Tile* tile, std::ostream& os);

    virtual void exportToPacketForUpdate(ODPacket& os, const Seat* seat) const override;
    virtual void updateFromPacket(ODPacket& is) override;

protected:
    virtual void exportHeadersToStream(std::ostream& os) const override
    {}
    virtual void exportHeadersToPacket(ODPacket& os) const override
    {}
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override
    { return true; }
    virtual void exportToPacket(ODPacket& os, const Seat* seat) const override
    {}
    virtual void importFromPacket(ODPacket& is) override
    {}

    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
private:
    //! \brief The tile position
    int mX, mY;

    //! \brief The tile type: Dirt, Gold, ...
    TileType mType;

    //! \brief The tile visual: Claimed, Dirt, Gold, ...
    //! On client side, we should rely on mTileVisual to know the tile type as claimed percentage
    //! could not be up to date
    TileVisual mTileVisual;

    //! \brief Whether the tile is selected.
    bool mSelected;

    //! \brief The tile fullness (0.0 - 100.0).
    //! At 0.0, it is a ground tile. Over it is a wall.
    //! Used on server side only
    double mFullness;

    //! Used on client side to know how much gold can be retrieved if the room/trap
    //! is sold. Note that it is needed because client are not aware of rooms/traps
    uint32_t mRefundPriceRoom;
    uint32_t mRefundPriceTrap;

    std::vector<Tile*> mNeighbors;
    std::vector<const Player*> mPlayersMarkingTile;
    std::vector<std::pair<Seat*, bool>> mTileChangedForSeats;
    std::vector<Seat*> mSeatsWithVision;

    //! \brief List of the entities actually on this tile. Most of the creatures actions will rely on this list
    std::vector<GameEntity*> mEntitiesInTile;

    Building* mCoveringBuilding;
    //! Floodfill values per seat and per floodfill type
    std::vector<std::vector<uint32_t>> mFloodFillColor;

    //! \brief The tile claiming. Used on server side only
    double mClaimedPercentage;

    //! \brief True if a building is on this tile. False otherwise. It is used on client side because the clients do not know about
    //! buildings. However, it needs to know the tiles where a building is to display the room/trap costs.
    bool mIsRoom;
    bool mIsTrap;

    bool mDisplayTileMesh;

    bool mColorCustomMesh;

    bool mHasBridge;

    //! \brief Used on client side. true if the local player has vision, false otherwise.
    bool mLocalPlayerHasVision;

    uint32_t mTileCulling;

    /*! \brief Set the fullness value for the tile.
     *  This only sets the fullness variable. This function is here to change the value
     *  before a map object has been set. setFullness is called once a map is assigned.
     */
    inline void setFullnessValue(double f)
    { mFullness = f; }

    void setDirtyForAllSeats();

    uint32_t mNbWorkersDigging;
    uint32_t mNbWorkersClaiming;
};

#endif // TILE_H
