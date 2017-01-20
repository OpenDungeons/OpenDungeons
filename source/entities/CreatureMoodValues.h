/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef CREATUREMOODVALUES_H
#define CREATUREMOODVALUES_H

#include <cstdint>

//! \brief Defines the bit array of the creature mood that will be
//! used to let the player know what happens to the creature
namespace CreatureMoodValues
{
    const uint32_t Nothing = 0x0000;
    const uint32_t Angry = 0x0001;
    const uint32_t Furious = 0x0002;
    const uint32_t GetFee = 0x0004;
    const uint32_t LeaveDungeon = 0x0008;
    const uint32_t KoDeath = 0x0010;
    const uint32_t Hungry = 0x0020;
    const uint32_t Tired = 0x0040;
    const uint32_t KoTemp = 0x0080;
    const uint32_t InJail = 0x0100;
    const uint32_t GoToCallToWar = 0x0200;
    // To know if a creature is KO
    const uint32_t KoDeathOrTemp = KoTemp | KoDeath;
    // Mood filters for creatures in prison that every player will see
    const uint32_t MoodPrisonFiltersAllPlayers = InJail;
    // Mood filters for creatures in prison that prison allied will see
    const uint32_t MoodPrisonFiltersPrisonAllies = KoTemp | InJail;
}

#endif // CREATUREMOODVALUES_H
