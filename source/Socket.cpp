// Implementation of the Socket class.

#include "Socket.h"

#include <iostream>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

Socket* Socket::serverSocket = 0;
Socket* Socket::clientSocket = 0;

Socket::Socket() :
    m_sock(-1)
{
    sem_init(&semaphore, 0, 1);
    memset(&m_addr, 0, sizeof(m_addr));
}

Socket::~Socket()
{
    if (is_valid())
#if defined(WIN32) || defined(_WIN32)
        closesocket ( m_sock );
#else
        ::close(m_sock);
#endif

}

bool Socket::create()
{
    m_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (!is_valid())
        return false;

    // TIME_WAIT - argh
    int on = 1;
    return (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) != -1);
}

bool Socket::bind(const int port)
{
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons(port);

    return (is_valid() && ::bind(m_sock, (struct sockaddr *) &m_addr, sizeof(m_addr)) != -1);
}

bool Socket::listen() const
{
    return (is_valid() && ::listen(m_sock, MAXCONNECTIONS) != -1);
}

bool Socket::accept(Socket& new_socket) const
{
    int addr_length = sizeof(m_addr);

#if defined(WIN32) || defined(_WIN32)
    new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( int * ) &addr_length );
#else
    new_socket.m_sock = ::accept(m_sock, (sockaddr *) &m_addr, (socklen_t *) &addr_length);
#endif

    return (new_socket.m_sock > 0);
}

bool Socket::send(const std::string& s) const
{
    return (::send(m_sock, s.c_str(), s.size(), MSG_NOSIGNAL) != -1);
}

int Socket::recv(std::string& s) const
{
    char buf[MAXRECV + 1];
    s = "";
    memset(buf, 0, MAXRECV + 1);
    int status = ::recv(m_sock, buf, MAXRECV, 0);

    switch(status)
    {
        case -1:
            std::cerr << "\n\nERROR:  status == -1   errno == " << errno << "  in Socket::recv\n";
            return 0;

        case 0:
            return 0;

        default:
            s = buf;
            return status;
    }
}

bool Socket::connect(const std::string& host, const int port)
{
    if (!is_valid())
        return false;

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);

    int status;

#ifdef WIN32
    struct sockaddr_storage ss;
    int sslen = sizeof(ss);
    WSAStringToAddress((CHAR*)host.c_str(), AF_INET, NULL, (struct sockaddr*)&ss, &sslen);
    m_addr.sin_addr = ((struct sockaddr_in *)&ss)->sin_addr;
#else
    status = inet_pton(AF_INET, host.c_str(), &m_addr.sin_addr);

    if (errno == EAFNOSUPPORT)
        return false;
#endif

    status = ::connect(m_sock, (sockaddr *) &m_addr, sizeof(m_addr));
    return (status == 0);
}

/*
 void Socket::set_non_blocking ( const bool b )
 {
 int opts;

 opts = fcntl ( m_sock, F_GETFL );

 if ( opts < 0 )
 {
 return;
 }

 if ( b )
 opts = ( opts | O_NONBLOCK );
 else
 opts = ( opts & ~O_NONBLOCK );

 fcntl ( m_sock, F_SETFL,opts );
 }
 */

