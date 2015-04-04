#include "game/PlayerSelection.h"

#include "rooms/RoomType.h"
#include "spell/SpellType.h"
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
