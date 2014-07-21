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

#include "Weapon.h"

std::string Weapon::getFormat()
{
    //NOTE:  When this format changes changes to RoomPortal::spawnCreature() may be necessary.
    return "name\tdamage\trange\tdefense";
}

std::ostream& operator<<(std::ostream& os, Weapon *w)
{
    os << w->getName() << "\t" << w->getDamage() << "\t" << w->getRange() << "\t" << w->getDefense();
    return os;
}

std::istream& operator>>(std::istream& is, Weapon *w)
{
    std::string name;
    is >> name >> w->mDamage >> w->mRange >> w->mDefense;
    w->setName(name);
    w->setMeshName(name + ".mesh");

    return is;
}

ODPacket& operator<<(ODPacket& os, Weapon *w)
{
    os << w->getName() << w->getDamage() << w->getRange() << w->getDefense();
    return os;
}

ODPacket& operator>>(ODPacket& is, Weapon *w)
{
    std::string name;
    is >> name >> w->mDamage >> w->mRange >> w->mDefense;
    w->setName(name);
    w->setMeshName(name + ".mesh");

    return is;
}
