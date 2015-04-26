/*! \file   main.cpp
 *  \author paul424
 *  \date   Sun Jun 22 18:16:35 CEST 2014
 *  \brief  file containing the main function
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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
#include <cstring>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_GNU
#define __USE_GNU
#endif

#include "ODApplication.h"

#include <OgrePlatform.h>
#include <OgreException.h>

#if !defined (WIN32) && (defined (__i386__) | defined (__x86_64__))  // Only for supported platforms
#include "utils/StackTraceUnix.h"
#include <iostream>
#endif //WIN32

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if defined __MINGW32__
#include "utils/StackTraceWinMinGW.h"
#endif // defined __MINGW32__

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
int main(int argc, char** argv)
#endif
{
#if !defined (WIN32) && (defined (__i386__) | defined (__x86_64__))
//Init the error hanlder used to get a full stacktrace when crashing
    struct sigaction sigact;
    std::memset(&sigact, 0, sizeof(sigact));
    sigact.sa_sigaction = StackTracePrint::critErrHandler;
    sigact.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGSEGV, &sigact, nullptr) != 0)
    {
        std::cerr << "error setting signal handler for: "
            << SIGSEGV << strsignal(SIGSEGV) << std::endl;
        exit(EXIT_FAILURE);
    }
#elif defined __MINGW32__
    // To log segfaults
    StackTraceWinMinGW trace("crash.log");
#endif //!WIN32
    try
    {
        ODApplication od;
    }
    catch (Ogre::Exception& e)
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBox(0, e.what(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        std::cerr << "An exception has occurred: " << e.what();
#endif
    }

    return 0;
}
