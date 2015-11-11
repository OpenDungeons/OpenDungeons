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

#include "mocks/ODClientTest.h"

#include "game/SeatData.h"
#include "network/ClientNotification.h"
#include "rooms/RoomType.h"
#include "utils/LogManager.h"
#include "utils/LogSinkConsole.h"

#define BOOST_TEST_MODULE TestRooms
#include <BoostTestTargetConfig.h>

class ODClientTestRooms : public ODClientTest
{
public:
    ODClientTestRooms(const std::vector<PlayerInfo>& players, uint32_t indexLocalPlayer) :
        ODClientTest(players, indexLocalPlayer)
    {}

};

BOOST_AUTO_TEST_CASE(test_Rooms)
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

    ODClientTestRooms client(players, indexPlayer);
    BOOST_CHECK(client.connect("localhost", 32222, 10, "test_RoomsReplay"));

    BOOST_CHECK(client.isConnected());

    // We run for 5s. Then, we will build a room
    client.runFor(5000);

    if(!client.isConnected())
    {
        BOOST_CHECK(false);
        return;
    }

    //! Now that we are connected, the local seat should be ok. We check we have 0 gold
    SeatData& seatLocal = *client.getLocalSeat();
    BOOST_CHECK(seatLocal.getGold() == 0);

    std::string cmd;
    // We build a 1 tile treasury
    ODPacket packSend;
    RoomType type;
    uint32_t nb;
    int32_t x;
    int32_t y;

    packSend.clear();
    type = RoomType::treasury;
    packSend << ClientNotificationType::askBuildRoom << type;
    nb = 1;
    x = 1;
    y = 10;
    packSend << nb << x << y;
    client.send(packSend);

    client.runFor(3000);

    // Then, we send a console command to give gold
    cmd = "addgold 1 1000";
    client.sendConsoleCmd(cmd);

    client.runFor(3000);

    BOOST_CHECK(seatLocal.getGold() == 1000);

    // We sell the treasury. We would expect to have 0 gold
    packSend.clear();
    packSend << ClientNotificationType::askSellRoomTiles;
    nb = 1;
    x = 1;
    y = 10;
    packSend << nb << x << y;
    client.send(packSend);

    client.runFor(3000);
    BOOST_CHECK(seatLocal.getGold() == 0);

    // We build again a treasury and add 1000 gold. Then, we will build another room
    packSend.clear();
    type = RoomType::treasury;
    packSend << ClientNotificationType::askBuildRoom << type;
    nb = 1;
    x = 1;
    y = 10;
    packSend << nb << x << y;
    client.send(packSend);

    client.runFor(3000);

    // Then, we send a console command to give gold
    cmd = "addgold 1 1000";
    client.sendConsoleCmd(cmd);

    client.runFor(3000);

    BOOST_CHECK(seatLocal.getGold() == 1000);

    // We build again a treasury and add 1000 gold. Then, we will build another room
    packSend.clear();
    type = RoomType::treasury;
    packSend << ClientNotificationType::askBuildRoom << type;
    nb = 3;
    packSend << nb;
    x = 1;
    y = 11;
    packSend << x << y;
    x = 1;
    y = 12;
    packSend << x << y;
    x = 1;
    y = 13;
    packSend << x << y;
    client.send(packSend);

    client.runFor(3000);

    // Then, we send a console command to give gold
    cmd = "addgold 1 2500";
    client.sendConsoleCmd(cmd);

    client.runFor(3000);

    int gold = seatLocal.getGold();
    OD_LOG_INF("seat1 gold=" + Helper::toString(gold));
    BOOST_CHECK(gold > 2500);

    // We build a dormitory
    packSend.clear();
    type = RoomType::dormitory;
    packSend << ClientNotificationType::askBuildRoom << type;
    nb = 9;
    packSend << nb;
    for(int32_t i = 0; i < 3; ++i)
    {
        for(int32_t j = 0; j < 3; ++j)
        {
            x = 2 + i;
            y = 10 + j;
            packSend << x << y;
        }
    }
    client.send(packSend);

    client.runFor(3000);

    // We check that we have gold
    OD_LOG_INF("seat1 gold=" + Helper::toString(seatLocal.getGold()));
    BOOST_CHECK(seatLocal.getGold() < gold);

    // We expect to have reached at least turn 10
    OD_LOG_INF("turnNum=" + Helper::toString(client.mTurnNum));
    BOOST_CHECK(client.mTurnNum > 0);

    client.disconnect(false);

    if(client.getLocalSeat() == nullptr)
    {
        BOOST_CHECK(false);
        return;
    }
    // We check that gold for local player is 0 (no treasury) and that we have more mana than at game start
    OD_LOG_INF("gold local player=" + Helper::toString(seatLocal.getGold()));
    BOOST_CHECK(seatLocal.getMana() > 500);
    OD_LOG_INF("mana local player=" + Helper::toString(seatLocal.getMana()));

}
