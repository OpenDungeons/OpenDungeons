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

#include "ODSocketServer.h"
#include "network/ODPacket.h"
#include "game/Player.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <SFML/System.hpp>

ODSocketServer::ODSocketServer():
    mThread(nullptr),
    mNewClient(nullptr),
    mIsConnected(false)
{
}

ODSocketServer::~ODSocketServer()
{
    if(mIsConnected)
        stopServer();
}

bool ODSocketServer::createServer(int listeningPort)
{
    mIsConnected = false;

    // As we use selector, there is no need to set the socket as not-blocking
    sf::Socket::Status status = mSockListener.listen(listeningPort);
    if (status != sf::Socket::Done)
    {
        LogManager::getSingleton().logMessage("ERROR : Could not listen to server port status="
            + Helper::toString(status));
        return false;
    }

    mSockSelector.add(mSockListener);
    mNewClient = new ODSocketClient;
    mIsConnected = true;
    LogManager::getSingleton().logMessage("Server connected and listening");
    mThread = new sf::Thread(&ODSocketServer::serverThread, this);
    mThread->launch();

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
        bool isSockReady;
        if(timeoutMs != 0)
        {
            // We adapt the timeout so that the function returns after timeoutMs
            // even if events occurred
            int timeoutMsAdjusted = std::max(1, timeoutMs - mClockMainTask.getElapsedTime().asMilliseconds());
            isSockReady = mSockSelector.wait(sf::milliseconds(timeoutMsAdjusted));
        }
        else
        {
            isSockReady = mSockSelector.wait(sf::Time::Zero);
        }

        // Check if a client tries to connect or to communicate
        if(!isSockReady)
            continue;

        if(mSockSelector.isReady(mSockListener))
        {
            // New connection
            sf::Socket::Status status = mSockListener.accept(mNewClient->mSockClient);

            if (status == sf::Socket::Done)
            {
                // New connection
                LogManager::getSingleton().logMessage("New client connected.");
                if(notifyNewConnection(mNewClient))
                {
                    // The server wants to keep the client
                    mNewClient->mSource = ODSocketClient::ODSource::network;
                    mSockSelector.add(mNewClient->mSockClient);
                    mSockClients.push_back(mNewClient);

                    mNewClient = new ODSocketClient;
                    LogManager::getSingleton().logMessage("New client accepted.");
                }
                else
                {
                    LogManager::getSingleton().logMessage("New client refused.");
                }
            }
            else
            {
                // Error
                LogManager::getSingleton().logMessage("ERROR : Could not listen to server port error="
                    + Helper::toString(status));
            }
        }
        else
        {

            for(std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end();)
            {
                ODSocketClient* client = *it;
                if((mSockSelector.isReady(client->mSockClient)) &&
                    (!notifyClientMessage(client)))
                {
                    // The server wants to remove the client
                    it = mSockClients.erase(it);
                    mSockSelector.remove(client->mSockClient);
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

ODSocketClient::ODComStatus ODSocketServer::receiveMsgFromClient(ODSocketClient* client, ODPacket& packetReceived)
{
    return client->recv(packetReceived);
}

ODSocketClient::ODComStatus ODSocketServer::sendMsgToClient(ODSocketClient* client, ODPacket& packetReceived)
{
    return client->send(packetReceived);
}

void ODSocketServer::stopServer()
{
    mIsConnected = false;
    if(mThread != nullptr)
        delete mThread; // Delete waits for the thread to finish
    mThread = nullptr;
    mSockSelector.clear();
    mSockListener.close();
    for (std::vector<ODSocketClient*>::iterator it = mSockClients.begin(); it != mSockClients.end(); ++it)
    {
        ODSocketClient* client = *it;
        client->disconnect();
        delete client;
    }

    mSockClients.clear();
    if(mNewClient != nullptr)
    {
        delete mNewClient;
        mNewClient = nullptr;
    }
}

void ODSocketServer::setClientState(ODSocketClient* client, const std::string& state)
{
    client->setState(state);
}
