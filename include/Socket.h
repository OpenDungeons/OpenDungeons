// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

//TODO:  This is a hack to make the windows version compile, it may not work properly at runtime
#define MSG_WAITALL 0
#define MSG_NOSIGNAL 0

#else

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

// This is a workaround to make mingw define 'getaddrinfo()'
#ifdef __MINGW32__
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif


#include <sys/types.h>
#include <string>
#include <semaphore.h>
#include <iostream>
using namespace std;

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 16;
const int MAXRECV = 2048;

class Socket
{
	public:
		Socket();
		virtual ~Socket();

		// Server initialization
		bool create();
		bool bind ( const int port );
		bool listen() const;
		bool accept ( Socket& ) const;

		// Client initialization
		bool connect ( const std::string host, const int port );

		// Data Transimission
		bool send ( const std::string ) const;
		int recv ( std::string& ) const;

		//void set_non_blocking ( const bool );

		bool is_valid() const { return m_sock != -1; }
		sem_t semaphore;

	private:
		int m_sock;
		sockaddr_in m_addr;

};

#endif

