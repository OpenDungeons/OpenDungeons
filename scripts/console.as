/*!
 * \file   console.as
 * \date:  16 August 2011
 * \author StefanP.MUC
 * \brief  AS script to handle the console commands
 */

/*! \brief Evaluate the comand and its params and call the according function
 *
 *  \param commands This string array contains on [0] the command and from
 *                  [1] to [length-1] the optional arguments for the command
 */
void executeConsoleCommand(string[] commands)
{
    //TODO: convert c++ code to as code and put it here

    if(commands.length() == 0)
    {
        console.print("ERROR: Tried to execute an empty command");
        return;
    }

    string com = commands[0];
    const uint argCount = commands.length() - 1;

    if(com == "exit" || com == "quit")
    {
        //TODO: code
    }

    else if(com == "help" || com == "?")
    {
        //TODO: code
    }

    else if(com == "echo")
    {
        //TODO: code
    }

    else if(com == "save")
    {
        //TODO: code
    }

    else if(com == "load")
    {
        //TODO: code
    }

    else if(com == "ambientlight")
    {
        //TODO: code
    }

    else if(com == "termwidth")
    {
        //TODO: code
    }

    else if(com == "addtiles")
    {
        //TODO: code
    }

    else if(com == "movespeed")
    {
        //TODO: code
    }

    else if(com == "rotatespeed")
    {
        //TODO: code
    }

    else if(com == "fps")
    {
        //TODO: code
    }

    else if(com == "aithreads")
    {
        //TODO: code
    }

    else if(com == "turnspersecond")
    {
        //TODO: code
    }

    else if(com == "nearclip")
    {
        //TODO: code
    }

    else if(com == "farclip")
    {
        //TODO: code
    }

    else if(com == "addcreature")
    {
        //TODO: code
    }

    else if(com == "addclass")
    {
        //TODO: code
    }

    else if(com == "list" || com == "ls")
    {
        //TODO: code
    }

    else if(com == "newmap")
    {
        //TODO: code
    }

    else if(com == "refreshmesh")
    {
        //TODO: code
    }

    else if(com == "nick")
    {
        //TODO: code
    }

    else if(com == "maxtime")
    {
        //TODO: code
    }

    else if(com == "maxmessages")
    {
        //TODO: code
    }

    else if(com == "connect")
    {
        //TODO: code
    }

    else if(com == "host")
    {
        //TODO: code
    }

    else if(com == "chathelp")
    {
        //TODO: code
    }

    else if(com == "chat")
    {
        //TODO: code
    }

    else if(com == "visdebug")
    {
        //TODO: code
    }

    else if(com == "disconnect")
    {
        //TODO: code
    }

    else if(com == "next")
    {
        //TODO: code
    }
}
