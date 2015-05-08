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

#ifndef TRAP_H
#define TRAP_H

#include "entities/Building.h"

#include <string>
#include <vector>
#include <iosfwd>

class CraftedTrap;
class Creature;
class GameMap;
class Player;
class Seat;
class Tile;
class TrapEntity;
class RenderedMovableEntity;
class ODPacket;

enum class TrapType;


//! \brief A small class telling whether a trap tile is activated.
class TrapTileData : public TileData
{
public:
    TrapTileData() :
        TileData(),
        mClaimedValue(1.0),
        mIsActivated(false),
        mReloadTime(0),
        mCraftedTrap(nullptr),
        mNbShootsBeforeDeactivation(0),
        mTrapEntity(nullptr),
        mIsWorking(false),
        mRemoveTrap(false)
    {}

    TrapTileData(const TrapTileData* trapTileData) :
        TileData(trapTileData),
        mIsActivated(trapTileData->mIsActivated),
        mReloadTime(trapTileData->mReloadTime),
        mCraftedTrap(trapTileData->mCraftedTrap),
        mNbShootsBeforeDeactivation(trapTileData->mNbShootsBeforeDeactivation),
        mTrapEntity(trapTileData->mTrapEntity),
        mIsWorking(trapTileData->mIsWorking),
        mRemoveTrap(trapTileData->mRemoveTrap)
    {}

    virtual ~TrapTileData()
    {}

    virtual TrapTileData* cloneTileData() const override
    { return new TrapTileData(this); }

    inline void setTrapEntity(TrapEntity* trapEntity)
    { mTrapEntity = trapEntity; }

    inline TrapEntity* getTrapEntity() const
    { return mTrapEntity; }

    bool decreaseReloadTime()
    {
        if (mReloadTime > 1)
        {
            --mReloadTime;
            return true;
        }

        mReloadTime = 0;
        return false;
    }

    inline void setActivated(bool activated)
    { mIsActivated = activated; }

    bool decreaseShoot()
    {
        if (mNbShootsBeforeDeactivation > 1)
        {
            --mNbShootsBeforeDeactivation;
            return true;
        }

        mNbShootsBeforeDeactivation = 0;
        return false;
    }

    inline bool isActivated() const
    { return mIsActivated; }

    inline uint32_t getReloadTime() const
    { return mReloadTime; }

    inline void setReloadTime(uint32_t reloadTime)
    { mReloadTime = reloadTime; }

    inline void setNbShootsBeforeDeactivation(int32_t nbShoot)
    { mNbShootsBeforeDeactivation = nbShoot; }

    inline int32_t getNbShootsBeforeDeactivation() const
    { return mNbShootsBeforeDeactivation; }

    inline void setCarriedCraftedTrap(CraftedTrap* craftedTrap)
    { mCraftedTrap = craftedTrap; }

    inline CraftedTrap* getCarriedCraftedTrap() const
    { return mCraftedTrap; }

    inline bool getIsWorking() const
    { return mIsWorking; }

    inline void setIsWorking(bool isWorking)
    { mIsWorking = isWorking; }

    inline bool getRemoveTrap() const
    { return mRemoveTrap; }

    inline void setRemoveTrap(bool removeTrap)
    { mRemoveTrap = removeTrap; }

    double mClaimedValue;

private:
    bool mIsActivated;
    uint32_t mReloadTime;
    CraftedTrap* mCraftedTrap;
    int32_t mNbShootsBeforeDeactivation;
    TrapEntity* mTrapEntity;
    bool mIsWorking;
    bool mRemoveTrap;
};

/*! \class Trap Trap.h
 *  \brief Defines a trap
 */
class Trap : public Building
{
public:
    Trap(GameMap* gameMap);
    virtual ~Trap()
    {}

    virtual GameEntityType getObjectType() const
    { return GameEntityType::trap; }

    virtual void addToGameMap() override;
    virtual void removeFromGameMap() override;

    virtual const TrapType getType() const = 0;

    //! Traps can be claimed by enemy seats
    virtual bool isClaimable(Seat* seat) const override;
    virtual void claimForSeat(Seat* seat, Tile* tile, double danceRate) override;

    virtual void doUpkeep() override;

    virtual bool shoot(Tile* tile)
    { return true; }

    //! \brief Tells whether the trap is activated.
    bool isActivated(Tile* tile) const;

    //! \brief Sets the name, seat and associates the given tiles with the trap
    virtual void setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles);

    virtual bool removeCoveredTile(Tile* t);
    virtual void updateActiveSpots();

    virtual int32_t getNbNeededCraftedTrap() const;

    bool hasCarryEntitySpot(GameEntity* carriedEntity);
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity);
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity);

    virtual bool isAttackable(Tile* tile, Seat* seat) const;

    virtual bool shouldSetCoveringTileDirty(Seat* seat, Tile* tile)
    { return false; }

    virtual void restoreInitialEntityState() override;

    virtual bool isTileVisibleForSeat(Tile* tile, Seat* seat) const override;

    virtual void exportHeadersToStream(std::ostream& os) const override;
    virtual void exportTileDataToStream(std::ostream& os, Tile* tile, TileData* tileData) const override;
    virtual void importTileDataFromStream(std::istream& is, Tile* tile, TileData* tileData) override;

    static std::string getTrapStreamFormat();

    static void buildTrapDefault(GameMap* gameMap, Trap* trap, const std::vector<Tile*>& tiles, Seat* seat);
    static int getTrapCostDefault(std::vector<Tile*>& tiles, GameMap* gameMap, TrapType type,
        int tileX1, int tileY1, int tileX2, int tileY2, Player* player);

protected:
    virtual TrapTileData* createTileData(Tile* tile) override;

    virtual RenderedMovableEntity* notifyActiveSpotCreated(Tile* tile);
    virtual TrapEntity* getTrapEntity(Tile* tile) = 0;
    virtual void notifyActiveSpotRemoved(Tile* tile);

    //! \brief Triggered when the trap is activated
    virtual void activate(Tile* tile);

    //! \brief Triggered when deactivated.
    virtual void deactivate(Tile* tile);

    uint32_t mNbShootsBeforeDeactivation;
    uint32_t mReloadTime;
    double mMinDamage;
    double mMaxDamage;

    //! List of traps destroyed but with at least 1 player having vision. They will
    //! get removed when vision is gained by every player having seen it before destruction
    std::vector<RenderedMovableEntity*> mTrapEntitiesWaitingRemove;
};

#endif // TRAP_H
