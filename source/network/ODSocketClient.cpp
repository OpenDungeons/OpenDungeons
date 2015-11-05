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

#include "ODSocketClient.h"
#include "network/ODPacket.h"
#include "network/ServerNotification.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

bool ODSocketClient::connect(const std::string& host, const int port, uint32_t timeout, const std::string& outputReplayFilename)
{
    mSource = ODSource::none;

    // As we use selector, there is no need to set the socket as not-blocking
    sf::Socket::Status status = mSockClient.connect(host, port, sf::milliseconds(timeout));
    if (status != sf::Socket::Done)
    {
        OD_LOG_ERR("Could not connect to distant server status="
            + Helper::toString(status));
        mSockClient.disconnect();
        return false;
    }
    mSockSelector.add(mSockClient);
    OD_LOG_INF("Connected to server successfully");

    mOutputReplayFilename = outputReplayFilename;

    mReplayOutputStream.open(mOutputReplayFilename, std::ios::out | std::ios::binary);
    mGameClock.restart();
    mSource = ODSource::network;
    return true;
}

bool ODSocketClient::replay(const std::string& filename)
{
    OD_LOG_INF("Reading replay from file " + filename);
    mReplayInputStream.open(filename, std::ios::in | std::ios::binary);
    mGameClock.restart();
    mSource = ODSource::file;
    return true;
}

void ODSocketClient::disconnect(bool keepReplay)
{
    mPendingTimestamp = -1;
    ODSource src = mSource;
    mSource = ODSource::none;
    switch(src)
    {
        case ODSource::none:
        {
            // Nothing to do
            return;
        }
        case ODSource::network:
        {
            // Remove any remaining client sockets from the socket selector,
            // if there is any left.
            mSockSelector.clear();
            mSockClient.disconnect();
            break;
        }
        case ODSource::file:
        {
            mReplayInputStream.close();
            return;
        }
        default:
            assert(false);
            break;
    }

    mReplayOutputStream.close();
    // Delete the replay newly created if asked to.
    if (!keepReplay)
        boost::filesystem::remove(mOutputReplayFilename);
    mOutputReplayFilename.clear();
}

bool ODSocketClient::isDataAvailable()
{
    switch(mSource)
    {
        case ODSource::none:
        {
            return false;
        }
        case ODSource::network:
        {
            // There is only 1 socket in the selector so it should be ready if
            // wait returns true but it doesn't hurt to return isReady...
            if(!mSockSelector.wait(sf::milliseconds(5)))
                return false;
            return mSockSelector.isReady(mSockClient);
        }
        case ODSource::file:
        {
            if(mReplayInputStream.eof())
                return false;

            if(mPendingTimestamp == -1)
                mPendingTimestamp = mPendingPacket.readPacket(mReplayInputStream);

            if(mPendingTimestamp < 0)
                return false;

            if(mPendingTimestamp < mGameClock.getElapsedTime().asMilliseconds())
                return true;

            return false;
        }
        default:
            assert(false);
            break;
    }

    return false;
}

ODSocketClient::ODComStatus ODSocketClient::send(ODPacket& s)
{
    if(mSource != ODSource::network)
        return ODComStatus::OK;

    sf::Socket::Status status = mSockClient.send(s.mPacket);
    if (status == sf::Socket::Done)
        return ODComStatus::OK;

    OD_LOG_ERR("Could not send data from client status="
        + Helper::toString(status));
    return ODComStatus::Error;
}

ODSocketClient::ODComStatus ODSocketClient::recv(ODPacket& s)
{
    switch(mSource)
    {
        case ODSource::none:
        {
            // We should not try to send anything until connected
            OD_LOG_ERR("Unexpected null server mode");
            return ODComStatus::Error;
        }
        case ODSource::network:
        {
            sf::Socket::Status status = mSockClient.receive(s.mPacket);
            if (status == sf::Socket::Done)
            {
                s.writePacket(mGameClock.getElapsedTime().asMilliseconds(),
                    mReplayOutputStream);
                return ODComStatus::OK;
            }

            if((!mSockClient.isBlocking()) &&
                    (status == sf::Socket::NotReady))
            {
                return ODComStatus::NotReady;
            }

            if(status == sf::Socket::Disconnected)
            {
                OD_LOG_WRN("Socket disconnected");
                return ODComStatus::Error;
            }
            OD_LOG_ERR("Could not receive data from client status=" + Helper::toString(status));
            return ODComStatus::Error;
        }
        case ODSource::file:
        {
            OD_ASSERT_TRUE(mPendingPacket != 0);
            s = mPendingPacket;
            mPendingTimestamp = -1;
            return ODComStatus::OK;
        }
        default:
            break;
    }
    return ODComStatus::Error;
}

bool ODSocketClient::isConnected()
{
    return mSource != ODSource::none;
}

void ODSocketClient::processClientSocketMessages()
{
    // If we receive message for a new turn, after processing every message,
    // we will refresh what is needed
    // We loop until no more data is available
    while(isConnected() && processOneClientSocketMessage());
}

bool ODSocketClient::processOneClientSocketMessage()
{
    if(!isDataAvailable())
        return false;

    ODPacket packetReceived;

    // Check if data available
    ODComStatus comStatus = recv(packetReceived);
    if(comStatus != ODComStatus::OK)
    {
        playerDisconnected();
        return false;
    }

    ServerNotificationType serverCommand;
    OD_ASSERT_TRUE(packetReceived >> serverCommand);

    return processMessage(serverCommand, packetReceived);
}
