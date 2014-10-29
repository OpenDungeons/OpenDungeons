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

#include <string>
#include <istream>
#include <ostream>

class Creature;
class ODPacket;
class WeaponDefinition;

class Weapon
{
public:
    Weapon(const std::string& name) :
       mName(name),
       mPhysicalDamage(0.0),
       mMagicalDamage(0.0),
       mRange(0.0),
       mPhysicalDefense(0.0),
       mMagicalDefense(0.0)
    {}

    virtual ~Weapon()
    {
    }

    //! \brief Loads a definition from the equipment file sub [Equipment][/Equipment] part
    //! \returns A Weapon if valid, nullptr otherwise.
    static Weapon* load(std::stringstream& defFile);
    static bool update(Weapon* weapon, std::stringstream& defFile);
    //! \brief Writes the differences between def1 and def2 in the given file. Note that def1 can be null. In
    //! this case, every parameters in def2 will be written. def2 cannot be null.
    static void writeWeaponDiff(const Weapon* def1, const Weapon* def2, std::ofstream& file);

    inline const std::string getOgreNamePrefix() const
    { return "Weapon_"; }

    inline const std::string& getName() const
    { return mName; }

    inline const std::string& getMeshName() const
    { return mMeshName; }

    inline double getPhysicalDamage() const
    { return mPhysicalDamage; }

    inline double getMagicalDamage() const
    { return mMagicalDamage; }

    inline double getRange() const
    { return mRange; }

    inline double getPhysicalDefense() const
    { return mPhysicalDefense; }

    inline double getMagicalDefense() const
    { return mMagicalDefense; }

private:
    Weapon() :
       mPhysicalDamage(0.0),
       mMagicalDamage(0.0),
       mRange(0.0),
       mPhysicalDefense(0.0),
       mMagicalDefense(0.0)
    {}

    std::string     mName;
    //! \brief the Weapon name this class extends. Can be empty if no class extended
    std::string     mBaseDefinition;
    std::string     mMeshName;
    double          mPhysicalDamage;
    double          mMagicalDamage;
    double          mRange;
    double          mPhysicalDefense;
    double          mMagicalDefense;
};

#endif // WEAPON_H
