#include "ChatMessage.h"

ChatMessage::ChatMessage()
{
    message = "UNSET_MESSAGE";
    clientNick = "UNSET_CLIENT_NICK";
    sendTime = recvTime = time(NULL);
}

ChatMessage::ChatMessage(std::string nNick, std::string nMessage,
        time_t nRecvTime, time_t nSendTime)
{
    message = nMessage;
    clientNick = nNick;
    sendTime = nSendTime;
    recvTime = nRecvTime;
}

