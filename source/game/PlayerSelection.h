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
