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

#ifndef ODSOCKETCLIENT_H
#define ODSOCKETCLIENT_H

#include <network/ODPacket.h>

#include <SFML/Network.hpp>

#include <string>
#include <cstdint>
#include <fstream>
#include <istream>

class Player;

class ODSocketClient
{
    friend class ODSocketServer;

    public:
        enum ODComStatus
        {
            OK, NotReady, Error
        };

        enum ODSource
        {
            none,
            network,
            file
        };

        ODSocketClient():
            mSource(ODSource::none),
            mPlayer(nullptr),
            mLastTurnAck(-1),
            mPendingTimestamp(-1)
        {}

        virtual ~ODSocketClient()
        {}

        // Client initialization
        bool isConnected();

        //! \brief Disconnect the client and tell whether to keep the replay file.
        virtual void disconnect(bool keepReplay = false);

        Player* getPlayer() { return mPlayer; }
        void setPlayer(Player* player) { mPlayer = player; }
        int64_t getLastTurnAck() { return mLastTurnAck; }
        void setLastTurnAck(int64_t lastTurnAck) { mLastTurnAck = lastTurnAck; }
        const std::string& getState() {return mState;}
        bool isDataAvailable();
        int32_t getGameTimeMillis()
        { return mGameClock.getElapsedTime().asMilliseconds(); }

    protected:
        virtual bool connect(const std::string& host, const int port);
        virtual bool replay(const std::string& filename);
        inline ODSource getSource() const
        { return mSource; }

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
        void setState(const std::string& state) {mState = state;}

        ODSource mSource;
        sf::SocketSelector mSockSelector;
        sf::TcpSocket mSockClient;
        Player* mPlayer;
        int64_t mLastTurnAck;
        std::string mState;

        sf::Clock mGameClock;
        std::ifstream mReplayInputStream;
        std::ofstream mReplayOutputStream;
        ODPacket mPendingPacket;
        int32_t mPendingTimestamp;

        //! \brief the replay filename being written. Used to later optionally delete it
        //! if asked to.
        std::string mOutputReplayFilename;
};

#endif // ODSOCKETCLIENT_H

