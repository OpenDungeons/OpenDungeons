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

#ifndef PLAYER_H
#define PLAYER_H

#include "traps/Trap.h"
#include "rooms/Room.h"

#include <string>
#include <vector>

class Seat;
class Creature;
enum class SpellType;

/*! \brief The player cleass contains information about a human, or computer, player in the game.
 *
 * When a new player joins a game being hosted on a server the server will
 * allocate a new Player structure and fill it in with the appropriate values.
 * Its relevant information will then be sent to the other players in the game
 * so they are aware of its presence.  In the future if we decide to do a
 * single player game, thiis is where the computer driven strategy AI
 * calculations will take place.
 */
class Player
{
    friend class Seat;
public:
    enum SelectedAction
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

    enum class Direction
    {
        left = -1,
        right = 1
    };

    Player(GameMap* gameMap, int32_t id);

    inline int32_t getId() const
    { return mId; }

    const std::string& getNick() const
    { return mNickname; }

    Seat* getSeat()
    { return mSeat; }

    const Seat* getSeat() const
    { return mSeat; }

    void setNick (const std::string& nick)
    { mNickname = nick; }

    //! \brief A simple accessor function to return the number of creatures
    //! this player is holding in his/her hand that belongs to seat seat.
    //! If seat is nullptr, then returns the total number of creatures
    unsigned int numCreaturesInHand(const Seat* seat = nullptr) const;
    unsigned int numObjectsInHand() const;

    /*! \brief Check to see if it is the user or another player picking up the creature and act accordingly.
     *
     * This function takes care of all of the operations required for a player to
     * pick up an object (creature, treasury, ...).  If the player is the user we need to move the creature
     * oncreen to the "hand" as well as add the creature to the list of creatures
     * in our own hand, this is done by setting moveToHand to true.  If move to
     * hand is false we just hide the creature (and stop its AI, etc.), rather than
     * making it follow the cursor.
     */
    void pickUpEntity(MovableGameEntity *entity, bool isEditorMode);

    //! \brief Check to see the first object in hand can be dropped on Tile t and do so if possible.
    bool isDropHandPossible(Tile *t, unsigned int index = 0, bool isEditorMode = false);

    //! \brief Drops the creature on tile t. Returns the dropped creature
    MovableGameEntity* dropHand(Tile *t, unsigned int index = 0);

    void rotateHand(Direction d);

    //! \brief Clears all creatures that a player might have in his hand
    void clearObjectsInHand();

    //! \brief Clears all creatures that a player might have in his hand
    void notifyNoMoreDungeonTemple();

    inline bool getIsHuman() const
    { return mIsHuman; }

    inline void setIsHuman(bool isHuman)
    { mIsHuman = isHuman; }

    inline const std::vector<MovableGameEntity*>& getObjectsInHand()
    { return mObjectsInHand; }

    inline const Room::RoomType getNewRoomType()
    { return mNewRoomType; }

    inline const SelectedAction getCurrentAction()
    { return mCurrentAction; }

    inline void setNewRoomType(Room::RoomType newRoomType)
    { mNewRoomType = newRoomType; }

    inline const Trap::TrapType getNewTrapType() const
    { return mNewTrapType; }

    inline void setNewTrapType(Trap::TrapType newTrapType)
    { mNewTrapType = newTrapType; }

    inline const SpellType getNewSpellType() const
    { return mNewSpellType; }

    inline void setNewSpellType(SpellType newSpellType)
    { mNewSpellType = newSpellType; }

    void setCurrentAction(SelectedAction action);

    //! \brief Notify the player is fighting
    //! Should be called on the server game map for human players only
    void notifyFighting();

    //! \brief Notify the player is fighting
    //! Should be called on the server game map for human players only
    void notifyNoTreasuryAvailable();

    //! \brief Allows to handle timed events like fighting music
    //! Should be called on the server game map for human players only
    void updateTime(Ogre::Real timeSinceLastUpdate);

private:
    //! \brief Player ID is only used during seat configuration phase
    //! During the game, one should use the seat ID to identify a player because
    //! every AI player has an id = 0.
    //! ID is unique only for human players
    int32_t mId;
    //! \brief Room, trap or Spell tile type the player is currently willing to place on map.
    Room::RoomType mNewRoomType;
    Trap::TrapType mNewTrapType;
    SpellType mNewSpellType;
    SelectedAction mCurrentAction;

    GameMap* mGameMap;
    Seat *mSeat;

    //! \brief The nickname used in chat, etc.
    std::string mNickname;

    //! \brief The creature the player has got in hand.
    std::vector<MovableGameEntity*> mObjectsInHand;

    //! True: player is human. False: player is a computer/inactive.
    bool mIsHuman;

    //! \brief This counter tells for how much time is left before considering
    //! the player to be out of struggle.
    //! When > 0, the player is considered attacking or being attacked.
    //! This member is used to trigger the calm or fighting music when incarnating
    //! the local player.
    float mFightingTime;

    //! \brief This counter tells for how much time is left before considering
    //! the player should be notified again that he has no free space to store gold.
    float mNoTreasuryAvailableTime;

    bool mIsPlayerLostSent;

    //! \brief A simple mutator function to put the given entity into the player's hand,
    //! note this should NOT be called directly for creatures on the map,
    //! for that you should use the correct function like pickUpEntity() instead.
    void addEntityToHand(MovableGameEntity *entity);
};

#endif // PLAYER_H
