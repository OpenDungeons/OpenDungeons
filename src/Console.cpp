/*!
 * \file   Console.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 */

/* TODO: decide and implement command handling (functors?)
 * TODO: decide and adjust the layout and prompt (size, position, color)
 * TODO: do intense testing that everything works
 * TODO: switch from TextRenderer to Console
 */

#include "ODApplication.h"

#include "Console.h"

template<> Console* Ogre::Singleton<Console>::ms_Singleton = 0;

Console::Console() :
        visible(false),
        updateOverlay(true),
        startLine(0),
        //these two define how much text goes into the console
        consoleLineLength(85),
        consoleLineCount(15)
{
    ODApplication::getSingleton().getRoot()->addFrameListener(this);

    Ogre::OverlayManager& olMgr = Ogre::OverlayManager::getSingleton();

    // Create a panel
    panel = static_cast<Ogre::OverlayContainer*>(
        olMgr.createOverlayElement("Panel", "ConsolePanel"));
    panel->setPosition(0, 0);
    panel->setDimensions(1, 0.5);
    panel->setMaterialName("console/background");

    // Create a text area
    textbox = olMgr.createOverlayElement("TextArea", "ConsoleText");
    textbox->setMetricsMode(Ogre::GMM_PIXELS);
    textbox->setPosition(0, 0);
    textbox->setDimensions(1, 1);
    textbox->setParameter("font_name", "FreeMono");
    textbox->setParameter("char_height", "14");

    // Create an overlay, and add the panel
    overlay = olMgr.create("Console");
    overlay->add2D(panel);

    // Add the text area to the panel
    panel->addChild(textbox);

   // Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
    delete panel;
    delete textbox;
    delete overlay;
}

/*! \brief handles the key input
 *
 */
void Console::onKeyPressed(const OIS::KeyEvent &arg)
{
    if (!visible)
    {
        return;
    }

    switch(arg.key)
    {
        case OIS::KC_RETURN:
        {
            //TODO: convert this to STL string functions
            //split the parameter list
            const char *str = prompt.c_str();
            std::vector<Ogre::String> params;
            Ogre::String param = "";

            for (int c = 0; c < prompt.length(); ++c)
            {
                if (str[c] == ' ')
                {
                    if (param.length())
                    {
                        params.push_back(param);
                    }

                    param = "";
                }
                else
                {
                    param += str[c];
                }
            }
            if (param.length())
            {
                params.push_back(param);
            }

            //try to execute the command
            for (std::map<Ogre::String, void(*)(std::vector<Ogre::String>&)>::iterator i = commands.begin();
                    i != commands.end(); ++i)
            {
                if ((*i).first == params[0])
                {
                    if ((*i).second)
                    {
                        (*i).second(params);
                    }
                    break;
                }
            }

            print(prompt);
            prompt = "";
            break;
        }

        case OIS::KC_BACK:
            prompt = prompt.substr(0, prompt.length() - 1);
            break;

        case OIS::KC_PGUP:
            if (startLine > 0)
            {
                --startLine;
            }
        break;

        case OIS::KC_PGDOWN:
            if (startLine < lines.size())
            {
                ++startLine;
            }
            break;

        default:
            if (std::string("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ,.<>/?1234567890-=\\!@#$%^&*()_+|;\':\"[]{}").find(
                    arg.text) != std::string::npos)
            {
                prompt += arg.text;
            }
            break;
    }

    updateOverlay = true;
}

/*! \brief Defines the action on starting the current frame
 *
 *  The Console listener checks if it needs updating and if it does it will
 *  redraw itself with the new text
 */
bool Console::frameStarted(const Ogre::FrameEvent &evt)
{
    if(updateOverlay)
    {
        Ogre::String text;
        std::list<Ogre::String>::iterator i, start, end;

        //make sure is in range
        if (startLine > lines.size())
        {
            startLine = lines.size();
        }

        int lcount = 0;
        start = lines.begin();
        for (int c = 0; c < startLine; ++c)
        {
            ++start;
        }

        end = start;
        for (int c = 0; c < consoleLineCount; ++c)
        {
            if (end == lines.end())
            {
                break;
            }
            ++end;
        }

        for (i = start; i != end; ++i)
        {
            text += (*i) + "\n";
        }

        //add the prompt
        text += "] " + prompt;

        textbox->setCaption(text);
        updateOverlay = false;
    }
    return true;
}

/*! \brief print text to the console
 *
 * This function automatically checks if there are linebreaks in the text
 * and separates the text into separate strings
 */
void Console::print(const Ogre::String &text)
{
    size_t lastBreak = 0;
    size_t pos = text.find('\n');
    do
    {
        lines.push_back(text.substr(lastBreak, pos));
        lastBreak = pos + 1; //+1: next time start AFTER the last line break
    }while(pos != std::string::npos);

    startLine = (lines.size() > consoleLineCount)
            ? lines.size() - consoleLineCount
            : 0;

    updateOverlay = true;
}

/*! \brief what happens after frame
 *
 */
bool Console::frameEnded(const Ogre::FrameEvent &evt)
{
    return true;
}

/*! \brief add a command
 *
 */
void Console::addCommand(const Ogre::String &command,
        void(*func)(std::vector<Ogre::String>&))
{
    commands[command] = func;
}

/*! \brief remove a command
 *
 */
void Console::removeCommand(const Ogre::String &command)
{
    commands.erase(commands.find(command));
}

/*! \brief show or hide the console manually
 *
 */
void Console::setVisible(const bool& newState)
{
    visible = newState;
    Gui::getSingleton().setVisible(!visible);
    checkVisibility();
}

/*! \brief enables or disables the console, depending on what state it has
 *
 */
void Console::toggleVisibility()
{
    visible = !visible;
    Gui::getSingleton().setVisible(!visible);
    checkVisibility();
}

/*! \brief Does the actual showing/hiding depending on bool visible
 *
 */
void Console::checkVisibility()
{
    if(visible)
    {
        overlay->show();
    }
    else
    {
        overlay->hide();
    }
}
