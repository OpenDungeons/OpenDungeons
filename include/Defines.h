#ifndef DEFINES_H
#define DEFINES_H

#define BLENDER_UNITS_PER_OGRE_UNIT       10.0
#define VERSION       "0.3.7"
#define PORT_NUMBER		 31222		// Both of these lines should be the same number
#define PORT_NUMBER_STRING	"31222"		// Both of these lines should be the same number
#define DEFAULT_FRAMES_PER_SECOND	30.0

#define HELP_MESSAGE	"\
The console is a way of interacting with the underlying game engine directly.  \
Commands given to the the console are made up of two parts: a \'command name\' and one or more \'arguments\'.  \
For information on how to use a particular command, type help followed by the command name.\
\n\nThe following commands are avaliable:\
\n\tlist - print out lists of creatures, classes, etc\n\thelp - displays this help screen\n\tsave - saves the current level to a file\
\n\tload - loads a level from a file\
\n\tquit - exit the program\
\n\ttermwidth - set the terminal width\
\n\taddcreature - load a creature into the file.\
\n\taddclass - Define a creature class\
\n\taddtiles - adds a rectangular region of tiles\
\n\tnewmap - Creates a new rectangular map\
\n\tmovespeed - sets the camera movement speed\
\n\trotatespeed - sets the camera rotation speed\
\n\tfps - sets the maximum framerate\
\n\tturnspersecond - sets the number of turns the AI will carry out per second\
\n\tambientlight - set the ambient light color\
\n\tconnect - connect to a server\
\n\thost - host a server\
\n\tchat - send a message to other people in the game\
\n\tnearclip - sets the near clipping distance\
\n\tfarclip - sets the far clipping distance\
\n\tvisdebug - turns on visual debugging for a creature\
\n\taddcolor - adds another player color\
\n\tsetcolor - changes the value of one of the player's color\
\n\tdisconnect - stops a running server or client and returns to the map editor"

#endif

