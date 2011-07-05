/*!
 * \file   Console.cpp
 * \date:  04 July 2011
 * \author StefanP.MUC
 * \brief  Ingame console
 */

#include "ODApplication.h"

#include "Console.h"

template<> Console* Ogre::Singleton<Console>::ms_Singleton = 0;

Console::Console() :
        visible(false),
        startLine(0),
        height(1),
        rect(new Ogre::Rectangle2D(true)),
        //these two define how much text goes into the console
        consoleLineLength(85),
        consoleLineCount(15)
{
    ODApplication::getSingleton().getRoot()->addFrameListener(this);

    // Create background rectangle covering the whole screen
    rect->setCorners(-1, 1, 1, 1 - height);
    rect->setMaterial("console/background");
    rect->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY);
    rect->setBoundingBox(
            Ogre::AxisAlignedBox(-100000.0 * Ogre::Vector3::UNIT_SCALE,
                    100000.0 * Ogre::Vector3::UNIT_SCALE));
    node = ODApplication::getSingleton().getRoot()
            ->getSceneManagerIterator().getNext()->getRootSceneNode()
                    ->createChildSceneNode("#Console");
    node->attachObject(rect);

    textbox = Ogre::OverlayManager::getSingleton().createOverlayElement(
            "TextArea", "ConsoleText");
    //textbox->setDimensions(1, 0.1);
    textbox->setPosition(0, 0);
    textbox->setParameter("font_name", "FreeMono");
    textbox->setParameter("char_height", "16");
    textbox->setColour(Ogre::ColourValue(1,1,1,1));
    textbox->setCaption("hello");

    overlay = Ogre::OverlayManager::getSingleton().create("Console");
    overlay->add2D(static_cast<Ogre::OverlayContainer*>(textbox));
    overlay->show();
    //LogManager::getSingleton().getDefaultLog()->addListener(this);
}

Console::~Console()
{
    delete rect;
    delete node;
    delete textbox;
    delete overlay;
}

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
        {
            char legalchars[] =
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+!\"#%&/()=?[]\\*-_.:,; ";
            for (int c = 0; c < sizeof(legalchars) - 1; ++c)
            {
                if (legalchars[c] == arg.text)
                {
                    prompt += arg.text;
                    break;
                }
            }
            break;
        }
    }

    updateOverlay = true;
}
bool Console::frameStarted(const Ogre::FrameEvent &evt)
{
    if (visible && height < 1)
    {
        height += evt.timeSinceLastFrame * 2;
        textbox->show();
        if (height >= 1)
        {
            height = 1;
        }
    }
    else if (!visible && height > 0)
    {
        height -= evt.timeSinceLastFrame * 2;
        if (height <= 0)
        {
            height = 0;
            textbox->hide();
        }
    }

    textbox->setPosition(0, (height - 1) * 0.5);
    rect->setCorners(-1, 1 + height, 1, 1 - height);

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

void Console::print(const Ogre::String &text)
{
    //subdivide it into lines
    const char *str = text.c_str();
    int start = 0, count = 0;
    int len = text.length();
    Ogre::String line;
    for (int c = 0; c < len; ++c)
    {
        if (str[c] == '\n' || line.length() >= consoleLineLength)
        {
            lines.push_back(line);
            line = "";
        }

        if (str[c] != '\n')
        {
            line += str[c];
        }
    }

    lines.push_back(line);

    startLine = (lines.size() > consoleLineCount)
            ? lines.size() - consoleLineCount
            : 0;

    updateOverlay = true;
}

bool Console::frameEnded(const Ogre::FrameEvent &evt)
{
    return true;
}

void Console::addCommand(const Ogre::String &command,
        void(*func)(std::vector<Ogre::String>&))
{
    commands[command] = func;
}

void Console::removeCommand(const Ogre::String &command)
{
    commands.erase(commands.find(command));
}
