/*!
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

#include "ODClientTest.h"

#include "game/SeatData.h"
#include "network/ClientNotification.h"
#include "network/ServerMode.h"
#include "network/ServerNotification.h"
#include "utils/LogManager.h"

#include <BoostTestTargetConfig.h>

#include <boost/algorithm/string.hpp>

#ifdef OD_VERSION
static const std::string OD_VERSION_STR = OD_VERSION;
#else
static const std::string OD_VERSION_STR = "undefined";
#endif

ODClientTest::ODClientTest(const std::vector<PlayerInfo>& players, uint32_t indexLocalPlayer) :
    mTurnNum(0),
    mContinueLoop(true),
    mIsActivated(false),
    mIsGameModeStarted(false),
    mPlayers(players),
    mLocalPlayerIndex(indexLocalPlayer)
{
    BOOST_CHECK(!players.empty());
    BOOST_CHECK(indexLocalPlayer < mPlayers.size());
    OD_LOG_INF("Local player is nick=" + mPlayers[mLocalPlayerIndex].mNick + ", id=" + Helper::toString(mPlayers[mLocalPlayerIndex].mPlayerId) + ", seatId=" + Helper::toString(mPlayers[mLocalPlayerIndex].mWantedSeatId));
}

ODClientTest::~ODClientTest()
{
    for(SeatData* seat : mSeats)
        delete seat;

    mSeats.clear();
}

bool ODClientTest::connect(const std::string& host, const int port, uint32_t timeout, const std::string& outputReplayFilename)
{
    if(!ODSocketClient::connect(host, port, timeout, outputReplayFilename))
    {
        OD_LOG_INF("Couldn't connect to server right away. Sleeping a bit before retry");
        // Units tests are supposed to be launched in parallel with OD exe. If we fail, we retry after
        // a small sleep 5 seconds to make sure the server had time to launch
        sf::sleep(sf::milliseconds(5000));
        if(!ODSocketClient::connect(host, port, timeout, outputReplayFilename))
            return false;
    }

    // Send a hello request to start the conversation with the server
    ODPacket packSend;
    packSend << ClientNotificationType::hello
        << std::string("OpenDungeons V ") + OD_VERSION_STR;
    send(packSend);

    return true;
}

void ODClientTest::disconnect(bool keepReplay)
{
    ODSocketClient::disconnect(keepReplay);

    // We wait for the server to terminate so that if a server is launched after this one, it doesn't collapse
    sf::sleep(sf::milliseconds(5000));
}

bool ODClientTest::processMessage(ServerNotificationType cmd, ODPacket& packetReceived)
{
    OD_LOG_INF("ServerNotificationType=" + ServerNotification::typeString(cmd));
    switch(cmd)
    {
        case ServerNotificationType::loadLevel:
        {
            std::string odVersion;
            BOOST_CHECK(packetReceived >> odVersion);

            OD_LOG_INF("odVersion=" + odVersion);
            // Map
            int32_t mapSizeX;
            int32_t mapSizeY;
            BOOST_CHECK(packetReceived >> mapSizeX);
            BOOST_CHECK(packetReceived >> mapSizeY);
            OD_LOG_INF("map x=" + Helper::toString(mapSizeX) + ", y=" + Helper::toString(mapSizeX));
            BOOST_CHECK(mapSizeX == 10);
            BOOST_CHECK(mapSizeY == 20);

            // Map infos
            std::string str;
            BOOST_CHECK(packetReceived >> str);
            OD_LOG_INF("level filename=" + str);
            BOOST_CHECK(packetReceived >> str);
            OD_LOG_INF("level description=" + str);
            BOOST_CHECK(packetReceived >> str);
            OD_LOG_INF("level background music=" + str);
            BOOST_CHECK(packetReceived >> str);
            OD_LOG_INF("level fight music=" + str);

            BOOST_CHECK(packetReceived >> str);
            OD_LOG_INF("level tileset=" + (str.empty() ? "default" : str));

            uint32_t nb;
            // We read the seats
            BOOST_CHECK(mSeats.empty());
            BOOST_CHECK(packetReceived >> nb);
            while(nb > 0)
            {
                --nb;
                SeatData* seat = new SeatData;
                seat->importFromPacket(packetReceived);
                mSeats.push_back(seat);
                OD_LOG_INF("Read seat id=" + Helper::toString(seat->getId()));
            }

            // We expect to have as many seats as players (+ rogue seat)
            BOOST_CHECK(mSeats.size() == (mPlayers.size() + 1));

            // We do not read the following data as it would imply to embed creature definitions
            // and many other stuff
            ODPacket packSend;
            packSend << ClientNotificationType::levelOK;
            send(packSend);
            return true;
        }

        case ServerNotificationType::pickNick:
        {
            ServerMode serverMode;
            BOOST_CHECK(packetReceived >> serverMode);
            OD_LOG_INF("serverMode=" + ServerModes::toString(serverMode));

            if(mPlayers.empty())
            {
                BOOST_CHECK(false);
                break;
            }

            ODPacket packSend;
            // We send the local player info
            PlayerInfo& player = mPlayers[mLocalPlayerIndex];
            packSend << ClientNotificationType::setNick << player.mNick;
            send(packSend);

            packSend.clear();
            packSend << ClientNotificationType::readyForSeatConfiguration;
            send(packSend);
            return true;
        }

        case ServerNotificationType::playerConfigChange:
        {
            mIsActivated = true;
            return true;
        }

        case ServerNotificationType::seatConfigurationRefresh:
        {
            int seatIdPacket;
            bool isSelected;

            // We should be the active player
            BOOST_CHECK(mIsActivated);

            // The server should send data for each seat. In the unit test map, we know we have 2 seats
            // We send the configuration to the server only if something changed
            bool isConfigured = true;
            for(PlayerInfo& player : mPlayers)
            {
                BOOST_CHECK(packetReceived >> seatIdPacket);
                BOOST_CHECK(player.mWantedSeatId == seatIdPacket);
                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t factionIndex;
                    BOOST_CHECK(packetReceived >> factionIndex);
                    OD_LOG_INF("wantedseat id=" + Helper::toString(player.mWantedSeatId) + ", factionIndex=" + Helper::toString(factionIndex));
                    BOOST_CHECK(player.mWantedFactionIndex == factionIndex);
                }

                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t playerId;
                    BOOST_CHECK(packetReceived >> playerId);
                    OD_LOG_INF("seat id=" + Helper::toString(player.mWantedSeatId) + ", playerId=" + Helper::toString(playerId));
                    BOOST_CHECK(player.mPlayerId == playerId);
                }

                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t teamId;
                    BOOST_CHECK(packetReceived >> teamId);
                    OD_LOG_INF("wantedseat id=" + Helper::toString(player.mWantedSeatId) + ", teamId=" + Helper::toString(teamId));
                    BOOST_CHECK(player.mWantedTeamId == teamId);
                }
            }

            if(isConfigured)
            {
                // If we are configured, we can launch the game
                ODPacket packSend;
                packSend << ClientNotificationType::seatConfigurationSet;
                send(packSend);
                OD_LOG_INF("Configuration done, launching the game");
                break;
            }

            OD_LOG_INF("Configuring the game");
            ODPacket packSend;
            packSend << ClientNotificationType::seatConfigurationRefresh;
            // We check if something changed. If yes, we send the refresh
            // Faction index should be 0 for every seat
            for(PlayerInfo& player : mPlayers)
            {
                packSend << player.mWantedSeatId;
                packSend << true << player.mWantedFactionIndex;
                packSend << true << player.mPlayerId;
                packSend << true << player.mWantedTeamId;
            }
            send(packSend);
            return true;
        }

        case ServerNotificationType::addPlayers:
        {
            uint32_t nbPlayers;
            BOOST_CHECK(packetReceived >> nbPlayers);
            OD_LOG_INF("Adding nbPlayers=" + Helper::toString(nbPlayers));
            // nbPlayers should be the number of human players
            uint32_t nbHumanPlayers = 0;
            for(PlayerInfo& player : mPlayers)
            {
                if(!player.mIsHuman)
                    continue;

                ++nbHumanPlayers;
            }

            BOOST_CHECK(nbPlayers <= nbHumanPlayers);
            for(uint32_t i = 0; i < nbPlayers; ++i)
            {
                std::string nick;
                int32_t id;
                BOOST_CHECK(packetReceived >> nick >> id);
                // We set the player id for the connected player
                bool isPLayerFound = false;
                for(PlayerInfo& player : mPlayers)
                {
                    if(player.mNick != nick)
                        continue;

                    // If the player is already set, we check the id is still good
                    isPLayerFound = true;
                    if(player.mPlayerId != -1)
                    {
                        BOOST_CHECK(player.mPlayerId == id);
                    }
                    else
                    {
                        player.mPlayerId = id;
                    }
                    break;
                }

                BOOST_CHECK(isPLayerFound);
            }
            return true;
        }

        case ServerNotificationType::removePlayers:
        {
            // We do not expect to have a player removed
            uint32_t nbPlayers;
            BOOST_CHECK(packetReceived >> nbPlayers);
            OD_LOG_INF("Removing nbPlayers=" + Helper::toString(nbPlayers));
            for(uint32_t i = 0; i < nbPlayers; ++i)
            {
                int32_t id;
                BOOST_CHECK(packetReceived >> id);
                OD_LOG_INF("Removing player id=" + Helper::toString(id));
            }
            BOOST_CHECK(false);
            break;
        }

        case ServerNotificationType::clientAccepted:
        {
            double turnsPerSecond;
            BOOST_CHECK(packetReceived >> turnsPerSecond);
            OD_LOG_INF("turnsPerSecond=" + Helper::toString(turnsPerSecond));

            int32_t nbPlayers;
            BOOST_CHECK(packetReceived >> nbPlayers);
            OD_LOG_INF("nbPlayers=" + Helper::toString(nbPlayers));
            // nbPlayers contains all the players + rogue
            if(static_cast<int32_t>(mPlayers.size() + 1) != nbPlayers)
            {
                OD_LOG_ERR("mPlayers.size()=" + Helper::toString(mPlayers.size()) + ", nbPlayers=" + Helper::toString(nbPlayers));
                BOOST_CHECK(false);
                break;
            }

            for(int32_t i = 0; i < nbPlayers; ++i)
            {
                std::string nick;
                int32_t playerId;
                int32_t seatId;
                int32_t teamId;
                BOOST_CHECK(packetReceived >> nick >> playerId >> seatId >> teamId);
                if(i <= 0)
                {
                    // We expected the rogue seat at first. We don't care about its nick
                    BOOST_CHECK(playerId == 0);
                    BOOST_CHECK(seatId == 0);
                    BOOST_CHECK(teamId == 0);
                    continue;
                }

                // We expect the players to be in the entered order
                PlayerInfo& player = mPlayers[i - 1];
                // For AI players, we do not care about nick
                if(player.mIsHuman)
                {
                    BOOST_CHECK(player.mNick == nick);
                }
                BOOST_CHECK(player.mPlayerId == playerId);
                BOOST_CHECK(player.mWantedSeatId == seatId);
                BOOST_CHECK(player.mWantedTeamId == teamId);
            }

            // We set the players seats
            for(PlayerInfo& player : mPlayers)
            {
                for(SeatData* seat : mSeats)
                {
                    if(seat->getId() != player.mWantedSeatId)
                        continue;

                    player.mSeat = seat;
                }
                BOOST_CHECK(player.mSeat != nullptr);
            }
            return true;
        }

        case ServerNotificationType::clientRejected:
        {
            BOOST_CHECK(false);
            break;
        }

        case ServerNotificationType::startGameMode:
        {
            mIsGameModeStarted = true;
            return true;
        }
        case ServerNotificationType::turnStarted:
        {
            BOOST_CHECK(mIsGameModeStarted);
            BOOST_CHECK(packetReceived >> mTurnNum);
            OD_LOG_INF("turnNum=" + Helper::toString(mTurnNum));
            handleTurnStarted(mTurnNum);

            ODPacket packSend;
            packSend << ClientNotificationType::ackNewTurn << mTurnNum;
            send(packSend);
            return true;
        }
        case ServerNotificationType::refreshPlayerSeat:
        {
            BOOST_CHECK(mPlayers[mLocalPlayerIndex].mSeat->importFromPacketForUpdate(packetReceived));
            BOOST_CHECK(packetReceived >> mPlayers[mLocalPlayerIndex].mGoals);
            break;
        }
        case ServerNotificationType::setObjectAnimationState:
        {
            std::string entityName;
            std::string animState;
            bool loop;
            bool playIdleWhenAnimationEnds;
            bool shouldSetWalkDirection;
            Ogre::Vector3 walkDirection(0, 0, 0);
            BOOST_CHECK(packetReceived >> entityName >> animState
                >> loop >> playIdleWhenAnimationEnds >> shouldSetWalkDirection);

            if(shouldSetWalkDirection)
            {
                BOOST_CHECK(packetReceived >> walkDirection);
            }

            animationPlayed(entityName, animState, loop, playIdleWhenAnimationEnds, shouldSetWalkDirection, walkDirection);
            break;
        }
        case ServerNotificationType::animatedObjectSetWalkPath:
        {
            std::string entityName;
            std::string walkAnim;
            std::string endAnim;
            bool loopEndAnim;
            bool playIdleWhenAnimationEnds;
            uint32_t nbDest;
            BOOST_CHECK(packetReceived >> entityName >> walkAnim >> endAnim);
            BOOST_CHECK(packetReceived >> loopEndAnim >> playIdleWhenAnimationEnds >> nbDest);
            std::vector<Ogre::Vector3> path;
            while(nbDest)
            {
                --nbDest;
                Ogre::Vector3 dest;
                BOOST_CHECK(packetReceived >> dest);
                path.push_back(dest);
            }

            //! We want to make sure animationPlayed is played for both animations (if required)
            if(!walkAnim.empty())
                animationPlayed(entityName, walkAnim, true, false, false, Ogre::Vector3::ZERO);
            if(!endAnim.empty())
                animationPlayed(entityName, endAnim, loopEndAnim, false, false, Ogre::Vector3::ZERO);
            break;
        }
        default:
        {
            break;
        }
    }
    return false;
}

SeatData* ODClientTest::getLocalSeat() const
{
    if(mLocalPlayerIndex >= mPlayers.size())
    {
        BOOST_CHECK(false);
        return nullptr;
    }

    if(mPlayers[mLocalPlayerIndex].mSeat == nullptr)
    {
        BOOST_CHECK(false);
        return nullptr;
    }

    return mPlayers[mLocalPlayerIndex].mSeat;
}

void ODClientTest::runFor(int32_t timeInMillis)
{
    if(!isConnected())
        return;

    mContinueLoop = true;

    sf::Clock clock;
    while(mContinueLoop &&
          (clock.getElapsedTime().asMilliseconds() < timeInMillis))
    {
        processClientSocketMessages();
        sf::sleep(sf::milliseconds(100));
    }
}

void ODClientTest::sendConsoleCmd(const std::string& cmd)
{
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens,
        cmd, boost::algorithm::is_space(),
        boost::algorithm::token_compress_on);
    uint32_t nb = tokens.size();

    ODPacket packSend;
    packSend << ClientNotificationType::askExecuteConsoleCommand << nb;
    for(const std::string& token : tokens)
        packSend << token;

    send(packSend);
}
