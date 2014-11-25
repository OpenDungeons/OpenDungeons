/*
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

#ifndef TRAP_H
#define TRAP_H

#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <map>

class GameMap;
class Player;
class Seat;
class Tile;
class RenderedMovableEntity;
class ODPacket;

#include "entities/Building.h"

/*! \class Trap Trap.h
 *  \brief Defines a trap
 */
class Trap : public Building
{
public:
    enum TrapType
    {
        nullTrapType = 0,
        cannon,
        spike,
        boulder
    };

    Trap(GameMap* gameMap);
    virtual ~Trap()
    {}

    virtual std::string getOgreNamePrefix() const { return "Trap_"; }

    static Trap* getTrapFromStream(GameMap* gameMap, std::istream &is);
    static Trap* getTrapFromPacket(GameMap* gameMap, ODPacket &is);

    virtual const TrapType getType() const = 0;

    static const char* getTrapNameFromTrapType(TrapType t);

    static int costPerTile(TrapType t);

    // Functions which can be overridden by child classes.
    virtual void doUpkeep();

    virtual bool shoot(Tile* tile)
    {
        return true;
    }

    //! \brief Tells whether the trap is activated.
    bool isActivated() const
    { return mIsActivated; }

    //! \brief Sets the name, seat and associates the given tiles with the trap
    void setupTrap(const std::string& name, Seat* seat, const std::vector<Tile*>& tiles);

    virtual void addCoveredTile(Tile* t, double nHP);
    virtual bool removeCoveredTile(Tile* t);
    virtual void updateActiveSpots();

    /*! \brief Exports the headers needed to recreate the Trap. It allows to extend Traps as much as wanted.
     * The content of the Trap will be exported by exportToPacket.
     */
    virtual void exportHeadersToStream(std::ostream& os);
    virtual void exportHeadersToPacket(ODPacket& os);
    //! \brief Exports the data of the RenderedMovableEntity
    virtual void exportToStream(std::ostream& os);
    virtual void importFromStream(std::istream& is);
    virtual void exportToPacket(ODPacket& os);
    virtual void importFromPacket(ODPacket& is);

    static std::string getFormat();
    friend std::istream& operator>>(std::istream& is, Trap::TrapType& tt);
    friend std::ostream& operator<<(std::ostream& os, const Trap::TrapType& tt);
    friend ODPacket& operator>>(ODPacket& is, Trap::TrapType& tt);
    friend ODPacket& operator<<(ODPacket& os, const Trap::TrapType& tt);

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
    virtual RenderedMovableEntity* notifyActiveSpotCreated(Tile* tile);
    virtual void notifyActiveSpotRemoved(Tile* tile);

    //! \brief Triggered when the trap is activated
    virtual void activate()
    { mIsActivated = true; }

    //! \brief Triggered when deactivated.
    virtual void deactivate()
    { mIsActivated = false; }

    //! \brief Tells whether the trap is activated.
    bool mIsActivated;

    uint32_t mReloadTime;
    double mMinDamage;
    double mMaxDamage;

    std::map<Tile*, uint32_t> mReloadTimeCounters;
};

#endif // TRAP_H
