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

#ifndef ODSOCKETCLIENT_H
#define ODSOCKETCLIENT_H

#include "ODPacket.h"

#include <SFML/Network.hpp>

#include <string>

class ODSocketClient
{
    friend class ODSocketServer;

    public:
        enum ODComStatus
        {
            OK, NotReady, Error
        };

        ODSocketClient(bool isBlocking = true);
        ~ODSocketClient();

        // Client initialization
        virtual bool connect(const std::string& host, const int port);
        virtual void disconnect();
        bool isConnected();

    protected:
        // Data Transimission
        /*! \brief Sends a packet through the network
         * ODPacket should preserve integrity. That means that if an ODSocketClient
         * sends an ODPacket, the server should receive exactly 1 similar ODPacket (same data,
         * nothing less, nothing more). It is up to ODSocketClient to do so.
         */
        ODComStatus send(ODPacket& s);
        /*! \brief Receives a packet through the network
         * ODPacket should preserve integrity. That means that if an ODSocketClient
         * sends an ODPacket, the server should receive exactly 1 similar ODPacket (same data,
         * nothing less, nothing more). It is up to ODSocketClient to do so.
         */
        ODComStatus recv(ODPacket& s);

    private :
        sf::TcpSocket mSockClient;
        bool mIsConnected;
};

#endif // ODSOCKETCLIENT_H

