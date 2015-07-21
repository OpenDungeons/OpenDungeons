/*
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

#ifndef ROOMPORTALWAVE_H
#define ROOMPORTALWAVE_H

#include "rooms/Room.h"
#include "rooms/RoomType.h"

#include <vector>

class CreatureDefinition;

enum class RoomPortalWaveStrategy
{
    // Checks for the closest dungeon and attack it until it is destroyed. Then, go to the next closest dungeon
    closestDungeon,
    // At each wave, the portal will choose randomly an accessible player and spend a go to war spell if the way is clear or
    // try to dig otherwise.
    randomPlayer,
    // Picks randomly a player of one of the chosen team
    fixedTeamIds
};

std::ostream& operator<<(std::ostream& os, const RoomPortalWaveStrategy& type);
std::istream& operator>>(std::istream& is, RoomPortalWaveStrategy& type);

class RoomPortalWaveData
{
public:
    RoomPortalWaveData() :
        mSpawnTurnMin(0),
        mSpawnTurnMax(-1)
    {}

    int64_t mSpawnTurnMin;
    int64_t mSpawnTurnMax;
    //! Pair with creature class name and level
    std::vector<std::pair<std::string, uint32_t>> mSpawnCreatureClassName;
};

class RoomPortalWave: public Room, public GameEntityListener
{
public:
    RoomPortalWave(GameMap* gameMap);
    virtual ~RoomPortalWave();

    virtual RoomType getType() const override
    { return RoomType::portalWave; }

    void absorbRoom(Room *r) override;
    bool removeCoveredTile(Tile* t) override;

    //! Room portal is claimable by enemy seats
    virtual bool isClaimable(Seat* seat) const override;
    virtual void claimForSeat(Seat* seat, Tile* tile, double danceRate) override;

    //! Room portal cannot be destroyed
    virtual bool isAttackable(Tile* tile, Seat* seat) const override
    { return false; }

    //! No seat can sell Room portals
    virtual bool canSeatSellBuilding(Seat* seat) const override
    { return false; }

    //! \brief In addition to the standard upkeep, check to see if a new creature should be spawned.
    void doUpkeep() override;

    //! \brief Spawns one of the available waves
    void spawnWave();

    //! \brief Portals only display claimed tiles on their ground.
    virtual bool shouldDisplayBuildingTile() const override
    { return false; }

    //! \brief Updates the portal position when in editor mode.
    void updateActiveSpots() override;

    virtual void setupRoom(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles) override;

    virtual void restoreInitialEntityState() override;

    void addRoomPortalWaveData(RoomPortalWaveData* roomPortalWaveData);

    //! \brief implementation of GameEntityListener
    std::string getListenerName() const override
    { return getName(); }
    bool notifyDead(GameEntity* entity) override
    { return true; }
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override
    { return true; }
    bool notifyDropped(GameEntity* entity) override
    { return true; }

    static void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet);
    static void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand);
    static bool buildRoomEditor(GameMap* gameMap, ODPacket& packet);
    static Room* getRoomFromStream(GameMap* gameMap, std::istream& is);

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

    void destroyMeshLocal() override;

    void notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile) override
    {
        // This Room keeps its building object until it is destroyed (it will be released when
        // the room is destroyed)
    }

private:
    //! \brief Stores the number of turns before spawning the next creature.
    uint32_t mSpawnCountdown;
    uint32_t mSearchFoeCountdown;
    uint32_t mTurnsBetween2Waves;
    RenderedMovableEntity* mPortalObject;

    double mClaimedValue;

    //! Stores the spawnable waves
    std::vector<RoomPortalWaveData*> mRoomPortalWaveDataSpawnable;

    //! Stores non spawnable waves
    std::vector<RoomPortalWaveData*> mRoomPortalWaveDataNotSpawnable;

    //! Stores the tiles to dig to go to the enemy dungeon temple. That allows
    //! to change at runtime the way if a tile is claimed while going there
    std::vector<Tile*> mMarkedTilesToEnemy;
    //! Stores seats that we currently want to attack depending on the strategy
    std::vector<Seat*> mTargetSeats;
    Room* mTargetDungeon;

    bool mIsFirstUpkeep;
    RoomPortalWaveStrategy mStrategy;
    //! \brief Range to attack. If a player starts claiming tiles within range, the portal will try to dig to the corresponding
    //! dungeon temple. If -1, there is no limit
    int32_t mRangeTilesAttack;
    std::vector<int> mTargetTeams;

    //! \brief Tiles that will be checked if claimed by an enemy. If yes, the corresponding player will get attacked
    std::vector<Tile*> mTilesBorder;
    //! \brief List of the seats that can be attacked by the portal
    std::vector<Seat*> mAttackableSeats;

    //! \brief Updates the portal mesh position.
    void updatePortalPosition();

    //! \brief Finds the best diggable path between tileStart and tileDest.
    //! Note that a path is returned even if tileDest is not reachable. It should go around
    //! the undiggable tiles
    //! Returns true if a path was found to the dungeon and false otherwise
    bool findBestDiggablePath(Tile* tileStart, Tile* tileDest, Creature* creature, std::vector<Tile*>& tiles);

    //! \brief Spawns a wave
    void spawnWave(RoomPortalWaveData* roomPortalWaveData, uint32_t maxCreaturesToSpawn);

    //! \brief Marks needed tiles to try to get to some player's dungeon. Returns true if an enemy dungeon
    //! is reachable by digging and marks corresponding tiles.
    bool handleDigging();

    //! \brief Searches for the best foe we can fight. Returns true if an enemy dungeon is reachable
    //! without digging and false otherwise
    bool handleSearchFoe();

    //! \brief Handles the first upkeep by filling the tiles in range, checking if there is a dungeon in range, ...
    void handleFirstUpkeep();

    //! \brief Handles the attack, check if there is already a target and tries to reach it if so. If not,
    //! tries to find a suitable target according to target seats
    void handleAttack();

    //! \brief Handles spawning a new wave
    void handleSpawnWave();

    //! \brief Handles choosing a target according to the portal strategy
    void handleChooseTarget();
};

#endif // ROOMPORTALWAVE_H
