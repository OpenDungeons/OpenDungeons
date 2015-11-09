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

#ifndef SEATDATA_H
#define SEATDATA_H


#include <OgreVector3.h>
#include <OgreColourValue.h>
#include <string>
#include <vector>
#include <iosfwd>
#include <cstdint>

class ODPacket;

enum class SkillType;
enum class RoomType;

//! \brief Base class for Seat that only embeds the data for the seat used through the network. It allows to use unit tests without having
//! to bring Seat dependencies (basically, the whole game)
class SeatData
{
public:
    // Constructors
    SeatData();

    inline int getId() const
    { return mId; }

    inline int getTeamId() const
    { return mTeamId; }

    inline unsigned int getNumClaimedTiles() const
    { return mNumClaimedTiles; }

    inline void setNumClaimedTiles(const unsigned int& num)
    { mNumClaimedTiles = num; }

    inline void incrementNumClaimedTiles()
    { ++mNumClaimedTiles; }

    void setTeamId(int teamId);

    inline const std::vector<int>& getAvailableTeamIds() const
    { return mAvailableTeamIds; }

    inline const std::string& getFaction() const
    { return mFaction; }

    inline void setFaction(const std::string& faction)
    { mFaction = faction; }

    inline const std::string& getColorId() const
    { return mColorId; }

    inline int getGold() const
    { return mGold; }

    inline int getGoldMax() const
    { return mGoldMax; }

    inline double getMana() const
    { return mMana; }

    inline double getManaDelta() const
    { return mManaDelta; }

    inline int getNumCreaturesFighters() const
    { return mNumCreaturesFighters; }

    inline int getNumCreaturesFightersMax() const
    { return mNumCreaturesFightersMax; }

    inline int getNumCreaturesWorkers() const
    { return mNumCreaturesWorkers; }

    uint32_t getNbRooms(RoomType roomType) const;

    inline const std::string& getPlayerType() const
    { return mPlayerType; }

    inline void setPlayerType(const std::string& playerType)
    { mPlayerType = playerType; }

    inline const std::vector<SkillType>& getSkillNotAllowed() const
    { return mSkillNotAllowed; }

    //! \brief functions to transfer Seat data through network. Note that they should not be overriden as
    //! unit tests use them to get the data from the Seat (without having to take Seat and all its dependencies)
    bool importFromPacket(ODPacket& is);
    void exportToPacket(ODPacket& os) const;
    bool importFromPacketForUpdate(ODPacket& is);
    void exportToPacketForUpdate(ODPacket& os) const;

protected:
    //! \brief The seat id. Allows to identify this seat. Must be unique per level file.
    int mId;

    //! \brief The team id of the player sitting in this seat.
    int mTeamId;

    //! \brief The type of player (can be Human or AI).
    std::string mPlayerType;

    //! \brief The name of the faction that this seat is playing as (can be Keeper or Hero).
    std::string mFaction;

    //! \brief The amount of 'keeper mana' the player has.
    double mMana;

    //! \brief The amount of 'keeper mana' the player gains/loses per turn, updated in GameMap::doTurn().
    double mManaDelta;

    //! \brief The starting camera location (in tile coordinates) of this seat.
    int mStartingX;
    int mStartingY;

    //! \brief The number of living creatures fighters under this seat's control
    int mNumCreaturesFighters;
    int mNumCreaturesFightersMax;
    int mNumCreaturesWorkers;

    //! \brief The actual color that this color index translates into.
    std::string mColorId;

    //! \brief Team ids this seat can use defined in the level file.
    std::vector<int> mAvailableTeamIds;

    //! \brief How many tiles have been claimed by this seat, updated in GameMap::doTurn().
    unsigned int mNumClaimedTiles;

    bool mHasGoalsChanged;

    //! \brief The total amount of gold coins in the keeper's treasury and in the dungeon heart.
    int mGold;

    //! \brief The total amount of gold coins that the keeper treasuries can have.
    int mGoldMax;

    //! \brief The number of rooms the player owns (room index being room type).
    //! Useful to display the first free tile on client side for example
    std::vector<uint32_t> mNbRooms;

    //! \brief Skills not allowed. Used on server side only
    std::vector<SkillType> mSkillNotAllowed;

    //! \brief Progress for current skill. Allows to display the progressbar on the client side
    SkillType mCurrentSkillType;
    float mCurrentSkillProgress;
};

#endif // SEATDATA_H
