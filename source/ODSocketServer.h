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

#ifndef ODSOCKETSERVER_H
#define ODSOCKETSERVER_H

#include "ODSocketClient.h"

#include <SFML/Network.hpp>

#include <string>

class ODPacket;

class ODSocketServer
{
    public:
        ODSocketServer();
        ~ODSocketServer();

        bool isConnected();

        // Data Transimission
        virtual bool createServer(int listeningPort);
        virtual void stopServer();

    protected:
        /*! \brief Function called when a new client connects. If it returns true,
         * the new client will be keeped in the list. If false, the client will be discarded.
         * As this function is called from the doTask context, it shall return as soon as
         * possible (no communication with the client should be done).
         */
        virtual bool notifyNewConnection(ODSocketClient *sock) = 0;
        /*! \brief Function called when a client sends a message. As this function is called
         * from the doTask context, it shall return as soon as possible (we should not send
         * a message and wait actively for its answer). The proper way of communicating should be :
         * 1 - read the message
         * 2 - If needed, send something (answer or new question)
         * 3 - Save somewhere if we are waiting for something from the client
         * 4 - Return from the function. When new data will be available, notifyClientMessage
         *     will be called again
         * If the function returns false, the client will be removed from the list and properly deleted
         */
        virtual bool notifyClientMessage(ODSocketClient *sock) = 0;

        /*! \brief Main function task. Checks if a new client connects. If so, notifyNewConnection
         * will be called with the client socket. If it returns true, the client is saved in the
         * client list. If not, the client is discarded. doTask also checks if a connected client sent
         * a message. If so, calls notifyClientMessage with the client socket.
         * If timeoutMs = 0, this function will never return. Otherwise, it will always return after
         * timeoutMs milliseconds, even if new clients connected or clients are sending messages.
         */
        void doTask(int timeoutMs);
        ODSocketClient::ODComStatus receiveMsgFromClient(ODSocketClient* client, ODPacket& packetReceived);
        ODSocketClient::ODComStatus sendMsgToClient(ODSocketClient* client, ODPacket& packetReceived);
        void sendMsgToAllClients(ODPacket& packetReceived);
        std::vector<ODSocketClient*> mSockClients;
        void setClientState(ODSocketClient* client, const std::string& state);
        virtual void serverThread() = 0;

    private:
        sf::Thread* mThread;
        sf::TcpListener mSockListener;
        sf::SocketSelector mSockSelector;
        sf::Clock mClockMainTask;
        ODSocketClient* mNewClient;
        bool mIsConnected;
};

#endif // ODSOCKETSERVER_H

