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
#include "utils/LogManager.h"
#include "utils/LogSinkConsole.h"

#define BOOST_TEST_MODULE TestCreatures
#include <BoostTestTargetConfig.h>

class ODClientTestCreatures : public ODClientTest
{
public:
    ODClientTestCreatures(const std::vector<PlayerInfo>& players, uint32_t indexLocalPlayer) :
        ODClientTest(players, indexLocalPlayer),
        mResultTest(false)
    {}

    std::string mAwaitedEntityName;
    std::string mAwaitedEntityAnimation;
    bool mResultTest;

    virtual void animationPlayed(const std::string& entityName, const std::string& animState, bool loop,
        bool playIdleWhenAnimationEnds, bool shouldSetWalkDirection, const Ogre::Vector3& walkDirection) override
    {
        if(mAwaitedEntityName.empty())
            return;
        if(mAwaitedEntityAnimation.empty())
            return;
        if(entityName != mAwaitedEntityName)
            return;
        if(animState != mAwaitedEntityAnimation)
            return;

        mContinueLoop = false;
        mResultTest = true;
    }
};

BOOST_AUTO_TEST_CASE(test_Creatures)
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

    ODClientTestCreatures client(players, indexPlayer);
    BOOST_CHECK(client.connect("localhost", 32222, 10, "test_Traps"));

    BOOST_CHECK(client.isConnected());

    // We run for 5s. Then, we will spawn some creatures
    client.runFor(5000);

    std::string cmd;
    // We spawn a creature with hp=1 from seat 0 near the seat 1 traps and check it is killed
    cmd = "addcreature 0 Wyvern1 Wyvern 1 16 0 Wyvern 1 0 1 100 0 0 none none 4 none 0";
    client.sendConsoleCmd(cmd);

    client.mResultTest = false;
    client.mAwaitedEntityName = "Wyvern1";
    client.mAwaitedEntityAnimation = "Die";
    client.runFor(5000);

    BOOST_CHECK(client.mResultTest);

    // We spawn a creature with hp=1 from seat 1 near the seat 1 traps and check it is not killed
    cmd = "addcreature 1 Wyvern2 Wyvern 1 16 0 Wyvern 1 0 1 100 0 0 none none 4 none 0";
    client.sendConsoleCmd(cmd);

    client.mResultTest = false;
    client.mAwaitedEntityName = "Wyvern2";
    client.mAwaitedEntityAnimation = "Die";
    client.runFor(5000);

    BOOST_CHECK(!client.mResultTest);

    // We spawn a creature with hp=1 from seat 1 near the seat 0 traps and check it is killed
    cmd = "addcreature 1 Wyvern3 Wyvern 8 15 0 Wyvern 1 0 1 100 0 0 none none 4 none 0";
    client.sendConsoleCmd(cmd);

    client.mResultTest = false;
    client.mAwaitedEntityName = "Wyvern3";
    client.mAwaitedEntityAnimation = "Die";
    client.runFor(5000);

    BOOST_CHECK(client.mResultTest);

    // We spawn a creature with hp=1 from seat 1 near the seat 1 traps and check it is not killed
    cmd = "addcreature 0 Wyvern4 Wyvern 8 15 0 Wyvern 1 0 1 100 0 0 none none 4 none 0";
    client.sendConsoleCmd(cmd);

    client.mResultTest = false;
    client.mAwaitedEntityName = "Wyvern4";
    client.mAwaitedEntityAnimation = "Die";
    client.runFor(5000);

    BOOST_CHECK(!client.mResultTest);

    // We spawn a creature with hp=1 from seat 0 near the seat 1 unactivated trap and check it is not killed
    cmd = "addcreature 0 Wyvern5 Wyvern 5 13 0 Wyvern 1 0 1 100 0 0 none none 4 none 0";
    client.sendConsoleCmd(cmd);

    client.mResultTest = false;
    client.mAwaitedEntityName = "Wyvern5";
    client.mAwaitedEntityAnimation = "Die";
    client.runFor(5000);

    BOOST_CHECK(!client.mResultTest);

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
    SeatData& seatLocal = *client.getLocalSeat();
    BOOST_CHECK(seatLocal.getGold() == 0);
    OD_LOG_INF("gold local player=" + Helper::toString(seatLocal.getGold()));
    BOOST_CHECK(seatLocal.getMana() > 500);
    OD_LOG_INF("mana local player=" + Helper::toString(seatLocal.getMana()));

}
