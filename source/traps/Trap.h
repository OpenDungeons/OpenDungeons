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

#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <map>

class CraftedTrap;
class Creature;
class GameMap;
class Player;
class Seat;
class Tile;
class TrapEntity;
class RenderedMovableEntity;
class ODPacket;

#include "entities/Building.h"

enum class TrapType
{
    nullTrapType = 0,
    cannon,
    spike,
    boulder
};

std::istream& operator>>(std::istream& is, TrapType& tt);
std::ostream& operator<<(std::ostream& os, const TrapType& tt);
ODPacket& operator>>(ODPacket& is, TrapType& tt);
ODPacket& operator<<(ODPacket& os, const TrapType& tt);

//! \brief A small class telling whether a trap tile is activated.
class TrapTileInfo
{
public:
    TrapTileInfo():
        mIsActivated(false),
        mReloadTime(0),
        mCraftedTrap(nullptr),
        mNbShootsBeforeDeactivation(0),
        mTrapEntity(nullptr)
    {}

    TrapTileInfo(uint32_t reloadTime, bool activated):
        mIsActivated(activated),
        mReloadTime(reloadTime),
        mCraftedTrap(nullptr),
        mNbShootsBeforeDeactivation(0),
        mTrapEntity(nullptr)
    {}

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

    void setActivated(bool activated)
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

    bool isActivated() const
    { return mIsActivated; }

    void setReloadTime(uint32_t reloadTime)
    { mReloadTime = reloadTime; }

    void setNbShootsBeforeDeactivation(int32_t nbShoot)
    { mNbShootsBeforeDeactivation = nbShoot; }

    void setCarriedCraftedTrap(CraftedTrap* craftedTrap)
    { mCraftedTrap = craftedTrap; }

    CraftedTrap* getCarriedCraftedTrap() const
    { return mCraftedTrap; }

private:
    bool mIsActivated;
    uint32_t mReloadTime;
    CraftedTrap* mCraftedTrap;
    int32_t mNbShootsBeforeDeactivation;
    TrapEntity* mTrapEntity;
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

    static Trap* getTrapFromStream(GameMap* gameMap, std::istream &is);
    static Trap* getTrapFromPacket(GameMap* gameMap, ODPacket &is);

    virtual const TrapType getType() const = 0;

    static std::string getTrapNameFromTrapType(TrapType t);

    static int costPerTile(TrapType t);

    // Functions which can be overridden by child classes.
    virtual void doUpkeep();

    virtual bool shoot(Tile* tile)
    { return true; }

    //! \brief Tells whether the trap is activated.
    bool isActivated(Tile* tile) const;

    //! \brief Sets the name, seat and associates the given tiles with the trap
    void setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles);

    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);
    virtual void updateActiveSpots();

    static int32_t getNeededWorkshopPointsPerTrap(TrapType trapType);
    virtual int32_t getNbNeededCraftedTrap() const;

    bool hasCarryEntitySpot(GameEntity* carriedEntity);
    Tile* askSpotForCarriedEntity(GameEntity* carriedEntity);
    void notifyCarryingStateChanged(Creature* carrier, GameEntity* carriedEntity);

    virtual bool isAttackable(Tile* tile, Seat* seat) const;

    virtual bool shouldSetCoveringTileDirty(Seat* seat, Tile* tile)
    { return false; }

    /*! \brief Exports the headers needed to recreate the Trap. It allows to extend Traps as much as wanted.
     * The content of the Trap will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os) const override;
    virtual void exportHeadersToPacket(ODPacket& os) const override;
    //! \brief Exports the data of the Trap
    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;
    virtual void exportToPacket(ODPacket& os) const override;
    virtual void importFromPacket(ODPacket& is) override;

    static std::string getTrapStreamFormat();

protected:
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

    //! \brief Tells the current reloading time left for each tiles and whether it is activated.
    std::map<Tile*, TrapTileInfo> mTrapTiles;
};

#endif // TRAP_H
