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

#include "Building.h"

/*! \class Trap Trap.h
 *  \brief Defines a trap
 */
class Trap : public Building
{
 /* TODO: Trap and room share a lot of things, so we might want to make a shared
 *  base-class, like "Building" or something.
 */
public:
    enum TrapType
    {
        nullTrapType = 0, cannon, boulder
    };

    Trap();
    virtual ~Trap()
    {}

    static Trap* createTrap(TrapType nType, const std::vector<Tile*> &nCoveredTiles,
                            Seat *nControllingSeat, void* params = NULL);

    /** \brief Builds a trap for the current player.
     *  Builds a trap for the current player. Checks if the player has enough gold,
     *  if not, NULL is returned.
     *  \return The trap built, or NULL if the player does not have enough gold.
     */
    static Trap* buildTrap(GameMap* gameMap, Trap::TrapType nType,
                           const std::vector< Tile* >& coveredTiles,
                           Player* player, bool inEditor = false, void* params = NULL);

    static Trap* createTrapFromStream(const std::string& trapName, std::istream &is, GameMap* gameMap);

    inline const TrapType& getType() const
    { return mType; }

    static std::string getMeshNameFromTrapType(TrapType t);
    static TrapType getTrapTypeFromMeshName(std::string s);

    static int costPerTile(TrapType t);

    // Functions which can be overridden by child classes.
    virtual bool doUpkeep();
    virtual bool doUpkeep(Trap *t);

    virtual std::vector<GameEntity*> aimEnemy();
    virtual void damage(std::vector<GameEntity*>);

    virtual void addCoveredTile(Tile* t, double nHP = mDefaultTileHP);
    virtual void removeCoveredTile(Tile* t);
    virtual Tile* getCoveredTile(int index);
    std::vector<Tile*> getCoveredTiles();virtual unsigned int numCoveredTiles();
    virtual void clearCoveredTiles();

    static std::string getFormat();
    friend std::istream& operator>>(std::istream& is, Trap *t);
    friend std::ostream& operator<<(std::ostream& os, Trap *t);

    // Methods inherited from AttackableObject.
    //TODO:  Sort these into the proper places in the rest of the file.
    double getHP(Tile *tile);
    double getDefense() const;
    void takeDamage(double damage, Tile *tileTakingDamage);
    void recieveExp(double experience);

protected:
    int mReloadTime;
    int mReloadTimeCounter;
    double mMinDamage;
    double mMaxDamage;
    const static double mDefaultTileHP;// = 10.0;

    std::vector<Tile*> mCoveredTiles;
    std::map<Tile*, double> mTileHP;
    TrapType mType;
};

#endif // TRAP_H
