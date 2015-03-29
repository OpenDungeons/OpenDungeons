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

class RoomPortalWave: public Room
{
public:
    RoomPortalWave(GameMap* gameMap);
    virtual ~RoomPortalWave();

    virtual RoomType getType() const override
    { return RoomType::portalWave; }

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

    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

    virtual void restoreInitialEntityState() override;

    void addRoomPortalWaveData(RoomPortalWaveData* roomPortalWaveData);

protected:
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
    std::vector<Tile*> mWayToEnemy;

    //! \brief Updates the portal mesh position.
    void updatePortalPosition();

    //! Spawns a wave
    void spawnWave(RoomPortalWaveData* roomPortalWaveData, uint32_t maxCreaturesToSpawn);

    //! Marks needed tiles to try to get to some player's dungeon. Returns true if an enemy dungeon
    //! is reachable by digging and marks corresponding tiles.
    bool handleDigging();

    //! Searches for the best foe we can fight. Returns true if an enemy dungeon is reachable
    //! without digging and false otherwise
    bool handleSearchFoe();
};

#endif // ROOMPORTALWAVE_H
