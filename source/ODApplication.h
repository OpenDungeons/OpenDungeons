/*! \file   ODApplication.cpp
 *  \author Ogre team, andrewbuck, oln, StefanP.MUC
 *  \date   07 April 2011
 *  \brief  Class ODApplication containing everything to start the game
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

#ifndef ODAPPLICATION_H
#define ODAPPLICATION_H

#include <string>

namespace Ogre {
  class Root;
  class RenderWindow;
  class OverlaySystem;
}

class GameMap;
class ODFrameListener;

//! \brief Base class which manages the startup of OpenDungeons.
class ODApplication
{
public:
    //! Initializes the Application along with the ResourceManager
    ODApplication();
    
    ~ODApplication();

    //! \brief Display a GUI error message
    static void displayErrorMessage(const std::string& message, bool log = true);

    static const unsigned int PORT_NUMBER;
    static const double DEFAULT_FRAMES_PER_SECOND;
    static double MAX_FRAMES_PER_SECOND;
    static double turnsPerSecond;
    static const std::string VERSION;
    static const std::string VERSIONSTRING;
    static const std::string POINTER_INFO_STRING;
    static std::string MOTD;
    static const std::string HELP_MESSAGE;

private:
    ODApplication(const ODApplication&);

    Ogre::Root* mRoot;
    Ogre::RenderWindow* mWindow;

    ODFrameListener* mFrameListener;
    //! \brief by both the RenderManager and the TextRenderer.
    Ogre::OverlaySystem* mOverlaySystem;

    //! \brief Delete the various singleton objects and clean up other stuff
    void cleanUp();
};

#endif // ODAPPLICATION_H
