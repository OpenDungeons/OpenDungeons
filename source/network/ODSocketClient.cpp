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

#include "utils/LogManager.h"
#include "utils/ResourceManager.h"

#include <OgreStringConverter.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

bool ODSocketClient::connect(const std::string& host, const int port)
{
    mSource = ODSource::none;
    // As we use selector, there is no need to set the socket as not-blocking
    sf::Socket::Status status = mSockClient.connect(host, port);
    if (status != sf::Socket::Done)
    {
        LogManager::getSingleton().logMessage("ERROR : Could not connect to distant server status="
            + Ogre::StringConverter::toString(status));
        return false;
    }
    mSockSelector.add(mSockClient);
    LogManager::getSingleton().logMessage("Connected to server successfully");
    static std::locale loc(std::wcout.getloc(), new boost::posix_time::time_facet("%Y%m%d_%H%M%S"));
    std::ostringstream ss;
    ss.imbue(loc);
    ss << "replay_" << boost::posix_time::second_clock::local_time();
    mOutputReplayFilename = ResourceManager::getSingleton().getReplayDataPath() + ss.str() + ".odr";

    mReplayOutputStream.open(mOutputReplayFilename, std::ios::out | std::ios::binary);
    mGameClock.restart();
    mSource = ODSource::network;
    return true;
}

bool ODSocketClient::replay(const std::string& filename)
{
    LogManager::getSingleton().logMessage("Reading replay from file " + filename);
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

    LogManager::getSingleton().logMessage("ERROR : Could not send data from client status="
        + Ogre::StringConverter::toString(status));
    return ODComStatus::Error;
}

ODSocketClient::ODComStatus ODSocketClient::recv(ODPacket& s)
{
    switch(mSource)
    {
        case ODSource::none:
        {
            // We should not try to send anything until connected
            OD_ASSERT_TRUE(false);
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
            else if((!mSockClient.isBlocking()) &&
                    (status == sf::Socket::NotReady))
            {
                    return ODComStatus::NotReady;
            }
            LogManager::getSingleton().logMessage("ERROR : Could not receive data from client status="
                + Ogre::StringConverter::toString(status));
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

