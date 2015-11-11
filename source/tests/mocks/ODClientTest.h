/*!
 *
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

#ifndef ODCLIENTTEST_H
#define ODCLIENTTEST_H

#include "network/ODSocketClient.h"

#include <string>

class SeatData;

class PlayerInfo
{
public:
    PlayerInfo() :
        mPlayerId(-1),
        mWantedSeatId(-1),
        mWantedTeamId(-1),
        mWantedFactionIndex(-1),
        mIsHuman(false),
        mSeat(nullptr)
    {}

    std::string mNick;
    int32_t mPlayerId;
    int mWantedSeatId;
    int32_t mWantedTeamId;
    int32_t mWantedFactionIndex;
    bool mIsHuman;
    SeatData* mSeat;
    std::string mGoals;
};

class ODClientTest : public ODSocketClient
{
public:
    ODClientTest(const std::vector<PlayerInfo>& players, uint32_t indexLocalPlayer);

    virtual ~ODClientTest();

    bool connect(const std::string& host, const int port, uint32_t timeout, const std::string& outputReplayFilename) override;
    void disconnect(bool keepReplay) override;

    void runFor(int32_t timeInMillis);

    void sendConsoleCmd(const std::string& cmd);

    const std::vector<SeatData*>& getSeats() const
    { return mSeats; }

    SeatData* getLocalSeat() const;

    // Allows to check that the server correctly launched and sent new turns
    int64_t mTurnNum;

protected:
    bool processMessage(ServerNotificationType cmd, ODPacket& packetReceived) override;
    virtual void handleTurnStarted(int64_t turnNum)
    {}
    virtual void handleSetAnimationState(const std::string& entityName, const std::string& animState,
        bool loop, bool shouldSetWalkDirection, const Ogre::Vector3& walkDirection)
    {}

    //! \brief This boolean can be used in the handle* functions to stop the processing loop
    //! before the end of the timeout
    bool mContinueLoop;

private:
    bool mIsActivated;
    bool mIsGameModeStarted;
    std::vector<PlayerInfo> mPlayers;
    std::vector<SeatData*> mSeats;
    uint32_t mLocalPlayerIndex;
};

#endif // ODCLIENTTEST_H
