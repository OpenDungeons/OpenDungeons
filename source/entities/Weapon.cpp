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

#include "entities/Weapon.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

Weapon* Weapon::load(std::stringstream& defFile)
{
    if (!defFile.good())
        return nullptr;

    Weapon* weapon = new Weapon();
    if(!update(weapon, defFile))
    {
        delete weapon;
        weapon = nullptr;
    }
    return weapon;
}
bool Weapon::update(Weapon* weapon, std::stringstream& defFile)
{
    std::string nextParam;
    bool exit = false;
    while (defFile.good())
    {
        if (exit)
            break;

        if(!(defFile >> nextParam))
            break;

        if (nextParam == "[/Equipment]" || nextParam == "[/EquipmentDefinitions]")
            break;

        if (nextParam == "Name")
        {
            defFile >> nextParam;
            weapon->mName = nextParam;
            continue;
        }

        if (nextParam != "[Stats]")
            continue;

        while (defFile.good())
        {
            if (exit)
                break;

            if(!(defFile >> nextParam))
                break;

            if (nextParam == "[/Stats]")
                break;

            // Handle ill-formed files.
            if (nextParam == "[/Equipment]" || nextParam == "[/EquipmentDefinitions]")
            {
                exit = true;
                break;
            }

            if (nextParam == "MeshName")
            {
                defFile >> nextParam;
                weapon->mMeshName = nextParam;
                continue;
            }

            if (nextParam == "PhysicalDamage")
            {
                defFile >> nextParam;
                weapon->mPhysicalDamage = Helper::toDouble(nextParam);
                continue;
            }

            if (nextParam == "MagicalDamage")
            {
                defFile >> nextParam;
                weapon->mMagicalDamage = Helper::toDouble(nextParam);
                continue;
            }

            if (nextParam == "Range")
            {
                defFile >> nextParam;
                weapon->mRange = Helper::toDouble(nextParam);
                continue;
            }

            if (nextParam == "PhysicalDefense")
            {
                defFile >> nextParam;
                weapon->mPhysicalDefense = Helper::toDouble(nextParam);
                continue;
            }

            if (nextParam == "MagicalDefense")
            {
                defFile >> nextParam;
                weapon->mMagicalDefense = Helper::toDouble(nextParam);
                continue;
            }
        }
    }

    if (weapon->mName.empty() || weapon->mMeshName.empty())
    {
        return false;
    }

    return true;
}

void Weapon::writeWeaponDiff(const Weapon* def1, const Weapon* def2, std::ofstream& file)
{
    file << "[Equipment]" << std::endl;
    file << "    Name\t" << def2->mName << std::endl;
    file << "    [Stats]" << std::endl;

    if(def1 == nullptr || (def1->mMeshName.compare(def2->mMeshName) != 0))
        file << "    MeshName\t" << def2->mMeshName << std::endl;

    if(def1 == nullptr || (def1->mPhysicalDamage != def2->mPhysicalDamage))
        file << "    PhysicalDamage\t" << def2->mPhysicalDamage << std::endl;

    if(def1 == nullptr || (def1->mMagicalDamage != def2->mMagicalDamage))
        file << "    MagicalDamage\t" << def2->mMagicalDamage << std::endl;

    if(def1 == nullptr || (def1->mRange != def2->mRange))
        file << "    Range\t" << def2->mRange << std::endl;

    if(def1 == nullptr || (def1->mPhysicalDefense != def2->mPhysicalDefense))
        file << "    PhysicalDefense\t" << def2->mPhysicalDefense << std::endl;

    if(def1 == nullptr || (def1->mMagicalDefense != def2->mMagicalDefense))
        file << "    MagicalDefense\t" << def2->mMagicalDefense << std::endl;

    file << "    [/Stats]" << std::endl;
    file << "[/Equipment]" << std::endl;
}
