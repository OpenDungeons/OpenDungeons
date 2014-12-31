/*!
 * \file   Console_getHelp.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
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

#include "modes/Console.h"
#include "ODApplication.h"
#include "render/ODFrameListener.h"
#include "utils/ConfigManager.h"

/*! \brief A helper function to return a help text string for a given terminal command.
 *
 */
string Console::getHelpText(std::string arg)
{
    std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

    if (arg.compare("addcreature") == 0)
    {
        return "Add a new creature to the current map.  The creature class to be used must be loaded first, either from the loaded map file or by using the addclass command.  Once a class has been declared a creature can be loaded using that class.  The argument to the addcreature command is interpreted in the same way as a creature line in a .level file.\n\nExample:\n"
                + mPrompt
                + "addcreature Skeleton Bob 10 15 0\n\nThe above command adds a creature of class \"Skeleton\" whose name is \"Bob\" at location X=10, y=15, and Z=0.  The new creature's name must be unique to the creatures in that level.  Alternatively the name can be se to \"autoname\" to have OpenDungeons assign a unique name.";
    }
    else if (arg.compare("ambientlight") == 0)
    {
        return "The 'ambientlight' command sets the minumum light that every object in the scene is illuminated with.  It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n"
                + mPrompt
                + "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.";
    }
    else if (arg.compare("fps") == 0)
    {
        return "'fps' (framespersecond) for short is a utility which displays or sets the maximum framerate at which the rendering will attempt to update the screen.\n\nExample:\n"
                + mPrompt
                + "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.";
    }
    else if (arg.compare("nearclip") == 0)
    {
        return "Sets the minimal viewpoint clipping distance. Objects nearer than that won't be rendered.\n\nE.g.: nearclip 3.0";
    }
    else if (arg.compare("farclip") == 0)
    {
        return "Sets the maximal viewpoint clipping distance. Objects farther than that won't be rendered.\n\nE.g.: farclip 30.0";
    }
    else if (arg.compare("list") == 0 || arg.compare("ls") == 0)
    {
        return "'list' (or 'ls' for short) is a utility which lists various types of information about the current game. "
               "Running list without an argument will produce a list of the lists available. "
               "Running list with an argument displays the contents of that list.\n\nExample:\n"
               + mPrompt
               + "list creatures\n\nThe above command will produce a list of all the creatures currently in the game.";
    }
    else if (arg.compare("connnect") == 0)
    {
        return "'connect' establishes a connection with a server.  It takes as its argument an IP address specified in dotted decimal notation (such as 192.168.1.100), "
               "and starts a client thread which monitors the connection for events.";
    }
    else if (arg.compare("chat") == 0)
    {
        return "'chat' is a utility to send messages to other players participating in the same game.  The argument to chat is broadcast to all members of the game, along with the nick of the person who sent the chat message.  When a chat message is recieved it is added to the chat buffer along with a timestamp indicating when it was recieved.\n\nThe chat buffer displays the last n chat messages recieved.  The number of displayed messages can be set with the \"chathist\" command.  Displayed chat messages will also be removed from the chat buffer after they age beyond a certain point.";
    }
    else if (arg.compare("creaturevisdebug") == 0)
    {
        return "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n"
                + mPrompt
                + "creaturevisdebug skeletor\n\nThe above command wil turn on visual debugging for the creature named \'skeletor\'.  The same command will turn it back off again.";
    }
    else if (arg.compare("seatvisdebug") == 0)
    {
        return "Visual debugging is a way to see all the tiles a given seat can see.\n\nExample:\n"
                + mPrompt
                + "seatvisdebug 1\n\nThe above command will show every tiles seat 1 can see.  The same command will turn it off.";
    }
    else if (arg.compare("icanseedeadpeople") == 0)
    {
        return "Toggles on/off fog of war for every connected player";
    }
    else if (arg.compare("setcreaturelevel") == 0)
    {
        return "Sets the level of a given creature.\n\nExample:\n"
                + mPrompt
                + "setlevel BigKnight1 10\n\nThe above command will set the creature \'BigKnight1\' to 10.";
    }
    else if (arg.compare("setcreaturedest") == 0)
    {
        return "setcreaturedest Sets the creature destination. The path will be computed if reachable. It takes as arguments the creature name, then the tile coordinales. Example : setdest Wizard1 10 10";
    }
    else if (arg.compare("addgold") == 0)
    {
        return "'addgold' adds the given amount of gold to one player. It takes as arguments the color of the player to whom the gold should be given and the amount. If the player's treasuries are full, no more gold is given. Note that this command is available in server mode only. Example to give 5000 gold to player color 1 : addgold 1 5000";
    }
    else if (arg.compare("logfloodfill") == 0)
    {
        return "'logfloodfill' logs the FloodFillValues of all the Tiles in the GameMap.";
    }
    else if (arg.compare("listmeshanims") == 0)
    {
        return "'listmeshanims' lists all the animations for the given mesh.";
    }
    else if (arg.compare("triggercompositor") == 0)
    {
        return "Starts the given compositor. The compositor must exist.\n\nExample:\n"
               + mPrompt
               + "triggercompositor blacknwhite";
    }
    else if (arg.compare("disconnect") == 0)
    {
        return "Disconnects from a server.";
    }
    else if (arg.compare("pause") == 0)
    {
        return "Pauses/unpauses a game. The game must act as a server to succeed.";
    }
    else if (arg.compare("maxmessages") == 0)
    {
        return "Sets the max number of lines displayed at once in the info text area.\n\nExample:\n"
               + mPrompt
               + "maxmessages 4";
    }
    else if (arg.compare("maxtime") == 0)
    {
        return "Sets the max time (in seconds) a message will be displayed in the info text area.\n\nExample:\n"
               + mPrompt
               + "maxtime 5";
    }
    else if (arg.compare("catmullspline") == 0)
    {
        return "Triggers the catmullspline camera movement behaviour.\n\nExample:\n"
               + mPrompt
               + "catmullspline 6 4 5 4 6 5 7\n"
               + "Make the camera follow a lazy curved path along the given coordinates pairs. "
               + "The first parameter is the number of pairs";
    }
    else if (arg.compare("circlearound") == 0)
    {
        return "Triggers the circle camera movement behaviour.\n\nExample:\n"
               + mPrompt
               + "circlearound 6 4 8\n"
               + "Make the camera follow a lazy a circle path around coors 6,4 with a radius of 8.";
    }
    else if (arg.compare("setcamerafovy") == 0)
    {
        return "Sets the camera vertical field of view aspect ratio on the Y axis.\n\nExample:\n"
               + mPrompt
               + "setcamerafovy 45";
    }
    else if (arg.compare("quit") == 0 || arg.compare("exit") == 0)
    {
        return "Exits OpenDungeons";
    }
    else if (arg.compare("keys") == 0)
    {
        return "|| Action               || US Keyboard layout ||     Mouse      ||\n\
                ==============================================================\n\
                || Pan Left             || Left   -   A       || -              ||\n\
                || Pan Right            || Right  -   D       || -              ||\n\
                || Pan Forward          || Up     -   W       || -              ||\n\
                || Pan Backward         || Down   -   S       || -              ||\n\
                || Rotate Left          || Q                  || -              ||\n\
                || Rotate right         || E                  || -              ||\n\
                || Zoom In              || End                || Wheel Up       ||\n\
                || Zoom Out             || Home               || Wheel Down     ||\n\
                || Tilt Up              || Page Up            || -              ||\n\
                || Tilt Down            || End                || -              ||\n\
                || Select Tile/Creature || -                  || Left Click     ||\n\
                || Drop Creature/Gold   || -                  || Right Click    ||\n\
                || Toggle Debug Info    || F11                || -              ||\n\
                || Toggle Console       || F12                || -              ||\n\
                || Quit Game            || ESC                || N/A            ||\n\
                || Take screenshot      || Printscreen        || N/A            ||";
    }
    else
    {
        return "Help for command:  \"" + ODFrameListener::getSingletonPtr()->mArguments + "\" not found.";
    }
}
