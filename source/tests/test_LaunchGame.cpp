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

#include "mocks/ODClientTest.h"

#include "game/SeatData.h"
#include "utils/LogManager.h"
#include "utils/LogSinkConsole.h"

#define BOOST_TEST_MODULE TestLaunchGame
#include <BoostTestTargetConfig.h>

class ODClientLaunchGame : public ODClientTest
{
public:
    ODClientLaunchGame(const std::vector<PlayerInfo>& players, uint32_t indexLocalPlayer) :
        ODClientTest(players, indexLocalPlayer)
    {}
};

BOOST_AUTO_TEST_CASE(test_LaunchGame)
{
    LogManager logMgr;
    logMgr.addSink(std::unique_ptr<LogSink>(new LogSinkConsole()));
    std::vector<PlayerInfo> players;

    // We know we have seat id = 1, 2
    int seatId = 1;
    // We know we have team id = 1, 2
    int32_t teamId = 1;
    // The first player is the local one
    for(uint32_t i = 0; i < 1; ++i)
    {
        PlayerInfo player;
        player.mNick = "PlayerStub" + Helper::toString(seatId);
        player.mWantedSeatId = seatId;
        player.mWantedTeamId = teamId;
        player.mIsHuman = true;
        // The player id will be set by the server
        player.mPlayerId = -1;
        // We take faction index 0 for every player (keeper faction)
        player.mWantedFactionIndex = 0;
        players.push_back(player);
        OD_LOG_INF("Adding player nick=" + player.mNick + ", id=" + Helper::toString(player.mPlayerId) + ", seatId=" + Helper::toString(player.mWantedSeatId));

        ++seatId;
        ++teamId;
    }

    // We add the AI players
    for(uint32_t i = 0; i < 2; ++i)
    {
        PlayerInfo playerAi;
        playerAi.mPlayerId = 0;
        playerAi.mWantedSeatId = seatId;
        playerAi.mWantedTeamId = teamId;
        playerAi.mWantedFactionIndex = 0;
        playerAi.mIsHuman = false;
        players.push_back(playerAi);
        OD_LOG_INF("Adding ai player id=" + Helper::toString(playerAi.mPlayerId) + ", seatId=" + Helper::toString(playerAi.mWantedSeatId));

        ++seatId;
        ++teamId;
    }

    uint32_t indexPlayer = 0;
    if(indexPlayer >= players.size())
    {
        BOOST_CHECK(false);
        return;
    }

    ODClientLaunchGame client(players, indexPlayer);
    BOOST_CHECK(client.connect("localhost", 32222, 10, "test_LaunchGameReplay"));

    BOOST_CHECK(client.isConnected());

    // We run for 15s. That should be enough to reach turn 10
    client.runFor(15000);

    client.disconnect(false);
    // We expect to have reached at least turn 10
    OD_LOG_INF("turnNum=" + Helper::toString(client.mTurnNum));
    BOOST_CHECK(client.mTurnNum > 10);

    if(client.getLocalSeat() == nullptr)
    {
        BOOST_CHECK(false);
        return;
    }
    // We check that gold for local player is 0 (no treasury) and that we have more mana than at game start
    SeatData& seatLocal = *client.getLocalSeat();
    BOOST_CHECK(seatLocal.getGold() == 0);
    OD_LOG_INF("gold local player=" + Helper::toString(seatLocal.getGold()));
    BOOST_CHECK(seatLocal.getMana() > 500);
    OD_LOG_INF("mana local player=" + Helper::toString(seatLocal.getMana()));

}
