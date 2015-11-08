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

#include "network/ClientNotification.h"
#include "network/ServerMode.h"
#include "network/ServerNotification.h"
#include "utils/LogManager.h"

#include <BoostTestTargetConfig.h>

#ifdef OD_VERSION
static const std::string OD_VERSION_STR = OD_VERSION;
#else
static const std::string OD_VERSION_STR = "undefined";
#endif

ODClientTest::ODClientTest(const std::vector<PlayerInfo>& players) :
    mTurnNum(0),
    mIsActivated(false),
    mIsGameModeStarted(false),
    mPlayers(players)
{
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
            // Local player is the first one
            PlayerInfo& player = mPlayers[0];
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
                BOOST_CHECK(player.mSeatId == seatIdPacket);
                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t factionIndex;
                    BOOST_CHECK(packetReceived >> factionIndex);
                    OD_LOG_INF("seat id=" + Helper::toString(player.mSeatId) + ", factionIndex=" + Helper::toString(factionIndex));
                    BOOST_CHECK(player.mFactionIndex == factionIndex);
                }

                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t playerId;
                    BOOST_CHECK(packetReceived >> playerId);
                    OD_LOG_INF("seat id=" + Helper::toString(player.mSeatId) + ", playerId=" + Helper::toString(playerId));
                    BOOST_CHECK(player.mId == playerId);
                }

                BOOST_CHECK(packetReceived >> isSelected);
                isConfigured &= isSelected;
                if(isSelected)
                {
                    int32_t teamId;
                    BOOST_CHECK(packetReceived >> teamId);
                    OD_LOG_INF("seat id=" + Helper::toString(player.mSeatId) + ", teamId=" + Helper::toString(teamId));
                    BOOST_CHECK(player.mTeamId == teamId);
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
                packSend << player.mSeatId;
                packSend << true << player.mFactionIndex;
                packSend << true << player.mId;
                packSend << true << player.mTeamId;
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
                    if(player.mId != -1)
                    {
                        BOOST_CHECK(player.mId == id);
                    }
                    else
                    {
                        player.mId = id;
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
                BOOST_CHECK(player.mId == playerId);
                BOOST_CHECK(player.mSeatId == seatId);
                BOOST_CHECK(player.mTeamId == teamId);
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
            OD_ASSERT_TRUE(packetReceived >> mTurnNum);
            OD_LOG_INF("turnNum=" + Helper::toString(mTurnNum));
            ODPacket packSend;
            packSend << ClientNotificationType::ackNewTurn << mTurnNum;
            send(packSend);
            return true;
        }
        default:
        {
            break;
        }
    }
    return false;
}

void ODClientTest::runFor(int32_t timeInMillis)
{
    if(!isConnected())
        return;

    sf::Clock clock;
    while(clock.getElapsedTime().asMilliseconds() < timeInMillis)
    {
        processClientSocketMessages();
        sf::sleep(sf::milliseconds(100));
    }
}
