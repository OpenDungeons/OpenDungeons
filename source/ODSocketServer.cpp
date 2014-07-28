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
#include "Player.h"

#include "LogManager.h"

#include <OgreStringConverter.h>

ODSocketServer::ODSocketServer()
{
    mNewClient = NULL;
    mIsConnected = false;
}

ODSocketServer::~ODSocketServer()
{
    if(mIsConnected)
        stopServer();
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

    mSockSelector.add(mSockListener);

    LogManager::getSingleton().logMessage("Server connected and listening");
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
        bool isSockReady;
        if(timeoutMs != 0)
        {
            // We adapt the timeout so that the function returns after timeoutMs
            // even if events occured
            int timeoutMsAdjusted = std::max(1, timeoutMs - mClockMainTask.getElapsedTime().asMilliseconds());
            isSockReady = mSockSelector.wait(sf::milliseconds(timeoutMsAdjusted));
        }
        else
        {
            isSockReady = mSockSelector.wait(sf::Time::Zero);
        }

        // Check if a client tries to connect or to communicate
        if(isSockReady)
        {
            if(mSockSelector.isReady(mSockListener))
            {
                // New connection
                sf::Socket::Status status = mSockListener.accept(
                    mNewClient->mSockClient);

                if (status == sf::Socket::Done)
                {
                    // New connection
                    LogManager::getSingleton().logMessage("New client connected");
                    if(notifyNewConnection(mNewClient))
                    {
                        // The server wants to keep the client
                        mNewClient->mIsConnected = true;
                        mSockSelector.add(mNewClient->mSockClient);
                        mSockClients.push_back(mNewClient);

                        mNewClient = new ODSocketClient;
                        LogManager::getSingleton().logMessage("New client accepted");
                    }
                    else
                    {
                        LogManager::getSingleton().logMessage("New client refused");
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
                std::vector<ODSocketClient*>::iterator it = mSockClients.begin();
                while (it != mSockClients.end())
                {
                    ODSocketClient* client = *it;
                    if((mSockSelector.isReady(client->mSockClient)) &&
                       (!notifyClientMessage(client)))
                    {
                        // The server wants to remove the client
                        it = mSockClients.erase(it);
                        client->disconnect();
                        delete client;
                    }
                    else
                    {
                        ++it;
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

void ODSocketServer::stopServer()
{
    // TODO : if there is a server thread, wait for the end of its task
    mIsConnected = false;
    mSockListener.close();
    mSockSelector.clear();
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        client->disconnect();
        delete client;
    }

    mSockClients.clear();
    if(mNewClient != NULL)
    {
        delete mNewClient;
        mNewClient = NULL;
    }
}
