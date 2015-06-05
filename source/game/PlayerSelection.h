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

#ifndef PLAYERSELECTION_H
#define PLAYERSELECTION_H

enum class RoomType;
enum class TrapType;
enum class SpellType;

enum class SelectedAction
{
    none,
    buildRoom,
    buildTrap,
    castSpell,
    changeTile,
    selectTile,
    destroyRoom,
    destroyTrap
};

//! \brief Class to store what the client-side player has currently selected.
class PlayerSelection
{
public:
    PlayerSelection();

    inline SelectedAction getCurrentAction() const
    { return mCurrentAction; }

    //! \brief Set the players current action. This also resets room/trap/spell type.
    void setCurrentAction(SelectedAction action);

    inline RoomType getNewRoomType() const
    { return mNewRoomType; }

    inline void setNewRoomType(RoomType newRoomType)
    { mNewRoomType = newRoomType; }

    inline TrapType getNewTrapType() const
    { return mNewTrapType; }

    inline void setNewTrapType(TrapType newTrapType)
    { mNewTrapType = newTrapType; }

    inline SpellType getNewSpellType() const
    { return mNewSpellType; }

    inline void setNewSpellType(SpellType newSpellType)
    { mNewSpellType = newSpellType; }

private:
    //! \brief Room, trap or Spell tile type the player has currently selected for an action
    RoomType mNewRoomType;
    TrapType mNewTrapType;
    SpellType mNewSpellType;
    SelectedAction mCurrentAction;
};

#endif // PLAYERSELECTION_H
