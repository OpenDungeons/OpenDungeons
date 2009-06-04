#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <stdio.h>
#include <string>
using namespace std;

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
		ChatMessage(string nNick, string nMessage, time_t nRecvTime, time_t nSendTime = 0);
		string message, clientNick;
		time_t sendTime, recvTime;
};

#endif

