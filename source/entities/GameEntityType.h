/*!
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

#ifndef GAMEENTITYTYPE_H
#define GAMEENTITYTYPE_H

#include <iosfwd>

class ODPacket;

enum class GameEntityType
{
    unknown,
    creature,
    room,
    trap,
    tile,
    mapLight,
    spell,
    buildingObject,
    treasuryObject,
    chickenEntity,
    smallSpiderEntity,
    craftedTrap,
    missileObject,
    persistentObject,
    trapEntity,
    skillEntity,
    giftBoxEntity
};

ODPacket& operator<<(ODPacket& os, const GameEntityType& type);
ODPacket& operator>>(ODPacket& is, GameEntityType& type);
std::ostream& operator<<(std::ostream& os, const GameEntityType& type);
std::istream& operator>>(std::istream& is, GameEntityType& type);

#endif
