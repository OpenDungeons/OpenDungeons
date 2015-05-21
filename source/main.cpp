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

#include "utils/ResourceManager.h"
#include "utils/StackTracePrint.h"
#include "ODApplication.h"

#include <OgrePlatform.h>
#include <OgreException.h>

#include <boost/program_options.hpp>

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
int main(int argc, char** argv)
#endif
{
    // To log segfaults
    StackTracePrint trace("crash.log");

    try
    {
        boost::program_options::options_description desc("Allowed options");
        // By default, we only add help option. The rest will be added by ConfigManager
        // to make sure it is done at the same location
        desc.add_options()
            ("help", "produce help message")
        ;
        ResourceManager::buildCommandOptions(desc);

        boost::program_options::variables_map options;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        std::vector<std::string> listArgs = boost::program_options::split_winmain(strCmdLine);
        boost::program_options::store(boost::program_options::command_line_parser(listArgs).options(desc).run(), options);
#else
        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(desc).run(), options);
#endif
        boost::program_options::notify(options);

        if (options.count("help"))
        {
            std::stringstream ss;
            ss << "OpenDungeons version: " << ODApplication::VERSION << "\n";
            ss << desc;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox(0, ss.str().c_str(), "Help", MB_OK | MB_TASKMODAL);
#else
            std::cout << ss.str() << "\n";
#endif
            return 0;
        }

        ODApplication od;
        od.startGame(options);
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
