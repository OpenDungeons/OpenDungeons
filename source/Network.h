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

#ifndef NETWORK_H
#define NETWORK_H

#include <deque>
#include <string>
#include <utility>

class ODFrameListener;
class Socket;

#include "ChatMessage.h"

// SERVER

/*! \brief A thread function which runs on the server and listens for new connections from clients.
 *
 * A single instance of this thread is spawned by running the "host" command
 * from the in-game console.  The thread then binds annd listens on the
 * specified port and when clients connect a new socket, and a
 * clientHandlerThread are spawned to handle communications with that client.
 * This function currently has no way of breaking out of its primary loop, so
 * once it is started it never exits until the program is closed.
 */
void processServerSocketMessages();

/*! \brief Monitors the serverNotificationQueue for new events and informs the clients about them.
 *
 * This function is used in server mode and acts as a "consumer" on the
 * serverNotificationQueue.  It takes an event out of the queue, determines
 * which clients need to be informed about that particular event, and
 * dispacthes TCP packets to inform the clients about the new information.
 */
void processServerNotifications();

/*! \brief The function running in server-mode which listens for messages from an individual, already connected, client.
 *
 * This function recieves TCP packets one at a time from a connected client,
 * decodes them, and carries out requests for the client, returning any
 * results.
 * \returns false When the client has disconnected.
 */
bool processClientNotifications(Socket* clientSocket);


// CLIENT

/*! \brief A function which runs on the client to handle communications with the server.
 *
 * A single instance of this thread is spawned by the client when it connects
 * to a server.
 */
void processClientSocketMessages();

/*! \brief The function which monitors the clientNotificationQueue for new events and informs the server about them.
 *
 * This runs on the client side and acts as a "consumer" on the
 * clientNotificationQueue.  It takes an event out of the queue, determines
 * which clients need to be informed about that particular event, and
 * dispacthes TCP packets to inform the clients about the new information.
 */
void processClientNotifications();

// Other functions  (these are defined in src/Server.cpp)
std::string formatCommand(std::string command, std::string arguments);
bool parseCommand(std::string &command, std::string &commandName, std::string &arguments);
ChatMessage *processChatMessage(std::string arguments);
void sendToAllClients(ODFrameListener *frameListener, std::string str);

#endif // NETWORK_H
