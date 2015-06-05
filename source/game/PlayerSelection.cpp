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

#include "game/PlayerSelection.h"

#include "modes/InputCommand.h"
#include "rooms/RoomType.h"
#include "spells/SpellType.h"
#include "traps/TrapType.h"

PlayerSelection::PlayerSelection()
{
    mCurrentAction = SelectedAction::none;
    mNewTrapType = TrapType::nullTrapType;
    mNewRoomType = RoomType::nullRoomType;
    mNewSpellType = SpellType::nullSpellType;
}

void PlayerSelection::setCurrentAction(SelectedAction action)
{
    mCurrentAction = action;
    mNewTrapType = TrapType::nullTrapType;
    mNewRoomType = RoomType::nullRoomType;
    mNewSpellType = SpellType::nullSpellType;
}
