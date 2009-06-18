// Implementation of the Socket class.


#include "string.h"
#include <iostream>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "Socket.h"
#include "Defines.h"

Socket::Socket() :
  m_sock ( -1 )
{
	sem_init(&semaphore, 0, 1);
	memset ( &m_addr, 0, sizeof ( m_addr ) );
}

Socket::~Socket()
{
	if ( is_valid() )
#if defined(WIN32) || defined(_WIN32)
		closesocket ( m_sock );
#else
		::close ( m_sock );
#endif

}

bool Socket::create()
{
	m_sock = socket ( AF_INET, SOCK_STREAM, 0 );

	if ( ! is_valid() )
		return false;

	// TIME_WAIT - argh
	int on = 1;
	if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 )
		return false;

	return true;
}

bool Socket::bind ( const int port )
{

	if ( ! is_valid() )
	{
		return false;
	}

	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port = htons ( port );

	int bind_return = ::bind ( m_sock, ( struct sockaddr * ) &m_addr, sizeof ( m_addr ) );

	if ( bind_return == -1 )
	{
		return false;
	}

	return true;
}

bool Socket::listen() const
{
	if ( ! is_valid() )
	{
		return false;
	}

	int listen_return = ::listen ( m_sock, MAXCONNECTIONS );

	if ( listen_return == -1 )
	{
		return false;
	}

	return true;
}

bool Socket::accept ( Socket& new_socket ) const
{
	int addr_length = sizeof ( m_addr );

#if defined(WIN32) || defined(_WIN32)
	new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( int * ) &addr_length );
#else
	new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );
#endif

	if ( new_socket.m_sock <= 0 )
		return false;
	else
		return true;
}

bool Socket::send ( const std::string s ) const
{
	int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
	if ( status == -1 )
	{
		return false;
	}
	else
	{
		return true;
	}
}

int Socket::recv ( std::string& s ) const
{
	char buf [ MAXRECV + 1 ];

	s = "";

	memset ( buf, 0, MAXRECV + 1 );

	int status = ::recv ( m_sock, buf, MAXRECV, 0 );

	if ( status == -1 )
	{
		std::cout << "status == -1   errno == " << errno << "  in Socket::recv\n";
		return 0;
	}
	else if ( status == 0 )
	{
		return 0;
	}
	else
	{
		s = buf;
		return status;
	}
}

bool Socket::connect ( const std::string host, const int port )
{
	if ( ! is_valid() )
		return false;

	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons ( port );

	int status;

#ifdef WIN32
	/*
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int status = getaddrinfo(host.c_str(), PORT_NUMBER_STRING, &hints, &result);

	if ( status != 0 )
	{
		cout << "getaddrinfo failed with error: " << status << endl;
		WSACleanup();
		exit(1);
	}

	m_addr.sin_addr = ((sockaddr_in*)(result->ai_addr))->sin_addr;
	*/

	struct sockaddr_storage ss;
	int sslen = sizeof(ss);
	WSAStringToAddress((CHAR*)host.c_str(), AF_INET, NULL, (struct sockaddr*)&ss, &sslen);
	m_addr.sin_addr = ((struct sockaddr_in *)&ss)->sin_addr;
#else
	status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

	if ( errno == EAFNOSUPPORT )
		return false;
#endif

	status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

	if ( status == 0 )
		return true;
	else
		return false;
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

