/*! \file   main.cpp
 *  \author paul424
 *  \date   Sun Jun 22 18:16:35 CEST 2014
 *  \brief  file containing the main function
 *
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


#include <iostream>
#include <string.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include "ODApplication.h"
#include "Socket.h"
#include "StackTracePrint.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


using std::cerr, std::endl;
using namespace StackTracePrint;

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

    struct sigaction sigact;
    sigact.sa_sigaction = StackTracePrint::crit_err_hdlr;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0)
    {
    cerr << "error setting signal handler for " << SIGSEGV << strsignal(SIGSEGV) <<endl;
        exit(EXIT_FAILURE);
    }
    try
    {
        new ODApplication;
    }
    catch (Ogre::Exception& e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32 
        MessageBox(0, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        fprintf(stderr, "An exception has occurred: %s\n", e.what());
#endif

    }

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
