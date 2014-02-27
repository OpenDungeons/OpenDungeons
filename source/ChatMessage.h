/*!
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

#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <string>
#include <ctime>

/*! \brief A data structure to store a chat message and its relevant time stamps.
 *
 * The chat message data structure is filled up and placed in a queue when a
 * chat message is received from the server.  When the server itself receives
 * a chat message from a client it places a chat message structure in its own
 * queue like a client would and additionally forwards it to any other clients
 * who should receive it.
 */
class ChatMessage
{
public:
    ChatMessage();
    ChatMessage(const std::string& nNick, const std::string& nMessage,
                const time_t& nRecvTime, time_t nSendTime = 0);

    inline const std::string& getMessage() const
    { return mMessage; }

    inline const std::string& getClientNick() const
    { return mClientNick; }

    inline const time_t& getRecvTime() const
    { return mRecvTime; }

private:
    std::string mMessage;
    std::string mClientNick;
    time_t      mSendTime;
    time_t      mRecvTime;
};

#endif // CHATMESSAGE_H
