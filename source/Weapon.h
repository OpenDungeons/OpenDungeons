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

#ifndef WEAPON_H
#define WEAPON_H

#include "GameEntity.h"

#include <string>
#include <istream>
#include <ostream>

class Creature;
class ODPacket;

class Weapon : public GameEntity
{
public:
    Weapon(GameMap*           gameMap,
           const std::string& name,
           double             damage,
           double             range,
           double             defense,
           const std::string& handString,
           Creature*          parent = NULL);

    virtual ~Weapon()
    {
        destroyMesh();
    }

    virtual std::string getOgreNamePrefix() const { return "Weapon_"; }

    static std::string getFormat();
    friend std::ostream& operator<<(std::ostream& os, Weapon *w);
    friend std::istream& operator>>(std::istream& is, Weapon *w);
    friend ODPacket& operator<<(ODPacket& os, Weapon *w);
    friend ODPacket& operator>>(ODPacket& is, Weapon *w);

    inline double getDamage() const
    { return mDamage; }

    inline void setDamage(const double nDamage)
    { mDamage = nDamage; }

    inline double getPhysicalDefense() const
    { return mDefense; }

    inline void setPhysicalDefense(const double nDefense)
    { mDefense = nDefense; }

    inline const std::string& getHandString() const
    { return mHandString; }

    inline void setHandString(const std::string& nHandString)
    { mHandString = nHandString; }

    inline Creature* getParentCreature() const
    { return mParentCreature; }

    inline void setParentCreature(Creature* nParent)
    { mParentCreature = nParent; }

    inline double getRange() const
    { return mRange; }

    inline void setRange(const double nRange)
    { mRange = nRange; }

    void doUpkeep()
    {}

    void receiveExp(double experience)
    {}

    void takeDamage(GameEntity* attacker, double damage, Tile* tileTakingDamage)
    {}

    double getHP(Tile* tile) const
    { return 0.0; }

    std::vector<Tile*> getCoveredTiles()
    { return std::vector<Tile*>(); }

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
private:
    std::string mHandString;
    double      mDamage;
    double      mRange;
    double      mDefense;
    Creature*   mParentCreature;
};

#endif // WEAPON_H
