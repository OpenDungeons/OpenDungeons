#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <stdio.h>
#include <string>
using namespace std;

class ChatMessage
{
	public:
		ChatMessage();
		ChatMessage(string nNick, string nMessage, time_t nRecvTime, time_t nSendTime = 0);
		string message, clientNick;
		time_t sendTime, recvTime;
};

#endif

