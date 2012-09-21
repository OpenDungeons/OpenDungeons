#include <iostream>

#include "ODApplication.h"
#include "Socket.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
int main(int argc, char **argv)
#endif
{
#ifdef WIN32
    // Set up windows sockets
    WSADATA wsaData;

    if(WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        cerr << "Couldn't not find a usable WinSock DLL.n";
        exit(1);
    }
#endif
    try
    {
        new ODApplication;
    }
    catch (const std::exception& e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
        MessageBox(0, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
		std::cerr<< "An exception has occurred: " << e.what() << std::endl;
#endif
    }

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}
