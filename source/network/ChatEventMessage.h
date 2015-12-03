/*!
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

#ifndef CHATEVENTMESSAGE_H
#define CHATEVENTMESSAGE_H

#include <SFML/System.hpp>
#include <string>

#include "network/ODServer.h"

class Seat;

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
    ChatMessage(const std::string& playerNick, const std::string& message, Seat* seat);

    inline const std::string& getMessage() const
    { return mMessage; }

    //! \brief Computes and returns the preformatted message string.
    std::string getMessageAsString() const;

private:
    std::string mMessage;
    std::string mPlayerNick;
    Seat* mSeat;
};

class EventMessage
{
public:
    EventMessage(const std::string& message, EventShortNoticeType type = EventShortNoticeType::genericGameInfo);

    inline const std::string& getMessage() const
    { return mMessage; }

    bool isMessageTooOld(float maxTimeDisplay) const;

    //! \brief Computes and returns the preformatted message string.
    std::string getMessageAsString();

private:
    std::string mMessage;
    EventShortNoticeType mType;
    sf::Clock   mClockCreation;
};

#endif // CHATEVENTMESSAGE_H
