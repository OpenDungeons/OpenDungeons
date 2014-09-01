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

/* TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */

#include "Console.h"
#include "ODApplication.h"
#include "ODFrameListener.h"

//TODO: make rest of commands scriptable
/*! \brief Process the commandline from the terminal and carry out the actions
 *  specified in by the user.
 */

/*! \brief A helper function to return a help text string for a given terminal command.
 *
 */
string Console::getHelpText(std::string arg)
{
    std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

    if (arg.compare("save") == 0)
    {
        return "Save the current level to a file.  The file name is given as an argument to the save command, if no file name is given the last file loaded is overwritten by the save command.\n\nExample:\n"
                + mPrompt
                + "save Test\n\nThe above command will save the level to levels/Test.level.  The Test level is loaded automatically when OpenDungeons starts.";
    }

    else if (arg.compare("load") == 0)
    {
        return "Load a level from a file.  The new level replaces the current level.  The levels are stored in the levels directory and have a .level extension on the end.  Both the directory and the .level extension are automatically applied for you, if no file name is given the last file loaded is reloaded.\n\nExample:\n"
                + mPrompt
                + "load Level1\n\nThe above command will load the file Level1.level from the levels directory.";
    }

    else if (arg.compare("addclass") == 0)
    {
        return "Add a new class decription to the current map.  Because it is common to load many creatures of the same type creatures are given a class which stores their common information such as the mesh to load, scaling, etc.  Addclass defines a new class of creature, allowing creatures of this class to be loaded in the future.  The argument to addclass is interpreted in the same was as a class description line in the .level file format.\n\nExample:\n"
                + mPrompt
                + "addclass Skeleton Skeleton.mesh 0.01 0.01 0.01\n\nThe above command defines the class \"Skeleton\" which uses the mesh file \"Skeleton.mesh\" and has a scale factor of 0.01 in the X, Y, and Z dimensions.";
    }

    else if (arg.compare("addcreature") == 0)
    {
        return "Add a new creature to the current map.  The creature class to be used must be loaded first, either from the loaded map file or by using the addclass command.  Once a class has been declared a creature can be loaded using that class.  The argument to the addcreature command is interpreted in the same way as a creature line in a .level file.\n\nExample:\n"
                + mPrompt
                + "addcreature Skeleton Bob 10 15 0\n\nThe above command adds a creature of class \"Skeleton\" whose name is \"Bob\" at location X=10, y=15, and Z=0.  The new creature's name must be unique to the creatures in that level.  Alternatively the name can be se to \"autoname\" to have OpenDungeons assign a unique name.";
    }

    else if (arg.compare("quit") == 0)
    {
        return "Exits OpenDungeons";
    }

    else if (arg.compare("termwidth") == 0)
    {
        return "The termwidth program sets the maximum number of characters that can be displayed on the terminal without word wrapping taking place.  When run with no arguments, termwidth displays a ruler across the top of you terminal indicating the terminal's current width.  When run with an argument, termwidth sets the terminal width to a new value specified in the argument.\n\nExample:\n"
                + mPrompt
                + "termwidth 80\n\nThe above command sets the terminal width to 80.";
    }

    else if (arg.compare("addtiles") == 0)
    {
        return "The addtiles command adds a rectangular region of tiles to the map.  The tiles are initialized to a fullness of 100 and have their type set to dirt.  The region to be added is given as two pairs of X-Y coordinates.\n\nExample:\n"
                + mPrompt
                + "addtiles -10 -5 34 20\n\nThe above command adds the tiles in the given region to the map.  Tiles which overlap already existing tiles will be ignored.";
    }

    else if (arg.compare("newmap") == 0)
    {
        return "Replaces the existing map with a new rectangular map.  The X and Y dimensions of the new map are given as arguments to the newmap command.\n\nExample:\n"
                + mPrompt
                + "newmap 10 20\n\nThe above command creates a new map 10 tiles by 20 tiles.  The new map will be filled with dirt tiles with a fullness of 100.";
    }

    else if (arg.compare("refreshmesh") == 0)
    {
        return "Clears every mesh in the entire game (creatures, tiles, etc) and then reloads them so they have the new look in the material files, etc.";
    }

    else if (arg.compare("movespeed") == 0)
    {
        return "The movespeed command sets how fast the camera moves at.  When run with no argument movespeed simply prints out the current camera move speed.  With an argument movespeed sets the camera move speed.\n\nExample:\n"
                + mPrompt
                + "movespeed 3.7\n\nThe above command sets the camera move speed to 3.7.";
    }

    else if (arg.compare("rotatespeed") == 0)
    {
        return "The rotatespeed command sets how fast the camera rotates.  When run with no argument rotatespeed simply prints out the current camera rotation speed.  With an argument rotatespeed sets the camera rotation speed.\n\nExample:\n"
                + mPrompt
                + "rotatespeed 35\n\nThe above command sets the camera rotation speed to 35.";
    }

    else if (arg.compare("ambientlight") == 0)
    {
        return "The ambientlight command sets the minumum light that every object in the scene is illuminated with.  It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n"
                + mPrompt
                + "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.";
    }

    else if (arg.compare("host") == 0)
    {
        std::stringstream s;
        s << ODApplication::PORT_NUMBER;
        return "Starts a server thread running on this machine.  This utility takes a port number as an argument.  The port number is the port to listen on for a connection.  The default (if no argument is given) is to use" + s.str() + "for the port number.";
    }

    else if (arg.compare("connnect") == 0)
    {
        return "Connect establishes a connection with a server.  It takes as its argument an IP address specified in dotted decimal notation (such as 192.168.1.100), and starts a client thread which monitors the connection for events.";
    }

    else if (arg.compare("chat") == 0 || arg.compare("c") == 0)
    {
        return "Chat (or \"c\" for short) is a utility to send messages to other players participating in the same game.  The argument to chat is broadcast to all members of the game, along with the nick of the person who sent the chat message.  When a chat message is recieved it is added to the chat buffer along with a timestamp indicating when it was recieved.\n\nThe chat buffer displays the last n chat messages recieved.  The number of displayed messages can be set with the \"chathist\" command.  Displayed chat messages will also be removed from the chat buffer after they age beyond a certain point.";
    }

    else if (arg.compare("addgold") == 0)
    {
        return "addgold adds the given amount of gold to one player. It takes as arguments the color of the player to whom the gold should be given and the amount. If the player's treasuries are full, no more gold is given. Note that this command is available in server mode only. Example to give 5000 gold to player color 1 : addgold 1 5000";
    }

    else if (arg.compare("list") == 0 || arg.compare("ls") == 0)
    {
        return "List (or \"ls\" for short is a utility which lists various types of information about the current game.  Running list without an argument will produce a list of the lists available.  Running list with an argument displays the contents of that list.\n\nExample:\n"
                + mPrompt
                + "list creatures\n\nThe above command will produce a list of all the creatures currently in the game.";
    }

    else if (arg.compare("visdebug") == 0)
    {
        return "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n"
                + mPrompt
                + "visdebug skeletor\n\nThe above command wil turn on visual debugging for the creature named \'skeletor\'.  The same command will turn it back off again.";
    }

    else if (arg.compare("turnspersecond") == 0 || arg.compare("tps") == 0)
    {
        return "turnspersecond (or \"tps\" for short is a utility which displays or sets the speed at which the game is running.\n\nExample:\n"
                + mPrompt
                + "tps 5\n\nThe above command will set the current game speed to 5 turns per second.";
    }

    else if (arg.compare("mousespeed") == 0)
    {
        return "Mousespeed sets the mouse movement speed scaling factor. It takes a decimal number as argument, which the mouse movement will get multiplied by.";
    }

    else if (arg.compare("framespersecond") == 0 || arg.compare("fps") == 0)
    {
        return "framespersecond (or \"fps\" for short is a utility which displays or sets the maximum framerate at which the rendering will attempt to update the screen.\n\nExample:\n"
                + mPrompt
                + "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.";
    }

    else if (arg.compare("keys") == 0)
    {
        return "|| Action           || Keyboard 1       || Keyboard 2       ||\n\
                ==============================================================\n\
                || Zoom In          || Page Up          || e                ||\n\
                || Zoom Out         || Insert           || q                ||\n\
                || Pan Left         || Left             || a                ||\n\
                || Pan Right        || Right            || d                ||\n\
                || Pan Forward      || Up               || w                ||\n\
                || Pan Backward     || Down             || s                ||\n\
                || Pan Backward     || Down             || s                ||\n\
                || Tilt Up          || Home             || N/A              ||\n\
                || Tilt Down        || End              || N/A              ||\n\
                || Rotate Left      || Delete           || N/A              ||\n\
                || Rotate right     || Page Down        || N/A              ||\n\
                || Toggle Console   || `                || F12              ||\n\
                || Quit Game        || ESC              || N/A              ||\n\
                || Take screenshot  || Printscreen      || N/A              ||\n\
                || Toggle Framerate || f                || N/A              ||";
    }
    else if (arg.compare("aithreads") == 0)
    {
        return "Sets the maximum number of threads the gameMap will attempt to spawn during the f() method.  The set value must be greater than or equal to 1.";
    }
    else
    {
        return "Help for command:  \"" + ODFrameListener::getSingletonPtr()->mArguments + "\" not found.";
    }
}
