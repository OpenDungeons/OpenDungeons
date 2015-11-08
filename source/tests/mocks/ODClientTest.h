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

class PlayerInfo
{
public:
    std::string mNick;
    int32_t mId;
    int mSeatId;
    int32_t mTeamId;
    int32_t mFactionIndex;
    bool mIsHuman;
};

class ODClientTest : public ODSocketClient
{
public:
    ODClientTest(const std::vector<PlayerInfo>& players);

    virtual ~ODClientTest()
    {}

    bool connect(const std::string& host, const int port, uint32_t timeout, const std::string& outputReplayFilename) override;

    void runFor(int32_t timeInMillis);

    // Allows to check that the server correctly launched and sent new turns
    int64_t mTurnNum;

protected:
    bool processMessage(ServerNotificationType cmd, ODPacket& packetReceived) override;

private:
    bool mIsActivated;
    bool mIsGameModeStarted;
    std::vector<PlayerInfo> mPlayers;
};

#endif // ODCLIENTTEST_H
