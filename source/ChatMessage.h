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
        ChatMessage(const std::string& nNick, const std::string& nMessage, const time_t& nRecvTime,
                time_t nSendTime = 0);

        inline const std::string&   getMessage      () const { return message; }
        inline const std::string&   getClientNick   () const { return clientNick; }
        inline const time_t&        getRecvTime     () const { return recvTime; }

    private:
        std::string message;
        std::string clientNick;
        time_t      sendTime;
        time_t      recvTime;
};

#endif

