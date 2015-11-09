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

#include "game/SeatData.h"

#include "game/SkillType.h"
#include "network/ODPacket.h"
#include "rooms/RoomType.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <istream>
#include <ostream>

SeatData::SeatData() :
    mId(-1),
    mTeamId(-1),
    mMana(1000),
    mManaDelta(0),
    mStartingX(0),
    mStartingY(0),
    mNumCreaturesFighters(0),
    mNumCreaturesFightersMax(0),
    mNumCreaturesWorkers(0),
    mNumClaimedTiles(0),
    mHasGoalsChanged(true),
    mGold(0),
    mGoldMax(0),
    mNbRooms(std::vector<uint32_t>(static_cast<uint32_t>(RoomType::nbRooms), 0)),
    mCurrentSkillType(SkillType::nullSkillType),
    mCurrentSkillProgress(0.0f)
{
}

void SeatData::setTeamId(int teamId)
{
    OD_ASSERT_TRUE_MSG(std::find(mAvailableTeamIds.begin(), mAvailableTeamIds.end(),
        teamId) != mAvailableTeamIds.end(), "Unknown team id=" + Helper::toString(teamId)
        + ", for seat id=" + Helper::toString(getId()));
    OD_ASSERT_TRUE_MSG(teamId != 0 || mId != 0, "Invalid rogue team id for seat id=" + Helper::toString(getId()));
    mTeamId = teamId;
}

uint32_t SeatData::getNbRooms(RoomType roomType) const
{
    uint32_t index = static_cast<uint32_t>(roomType);
    if(index >= mNbRooms.size())
    {
        OD_LOG_ERR("wrong index=" + Helper::toString(index) + ", size=" + Helper::toString(mNbRooms.size()));
        return 0;
    }

    return mNbRooms.at(index);
}

bool SeatData::importFromPacketForUpdate(ODPacket& is)
{
    // We only refresh data that changes over time (gold, mana, ...)
    OD_ASSERT_TRUE(is >> mGold);
    OD_ASSERT_TRUE(is >> mGoldMax);
    OD_ASSERT_TRUE(is >> mMana);
    OD_ASSERT_TRUE(is >> mManaDelta);
    OD_ASSERT_TRUE(is >> mNumClaimedTiles);
    OD_ASSERT_TRUE(is >> mNumCreaturesFighters);
    OD_ASSERT_TRUE(is >> mNumCreaturesFightersMax);
    OD_ASSERT_TRUE(is >> mNumCreaturesWorkers);
    OD_ASSERT_TRUE(is >> mHasGoalsChanged);
    mNbRooms.clear();
    uint32_t nb;
    OD_ASSERT_TRUE(is >> nb);
    while(nb > 0)
    {
        --nb;
        uint32_t nbRoom;
        OD_ASSERT_TRUE(is >> nbRoom);
        mNbRooms.push_back(nbRoom);
    }
    OD_ASSERT_TRUE(is >> mCurrentSkillType);
    OD_ASSERT_TRUE(is >> mCurrentSkillProgress);
    return true;
}

void SeatData::exportToPacketForUpdate(ODPacket& os) const
{
    os << mGold;
    os << mGoldMax;
    os << mMana;
    os << mManaDelta;
    os << mNumClaimedTiles;
    os << mNumCreaturesFighters;
    os << mNumCreaturesFightersMax;
    os << mNumCreaturesWorkers;
    os << mHasGoalsChanged;
    uint32_t nb = mNbRooms.size();
    os << nb;
    for(uint32_t nbRoom : mNbRooms)
        os << nbRoom;

    os << mCurrentSkillType;
    os << mCurrentSkillProgress;

}

void SeatData::exportToPacket(ODPacket& os) const
{
    os << mId << mTeamId << mPlayerType << mFaction << mStartingX
       << mStartingY;
    os << mColorId;
    os << mGold << mGoldMax;
    os << mMana << mManaDelta << mNumClaimedTiles;
    os << mNumCreaturesFighters << mNumCreaturesFightersMax;
    os << mNumCreaturesWorkers;
    os << mHasGoalsChanged;
    for(const uint32_t& nbRooms : mNbRooms)
    {
        os << nbRooms;
    }
    os << mCurrentSkillType;
    os << mCurrentSkillProgress;
    uint32_t nb;
    nb  = mAvailableTeamIds.size();
    os << nb;
    for(int teamId : mAvailableTeamIds)
        os << teamId;

    nb = mSkillNotAllowed.size();
    os << nb;
    for(SkillType resType : mSkillNotAllowed)
        os << resType;
}

bool SeatData::importFromPacket(ODPacket& is)
{
    is >> mId >> mTeamId >> mPlayerType;
    is >> mFaction >> mStartingX >> mStartingY;
    is >> mColorId;
    is >> mGold >> mGoldMax;
    is >> mMana >> mManaDelta >> mNumClaimedTiles;
    is >> mNumCreaturesFighters >> mNumCreaturesFightersMax;
    is >> mNumCreaturesWorkers;
    is >> mHasGoalsChanged;
    for(uint32_t& nbRooms : mNbRooms)
    {
        is >> nbRooms;
    }
    is >> mCurrentSkillType;
    is >> mCurrentSkillProgress;
    uint32_t nb;
    is >> nb;
    while(nb > 0)
    {
        --nb;
        int teamId;
        is >> teamId;
        mAvailableTeamIds.push_back(teamId);
    }

    is >> nb;
    while(nb > 0)
    {
        --nb;
        SkillType resType;
        is >> resType;
        mSkillNotAllowed.push_back(resType);
    }

    return true;
}
