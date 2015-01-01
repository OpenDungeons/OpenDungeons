/*!
 * \file   console.as
 * \date:  16 August 2011
 * \author StefanP.MUC
 * \brief  AS script to handle the console commands
 */

/*! \brief Evaluate the comand and its params and call the according function
 *
 *  \param com The command to be executed
 *  \param args This array conatins all arguments to the function
 *
 *  Complex operations should be put in separate functions. Easy tasks like
 *  getting or setting variables or printing a string can be stored directly
 *  in here.
 */
void executeConsoleCommand(string &in com, string[] &in args)
{
    if(com.length() == 0)
    {
        console.print("ERROR: Tried to execute an empty command");
        return;
    }

    const uint argCount = args.length();

    if(com == "maxtime")
    {
        if(argCount == 0)
        {
            console.print("Max display time for chat messages is: " + frameListener.get_ChatMaxTimeDisplay());
        }
        else
        {
            if(checkArgCount(argCount, 1))
            {
                if(checkIfInt(args[0]))
                {
                    frameListener.ChatMaxTimeDisplay = args[0];
                    console.print("Max display time for chat messages was changed to: " + frameListener.get_ChatMaxTimeDisplay());
                }
                else
                {
                    console.print("ERROR: Expected an integer greater or equal to 0 in first argument.");
                }
            }
        }
    }
    else if(com == "maxmessages")
    {
        if(argCount == 0)
        {
            console.print("Max chat messages to display is: " + frameListener.ChatMaxMessages);
        }
        else
        {
            if(checkArgCount(argCount, 1))
            {
                if(checkIfInt(args[0]))
                {
                    frameListener.ChatMaxMessages = args[0];
                    console.print("Max chat messages to display has been set to: " + frameListener.ChatMaxMessages);
                }
                else
                {
                    console.print("ERROR: Expected an integer greater or equal to 0 in first argument.");
                }
            }
        }
    }
    else
    {
        console.print("ERROR: Command not found. Try help to get an overview.");
    }
}

bool checkArgCount(uint &in actualCount, uint &in expectedCount)
{
    if(actualCount != expectedCount)
    {
        console.print("ERROR: Wrong number of arguments (" + actualCount + "), expected " + expectedCount);
        return false;
    }

    return true;
}
