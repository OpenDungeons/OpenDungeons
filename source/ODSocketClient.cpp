/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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
#include "ODPacket.h"

#include "LogManager.h"

#include <OgreStringConverter.h>

bool ODSocketClient::connect(const std::string& host, const int port)
{
    mIsConnected = false;

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
    mIsConnected = true;
    return true;
}

void ODSocketClient::disconnect()
{
    mIsConnected = false;
    mSockSelector.remove(mSockClient);
    mSockClient.disconnect();
}

bool ODSocketClient::isDataAvailable()
{
    // There is only 1 socket in the selector so it ld be ready if
    // wait returns true but it doesn't hurt to return isReady...
    if(mSockSelector.wait(sf::milliseconds(5)))
        return mSockSelector.isReady(mSockClient);
    else
        return false;
}

ODSocketClient::ODComStatus ODSocketClient::send(ODPacket& s)
{
    sf::Socket::Status status = mSockClient.send(s.packet);
    if (status == sf::Socket::Done)
    {
        return ODComStatus::OK;
    }
    LogManager::getSingleton().logMessage("ERROR : Could not send data from client status="
        + Ogre::StringConverter::toString(status));
    return ODComStatus::Error;
}

ODSocketClient::ODComStatus ODSocketClient::recv(ODPacket& s)
{
    sf::Socket::Status status = mSockClient.receive(s.packet);
    if (status == sf::Socket::Done)
    {
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

bool ODSocketClient::isConnected()
{
    return mIsConnected;
}

