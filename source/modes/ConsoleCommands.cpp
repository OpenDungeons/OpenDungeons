#include "modes/ConsoleCommands.h"

#include "entities/Creature.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "goals/Goal.h"
#include "modes/ConsoleInterface.h"
#include "network/ClientNotification.h"
#include "network/ODClient.h"
#include "network/ODServer.h"
#include "render/ODFrameListener.h"
#include "render/RenderManager.h"
#include "rooms/Room.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <OgreSceneManager.h>

#include <boost/algorithm/string/join.hpp>

#include <functional>

namespace
{

const std::string HELPMESSAGE =
        "The console is a way of interacting with the underlying game engine directly."
        "Commands given to the the are made up of two parts: a \'command name\' and one or more \'arguments\'."
        "For information on how to use a particular command, type help followed by the command name."
        "\n\nThe following commands are available:"
        "\n\n==General=="
        "\n\thelp - Displays this help message."
        "\n\thelp keys - Shows the keyboard controls."
        "\n\tlist/ls - Prints out lists of creatures, classes, etc..."
        "\n\tmaxtime - Sets or displays the max time for event messages to be displayed."
        "\n\ttermwidth - Sets the terminal width."
        "\n\n==Cheats=="
        "\n\taddcreature - Adds a creature."
        "\n\tsetcreaturelevel - Sets the level of a given creature."
        "\n\taddgold - Gives gold to one player."
        "\n\ticanseedeadpeople - Toggles on/off fog of war for every connected player."
        "\n\n==Developer\'s options=="
        "\n\tfps - Sets the maximum framerate cap."
        "\n\tambientlight - Sets the ambient light color."
        "\n\tnearclip - Sets the near clipping distance."
        "\n\tfarclip - Sets the far clipping distance."
        "\n\tcreaturevisdebug - Turns on visual debugging for a given creature."
        "\n\tseatvisdebug - Turns on visual debugging for a given seat."
        "\n\tsetcreaturedest - Sets the creature destination/"
        "\n\tlistmeshanims - Lists all the animations for the given mesh."
        "\n\ttriggercompositor - Starts the given Ogre Compositor."
        "\n\tcatmullspline - Triggers the catmullspline camera movement type."
        "\n\tcirclearound - Triggers the circle camera movement type."
        "\n\tsetcamerafovy - Sets the camera vertical field of view aspect ratio value."
        "\n\tlogfloodfill - Displays the FloodFillValues of all the Tiles in the GameMap.";

//! \brief Template function to get/set a variable from the ODFrameListener object
template<typename ValType, typename Getter, typename Setter>
Command::Result cSetFrameListenerVar(Getter getter,
                        Setter setter,
                        ODFrameListener& fl,
                        const std::string& name,
                        const Command::ArgumentList_t& args, ConsoleInterface& c)
{
    if(args.size() < 2)
    {
        c.print("Value " + name + " is: " + Helper::toString(getter(fl)));
    }
    else
    {
        ValType v = Helper::stringToT<ValType>(args[1]);
        setter(fl, v);
        c.print("Value " + name + " set to " + Helper::toString(v));
    }
    return Command::Result::SUCCESS;
}

Command::Result cSendCmdToServer(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    // If we should send a console command, we do it
    if(!ODClient::getSingleton().isConnected())
    {
        c.print("Trying to send a console command while not connected");
        return Command::Result::FAILED;
    }

    if(args.empty())
    {
        c.print("Trying to send an empty console command");
        return Command::Result::INVALID_ARGUMENT;
    }

    ClientNotification *clientNotification = new ClientNotification(
        ClientNotificationType::askExecuteConsoleCommand);
    uint32_t nbArgs = args.size();
    clientNotification->mPacket << nbArgs;
    for(const std::string& str : args)
        clientNotification->mPacket << str;

    ODClient::getSingleton().queueClientNotification(clientNotification);
    return Command::Result::SUCCESS;
}

Command::Result cAmbientLight(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    Ogre::SceneManager* mSceneMgr = RenderManager::getSingletonPtr()->getSceneManager();

    if (args.size() < 2)
    {
        // Display the current ambient light values.
        Ogre::ColourValue curLight = mSceneMgr->getAmbientLight();
        c.print("Current ambient light is:" + Helper::toString(curLight));
        return Command::Result::SUCCESS;
    }
    else if(args.size() >= 4)
    {
        // Set the new color.
        Ogre::ColourValue v(Helper::toFloat(args[1]), Helper::toFloat(args[2]), Helper::toFloat(args[3]));

        mSceneMgr->setAmbientLight(v);
        c.print("\nAmbient light set to:\n" +
                Helper::toString(v));
    }
    else
    {
        c.print("To set a colour value, 3 colour values are needed");
        return Command::Result::INVALID_ARGUMENT;
    }
    return Command::Result::SUCCESS;
}

Command::Result cFPS(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    if(args.size() < 2)
    {
        c.print("\nCurrent maximum framerate is "
                + Helper::toString(ODFrameListener::getSingleton().getMaxFPS())
                + "\n");
    }
    else if(args.size() >= 2)
    {
        int fps = Helper::toInt(args[1]);
        ODFrameListener::getSingleton().setMaxFPS(fps);
        c.print("\nMaximum framerate set to: " + Helper::toString(fps));
    }
    return Command::Result::SUCCESS;
}

Command::Result cSrvAddCreature(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if (args.size() < 6)
    {
        c.print("Invalid number of arguments\n");
        return Command::Result::INVALID_ARGUMENT;
    }

    // We remove the first argument (the console command)
    Command::ArgumentList_t tmp = args;
    tmp.erase(tmp.begin());
    std::string str = boost::algorithm::join(tmp, "\t");
    std::stringstream ss(str);
    Creature* creature = Creature::getCreatureFromStream(&gameMap, ss);
    creature->addToGameMap();
    //Set up definition for creature. This was previously done in createMesh for some reason.
    creature->setupDefinition(gameMap, *ConfigManager::getSingleton().getCreatureDefinitionDefaultWorker());
    //Set position to update info on what tile the creature is in.
    creature->setPosition(creature->getPosition());

    return Command::Result::SUCCESS;
}

Command::Result cList(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager& mm)
{
    ODFrameListener* frameListener = ODFrameListener::getSingletonPtr();
    GameMap* gameMap = frameListener->getClientGameMap();

    if (args.size() < 2)
    {
        c.print("lists available:\n\t\tclasses\tcreatures\tplayers\n\t\tnetwork\trooms\tcolors\n\t\tgoals\n");
        return Command::Result::SUCCESS;
    }

    std::stringstream stringStr;

    if (args[1].compare("creatures") == 0)
    {
        stringStr << "Class:\tCreature name:\tLocation:\tColor:\tLHand:\tRHand\n\n";
        for (Creature* creature : gameMap->getCreatures())
        {
            GameEntity::exportToStream(creature, stringStr);
            stringStr << std::endl;
        }
    }
    else if (args[1].compare("classes") == 0)
    {
        stringStr << "Class:\tMesh:\tScale:\tHP:\tMana:\tSightRadius:\tDigRate:\tMovespeed:\n\n";
        for (unsigned int i = 0; i < gameMap->numClassDescriptions(); ++i)
        {
            const CreatureDefinition* currentClassDesc = gameMap->getClassDescription(i);
            stringStr << currentClassDesc << "\n";
        }
    }
    else if (args[1].compare("players") == 0)
    {
        stringStr << "Local player:\tNick:\tSeatId\tTeamId:\n\n"
                  << "me\t\t" << gameMap->getLocalPlayer()->getNick() << "\t"
                  << gameMap->getLocalPlayer()->getSeat()->getId() << "\t"
                  << gameMap->getLocalPlayer()->getSeat()->getTeamId() << "\n\n";

        stringStr << "Player:\tNick:\tSeatId\tTeamId:\n\n";

        for (Player* player : gameMap->getPlayers())
        {
            stringStr << player->getId() << "\t\t" << player->getNick() << "\t"
                    << player->getSeat()->getId() << "\t"
                    << player->getSeat()->getTeamId() << "\n\n";
        }
    }
    else if (args[1].compare("network") == 0)
    {
        if (mm.getCurrentModeType() == AbstractModeManager::EDITOR)
        {
            stringStr << "You are currently in the map editor.";
        }
        else if (ODServer::getSingleton().isConnected())
        {
            stringStr << "You are currently acting as a server.";
        }
        else
        {
            stringStr << "You are currently connected to a server.";
        }
    }
    else if (args[1].compare("rooms") == 0)
    {
        stringStr << "Name:\tSeat Id:\tNum tiles:\n\n";
        for (Room* room : gameMap->getRooms())
        {
            stringStr << room->getName() << "\t" << room->getSeat()->getId()
                    << "\t" << room->numCoveredTiles() << "\n";
        }
    }
    else if (args[1].compare("colors") == 0 || args[1].compare("colours") == 0)
    {
        stringStr << "Number:\tRed:\tGreen:\tBlue:\n";
        const std::vector<Seat*> seats = gameMap->getSeats();
        for (Seat* seat : seats)
        {
            Ogre::ColourValue color = seat->getColorValue();

            stringStr << "\n" << seat->getId() << "\t\t" << color.r
                    << "\t\t" << color.g << "\t\t" << color.b;
        }
    }
    else if (args[1].compare("goals") == 0)
    {
        // Loop over the list of unmet goals for the seat we are sitting in an print them.
        stringStr << "Unfinished Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
        for (unsigned int i = 0; i < gameMap->getLocalPlayer()->getSeat()->numUncompleteGoals(); ++i)
        {
            Seat* s = gameMap->getLocalPlayer()->getSeat();
            Goal* tempGoal = s->getUncompleteGoal(i);
            stringStr << tempGoal->getName() << ":\t"
                    << tempGoal->getDescription(*s) << "\n";
        }

        // Loop over the list of completed goals for the seat we are sitting in an print them.
        stringStr << "\n\nCompleted Goals:\nGoal Name:\tDescription\n----------\t-----------\n";
        for (unsigned int i = 0; i < gameMap->getLocalPlayer()->getSeat()->numCompletedGoals(); ++i)
        {
            Seat* seat = gameMap->getLocalPlayer()->getSeat();
            Goal* tempGoal = seat->getCompletedGoal(i);
            stringStr << tempGoal->getName() << ":\t"
                    << tempGoal->getSuccessMessage(*seat) << "\n";
        }
    }
    else
    {
        stringStr << "ERROR:  Unrecognized list.  Type \"list\" with no arguments to see available lists.";
    }

    c.print("+\n" + stringStr.str() + "\n");
    return Command::Result::SUCCESS;
}

Command::Result cSrvCreatureVisDebug(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if(args.size() < 2)
        return Command::Result::INVALID_ARGUMENT;

    const std::string& name = args[1];
    gameMap.consoleToggleCreatureVisualDebug(name);

    return Command::Result::SUCCESS;
}

Command::Result cSrvSeatVisDebug(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if(args.size() < 2)
        return Command::Result::INVALID_ARGUMENT;

    int seatId = Helper::toInt(args[1]);
    gameMap.consoleToggleSeatVisualDebug(seatId);
    return Command::Result::SUCCESS;
}

Command::Result cSrvToggleFOW(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    gameMap.consoleAskToggleFOW();
    return Command::Result::SUCCESS;
}

Command::Result cSrvSetCreatureLevel(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if(args.size() < 3)
        return Command::Result::INVALID_ARGUMENT;

    const std::string& name = args[1];
    uint32_t level = Helper::toUInt32(args[2]);
    gameMap.consoleSetLevelCreature(name, level);
    return Command::Result::SUCCESS;
}

Command::Result cCircleAround(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    if(args.size() < 4)
    {
        c.print("\nERROR:  You need to specify an circle center "
                          "(two coordinates) and circle radius\n");
        return Command::Result::INVALID_ARGUMENT;
    }

    ODFrameListener& frameListener = ODFrameListener::getSingleton();
    CameraManager* cm = frameListener.getCameraManager();
    int centerX = Helper::toDouble(args[1]);
    int centerY = Helper::toDouble(args[2]);
    unsigned int radius = Helper::toDouble(args[3]);


    cm->circleAround(centerX, centerY, radius);
    return Command::Result::SUCCESS;
}

Command::Result cHermiteCatmullSpline(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    //TODO: only works on larger maps with coordinates > 10
    if(args.size() < 5)
    {
        c.print("\nERROR:  You need to specify at least two coordinate's pair.\n");
        return Command::Result::INVALID_ARGUMENT;
    }


    CameraManager* cm = ODFrameListener::getSingleton().getCameraManager();
    std::size_t numPairs = (args.size() - 1) / 2;

    cm->resetHCSNodes(numPairs);
    for(std::size_t i = 1; i < args.size() - 1; i +=2)
    {
        int a = Helper::toInt(args[i]);
        int b = Helper::toInt(args[i + 1]);
        //TODO: Why are the points specified as integers?
        cm->addHCSNodes(a, b);
        c.print("Adding nodes: " + args[i] + ", " + args[i + 1]);
    }

    cm->setCatmullSplineMode(true);
    return Command::Result::SUCCESS;
}

Command::Result cSrvAddGold(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if(args.size() < 3)
        return Command::Result::INVALID_ARGUMENT;

    int seatId = Helper::toInt(args[1]);
    int gold = Helper::toInt(args[2]);
    gameMap.addGoldToSeat(gold, seatId);
    return Command::Result::SUCCESS;
}

Command::Result cSrvSetCreatureDest(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    if(args.size() < 3)
        return Command::Result::INVALID_ARGUMENT;

    const std::string& name = args[0];
    int x = Helper::toInt(args[1]);
    int y = Helper::toInt(args[2]);
    gameMap.consoleSetCreatureDestination(name, x, y);
    return Command::Result::SUCCESS;
}

Command::Result cSrvLogFloodFill(const Command::ArgumentList_t&, ConsoleInterface& c, GameMap& gameMap)
{
    gameMap.logFloodFileTiles();
    return Command::Result::SUCCESS;
}

Command::Result cSetCameraFOVy(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    Ogre::Camera* cam = ODFrameListener::getSingleton().getCameraManager()->getActiveCamera();
    if(args.size() < 2)
    {
        c.print("Camera FOVy :" + Helper::toString(cam->getFOVy().valueDegrees()));
    }
    else
    {
        cam->setFOVy(Ogre::Degree(static_cast<Ogre::Real>(Helper::toFloat(args[1]))));
    }
    return Command::Result::SUCCESS;
}

Command::Result cListMeshAnims(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    if(args.size() < 2)
    {
        c.print("\nERROR : Need to specify name of mesh");
        return Command::Result::INVALID_ARGUMENT;
    }
    std::string anims = RenderManager::consoleListAnimationsForMesh(args[1]);
    c.print("\nAnimations for " + args[1] + ": " + anims);
    return Command::Result::SUCCESS;
}

Command::Result cSrvUnlockSkills(const Command::ArgumentList_t& args, ConsoleInterface& c, GameMap& gameMap)
{
    gameMap.consoleAskUnlockSkills();
    return Command::Result::SUCCESS;
}

Command::Result cKeys(const Command::ArgumentList_t&, ConsoleInterface& c, AbstractModeManager&)
{
    c.print("|| Action               || US Keyboard layout ||     Mouse      ||\n\
             ==================================================================\n\
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
             || Change view mode     || V                  || -              ||\n\
             || Select Tile/Creature || -                  || Left Click     ||\n\
             || Drop Creature/Gold   || -                  || Right Click    ||\n\
             || Toggle Debug Info    || F11                || -              ||\n\
             || Toggle Console       || F12                || -              ||\n\
             || Quit Game            || ESC                || -              ||\n\
             || Take screenshot      || Printscreen        || -              ||\n");
    return Command::Result::SUCCESS;
}

Command::Result cTriggerCompositor(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    if(args.size() < 2)
    {
        c.print("ERROR: Needs name of compositor");
        return Command::Result::INVALID_ARGUMENT;
    }
    RenderManager::getSingleton().triggerCompositor(args[1]);
    return Command::Result::SUCCESS;
}

Command::Result cStartTileCulling(const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&)
{
    ODFrameListener::getSingleton().getCameraManager()->startTileCulling();
    return Command::Result::SUCCESS;
}

} // namespace <none>

namespace ConsoleCommands
{
void addConsoleCommands(ConsoleInterface& cl)
{
    cl.addCommand("ambientlight",
                   "The 'ambientlight' command sets the minimum light that every object in the scene is illuminated with. "
                   "It takes as it's argument and RGB triplet whose values for red, green, and blue range from 0.0 to 1.0.\n\nExample:\n"
                   "ambientlight 0.4 0.6 0.5\n\nThe above command sets the ambient light color to red=0.4, green=0.6, and blue = 0.5.",
                   cAmbientLight,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("fps",
                  "'fps' (framespersecond) for short is a utility which displays or sets the maximum framerate at which the"
                  "rendering will attempt to update the screen.\n\nExample:\n"
                  "fps 35\n\nThe above command will set the current maximum framerate to 35 turns per second.",
                  cFPS,
                  Command::cStubServer,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("nearclip",
                   "Sets the minimal viewpoint clipping distance. Objects nearer than that won't be rendered.\n\nE.g.: nearclip 3.0",
                   [](const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&) {
                           return cSetFrameListenerVar<float>(std::mem_fn(&ODFrameListener::getActiveCameraNearClipDistance),
                                                                 std::mem_fn(&ODFrameListener::setActiveCameraNearClipDistance),
                                                                 ODFrameListener::getSingleton(),
                                                                 "near clip distance", args, c);
                    },
                  Command::cStubServer,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("farclip",
                  "Sets the maximal viewpoint clipping distance. Objects farther than that won't be rendered.\n\nE.g.: farclip 30.0",
                  [](const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&) {
                          return cSetFrameListenerVar<float>(std::mem_fn(&ODFrameListener::getActiveCameraFarClipDistance),
                                                                std::mem_fn(&ODFrameListener::setActiveCameraFarClipDistance),
                                                                ODFrameListener::getSingleton(),
                                                                "far clip distance", args, c);
                  },
                  Command::cStubServer,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("maxtime",
                  "Sets the max time (in seconds) a message will be displayed in the info text area.\n\nExample:\n"
                  "maxtime 5",
                  [](const Command::ArgumentList_t& args, ConsoleInterface& c, AbstractModeManager&) {
                          return cSetFrameListenerVar<float>(std::mem_fn(&ODFrameListener::getEventMaxTimeDisplay),
                                                                std::mem_fn(&ODFrameListener::setEventMaxTimeDisplay),
                                                                ODFrameListener::getSingleton(),
                                                                "event max time display", args, c);
                  },
                  Command::cStubServer,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("addcreature",
                  "Adds a new creature according to following parameters",
                  cSendCmdToServer,
                  cSrvAddCreature,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});

    cl.addCommand("startTileCulling",
                  "start culling of the tiles",
                  cStartTileCulling,
                  {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});

    std::string listDescription = "'list' (or 'ls' for short) is a utility which lists various types of information about the current game. "
            "Running list without an argument will produce a list of the lists available. "
            "Running list with an argument displays the contents of that list.\n\nExamples:\n"
            "list creatures\tLists all the creatures currently in the game.\n"
            "list classes\tLists all creature classes.\n"
            "list players\tLists every player in game.\n"
            "list network\tTells whether the game is running as a server, a client or as the map editor.\n"
            "list rooms\tLists all the current rooms in game.\n"
            "list colors\tLists all seat's color values.\n"
            "list goals\tLists The local player goals.\n";
    cl.addCommand("list",
                   listDescription,
                   cList,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"ls"});
    cl.addCommand("creaturevisualdebug",
                   "Visual debugging is a way to see a given creature\'s AI state.\n\nExample:\n"
                   "creaturevisdebug skeletor\n\n"
                   "The above command wil turn on visual debugging for the creature named \'skeletor\'. "
                   "The same command will turn it back off again.",
                   cSendCmdToServer,
                   cSrvCreatureVisDebug,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"creaturevisdebug"});
    cl.addCommand("seatvisualdebug",
                   "Visual debugging is a way to see all the tiles a given seat can see.\n\nExample:\n"
                   "seatvisualdebug 1\n\nThe above command will show every tiles seat 1 can see.  The same command will turn it off.",
                   cSendCmdToServer,
                   cSrvSeatVisDebug,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"seatvisdebug"});
    cl.addCommand("togglefow",
                   "Toggles on/off fog of war for every connected player",
                   cSendCmdToServer,
                   cSrvToggleFOW,
                   {AbstractModeManager::ModeType::GAME},
                   {"icanseedeadpeople"});
    cl.addCommand("setcreaturelevel",
                   "Sets the level of a given creature.\n\nExample:\n"
                   "setlevel BigKnight1 10\n\nThe above command will set the creature \'BigKnight1\' to 10.",
                   cSendCmdToServer,
                   cSrvSetCreatureLevel,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("hermitecatmullspline",
                   "Triggers the catmullspline camera movement behaviour.\n\nExample:\n"
                   "catmullspline 6 4 5 4 6 5 7\n"
                   "Make the camera follow a lazy curved path along the given coordinates pairs. "
                   "The first parameter is the number of pairs",
                   cHermiteCatmullSpline,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"catmullspline"});
    cl.addCommand("circlearound",
                   "Triggers the circle camera movement behaviour.\n\nExample:\n"
                   "circlearound 6 4 8\n"
                   "Make the camera follow a lazy a circle path around coors 6,4 with a radius of 8.",
                   cCircleAround,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("setcamerafovy",
                   "Sets the camera vertical field of view aspect ratio on the Y axis.\n\nExample:\n"
                   "setcamerafovy 45",
                   cSetCameraFOVy,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("addgold",
                   "'addgold' adds the given amount of gold to one player. It takes as arguments the color of the player to"
                   "whom the gold should be given and the amount. If the player's treasuries are full, no more gold is given."
                   "Note that this command is available in server mode only. \n\nExample\n"
                   "to give 5000 gold to player color 1 : addgold 1 5000",
                   cSendCmdToServer,
                   cSrvAddGold,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("setcreaturedest",
                   "Sets the camera vertical field of view aspect ratio on the Y axis.\n\nExample:\n"
                   "setcamerafovy 45",
                   cSendCmdToServer,
                   cSrvSetCreatureDest,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"setcreaturedestination"});
    cl.addCommand("logfloodfill",
                   "'logfloodfill' logs the FloodFillValues of all the Tiles in the GameMap.",
                   cSendCmdToServer,
                   cSrvLogFloodFill,
                   {AbstractModeManager::ModeType::GAME},
                   {});
    cl.addCommand("listmeshanims",
                   "'listmeshanims' lists all the animations for the given mesh.",
                   cListMeshAnims,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {"listmeshanimations"});
    cl.addCommand("keys",
                   "list keys",
                   cKeys,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR},
                   {});
    cl.addCommand("triggercompositor",
                   "Starts the given compositor. The compositor must exist.\n\nExample:\n"
                   "triggercompositor blacknwhite",
                   cTriggerCompositor,
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("helpmessage",
                   "Display help message",
                   [](const Command::ArgumentList_t&, ConsoleInterface& c, AbstractModeManager&) {
                        c.print(HELPMESSAGE);
                        return Command::Result::SUCCESS;
                   },
                   Command::cStubServer,
                   {AbstractModeManager::ModeType::GAME, AbstractModeManager::ModeType::EDITOR});
    cl.addCommand("unlockskills",
                   "Unlock all skills for every seats\n"
                   "unlockskills",
                   cSendCmdToServer,
                   cSrvUnlockSkills,
                   {AbstractModeManager::ModeType::GAME});

}

} // namespace ConsoleCommands
