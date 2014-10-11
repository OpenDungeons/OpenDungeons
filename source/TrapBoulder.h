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

#ifndef TRAPBOULDER_H
#define TRAPBOULDER_H

#include "DirectionalTrap.h"

class TrapBoulder : public DirectionalTrap
{
public:
    TrapBoulder(GameMap* gameMap, int x, int y);

    static TrapBoulder* getTrapBoulderFromStream(GameMap* gameMap, std::istream& is);
    static TrapBoulder* getTrapBoulderFromPacket(GameMap* gameMap, ODPacket& is);

    virtual const TrapType getType() const
    { return TrapType::boulder; }

    friend std::istream& operator>>(std::istream& is, TrapBoulder *trap);
    friend std::ostream& operator<<(std::ostream& os, TrapBoulder *trap);
    friend ODPacket& operator>>(ODPacket& is, TrapBoulder *trap);
    friend ODPacket& operator<<(ODPacket& os, TrapBoulder *trap);
private:
    TrapBoulder(GameMap* gameMap);
};

#endif // TRAPBOULDER_H
