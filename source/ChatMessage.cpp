#include "ChatMessage.h"

ChatMessage::ChatMessage() :
        message     ("UNSET_MESSAGE"),
        clientNick  ("UNSET_CLIENT_NICK"),
        sendTime    (time(0)),
        recvTime    (time(0))
{
}

ChatMessage::ChatMessage(const std::string& nNick, const std::string& nMessage,
        const time_t& nRecvTime, time_t nSendTime) :
            message     (nMessage),
            clientNick  (nNick),
            sendTime    (nSendTime),
            recvTime    (nRecvTime)
{
}
