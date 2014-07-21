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

#include "ODSocketServer.h"

#include "LogManager.h"

#include <OgreStringConverter.h>

ODSocketServer::ODSocketServer()
{
    mNewClient = NULL;
    mIsConnected = false;
    mSockSelector.add(mSockListener);
}

ODSocketServer::~ODSocketServer()
{
    clearAllClientSockets();
    if(mNewClient != NULL)
    {
        delete mNewClient;
    }
}

bool ODSocketServer::createServer(int listeningPort)
{
    mIsConnected = false;
    mNewClient = new ODSocketClient;

    // As we use selector, there is no need to set the socket as not-blocking
    sf::Socket::Status status = mSockListener.listen(listeningPort);
    if (status != sf::Socket::Done)
    {
        LogManager::getSingleton().logMessage("ERROR : Could not listen to server port status="
            + Ogre::StringConverter::toString(status));
        return false;
    }
    mIsConnected = true;
    return true;
}

bool ODSocketServer::isConnected()
{
    return mIsConnected;
}

void ODSocketServer::doTask(int timeoutMs)
{
    mClockMainTask.restart();
    while((timeoutMs == 0) ||
          (timeoutMs > mClockMainTask.getElapsedTime().asMilliseconds()))
    {
        int timeoutMsAdjusted = 0;
        if(timeoutMs != 0)
        {
            // We adapt the timeout so that the function returns after timeoutMs
            // even if events occured
            timeoutMsAdjusted = timeoutMs - mClockMainTask.getElapsedTime().asMilliseconds();
        }
        // Check if a client tries to connect or to communicate
        if(mSockSelector.wait(sf::milliseconds(timeoutMsAdjusted)))
        {
            if(mSockSelector.isReady(mSockListener))
            {
                // New connection
                sf::Socket::Status status = mSockListener.accept(
                    mNewClient->mSockClient);
                if (status == sf::Socket::Done)
                {
                    // New connection
                    if(notifyNewConnection(mNewClient))
                    {
                        // The server wants to keep the client
                        mNewClient->mIsConnected = true;
                        mSockSelector.add(mNewClient->mSockClient);
                        mSockClients.push_back(mNewClient);

                        mNewClient = new ODSocketClient;
                    }
                }
                else
                {
                    // Error
                    LogManager::getSingleton().logMessage("ERROR : Could not listen to server port error="
                        + Ogre::StringConverter::toString(status));
                }
            }
            else
            {
                // TODO : How to know if a client disconnect ?
                // In forum threads, I have seen people using last communication time to check
                // if clients are still connected. We should check if there is no better way.
                for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
                {
                    ODSocketClient* client = *it;
                    if (!client)
                        continue;

                    if(mSockSelector.isReady(client->mSockClient))
                    {
                        notifyClientMessage(client);
                    }
                }
            }
        }
    }
}

ODSocketClient::ODComStatus ODSocketServer::receiveMsgFromClient(ODSocketClient* client, ODPacket& packetReceived)
{
    return client->recv(packetReceived);
}

ODSocketClient::ODComStatus ODSocketServer::sendMsgToClient(ODSocketClient* client, ODPacket& packetReceived)
{
    return client->send(packetReceived);
}

void ODSocketServer::sendMsgToAllClients(ODPacket& packetReceived)
{
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        if (!client)
            continue;

        client->send(packetReceived);
    }
}

void ODSocketServer::clearClientSocket(ODSocketClient* client)
{
    for(std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        if(*it == client)
        {
            mSockClients.erase(it);
            break;
        }
    }
    client->disconnect();
    delete client;
}

void ODSocketServer::clearAllClientSockets()
{
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        client->disconnect();
        delete client;
    }

    mSockClients.clear();
}
