/*
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Class Gui containing all the stuff for the GUI, including translation.
 */

#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

#include "Globals.h"
#include "MapLoader.h"
#include "ODFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"
#include "TextRenderer.h"
#include "ODApplication.h"
#include "Functions.h"

#include "Gui.h"

template<> Gui* Ogre::Singleton<Gui>::ms_Singleton = 0;

/*! \brief Constructor that initializes the whole CEGUI system
 *  including renderer, system, resource provider, setting defaults,
 *  loading all sheets, assigning all event handler
 */
Gui::Gui() :
        activeSheet(hideGui)
{
    CEGUI::OgreRenderer::bootstrapSystem();
    CEGUI::SchemeManager::getSingleton().create("OpenDungeonsSkin.scheme");
    CEGUI::System::getSingleton().setDefaultMouseCursor("OpenDungeons", "MouseArrow");
    CEGUI::System::getSingleton().setDefaultTooltip("OD/Tooltip");

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    sheets[hideGui] = 0;
    sheets[ingameMenu] = wmgr->loadWindowLayout("OpenDungeons.layout");
    sheets[mainMenu] = wmgr->loadWindowLayout("OpenDungeonsMainMenu.layout");

    assignEventHandlers();
}

Gui::~Gui()
{
    CEGUI::OgreRenderer::destroySystem();
}

/*! \brief A required function to pass input to the OIS system.
 *
 */
CEGUI::MouseButton Gui::convertButton(OIS::MouseButtonID buttonID)
{
    switch (buttonID)
    {
        case OIS::MB_Left:
            return CEGUI::LeftButton;

        case OIS::MB_Right:
            return CEGUI::RightButton;

        case OIS::MB_Middle:
            return CEGUI::MiddleButton;

        default:
            return CEGUI::LeftButton;
    }
}

/*! \brief loads the specified gui sheet
 */
void Gui::loadGuiSheet(const guiSheet& newSheet)
{
    activeSheet = newSheet;
    CEGUI::System::getSingletonPtr()->setGUISheet(sheets[newSheet]);
}

/*! \brief Assigns all event handlers to the GUI elements
 */
void Gui::assignEventHandlers()
{
    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

    wmgr->getWindow(BUTTON_QUARTERS)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&quartersButtonPressed));

    wmgr->getWindow(BUTTON_TREASURY)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&treasuryButtonPressed));

    wmgr->getWindow(BUTTON_FORGE)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&forgeButtonPressed));

    wmgr->getWindow(BUTTON_DOJO)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&dojoButtonPressed));

    wmgr->getWindow(BUTTON_CANNON)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&cannonButtonPressed));

    wmgr->getWindow(BUTTON_HOST)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&serverButtonPressed));

    wmgr->getWindow(BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&quitButtonPressed));

    wmgr->getWindow(MM_BUTTON_START_NEW_GAME)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMNewGameButtonPressed));

    wmgr->getWindow(MM_BUTTON_MAPEDITOR)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMMapEditorButtonPressed));

    wmgr->getWindow(MM_BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMQuitButtonPressed));
}

bool Gui::quitButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    MapLoader::writeGameMapToFile(std::string("levels/Test.level") + std::string(".out"), *gameMap);
    ODFrameListener::getSingletonPtr()->requestExit();
    return true;
}

bool Gui::quartersButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->newRoomType = Room::quarters;
    gameMap->getLocalPlayer()->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "quarters");
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->newRoomType = Room::treasury;
    gameMap->getLocalPlayer()->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "treasury");
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->newRoomType = Room::forge;
    gameMap->getLocalPlayer()->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "forge");
    return true;
}

bool Gui::dojoButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->newRoomType = Room::dojo;
    gameMap->getLocalPlayer()->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "dojo");
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->newRoomType = Room::nullRoomType;
    gameMap->getLocalPlayer()->newTrapType = Trap::cannon;
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "cannon");
    return true;
}

bool Gui::serverButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    return startServer(*gameMap);
}

//! \brief What happens after a click on New Game in the main menu
bool Gui::mMNewGameButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    Gui::getSingletonPtr()->loadGuiSheet(ingameMenu);
    return startServer(*gameMap);
}

bool Gui::mMMapEditorButtonPressed(const CEGUI::EventArgs& e)
{
    Gui::getSingletonPtr()->loadGuiSheet(ingameMenu);
    return true;
}

//! \brief What happens after a click on Load Game in the main menu
bool Gui::mMLoadButtonPressed(const CEGUI::EventArgs& e)
{
    return true;
}

//! \brief What happens after a click on Options in the main menu
bool Gui::mMOptionsButtonPressed(const CEGUI::EventArgs& e)
{
    return true;
}

//! \brief What happens after a click on Quit in the main menu
bool Gui::mMQuitButtonPressed(const CEGUI::EventArgs& e)
{
    ODFrameListener::getSingletonPtr()->requestExit();
    return true;
}

/*! \brief shows/hides the GUI if it is hidden/visible
 *
 */
void Gui::toggleGui()
{
    if(activeSheet != hideGui)
    {
        loadGuiSheet(hideGui);
    }
    else
    {
        loadGuiSheet(ingameMenu);
    }
}

/*! \brief shows (true) or hides (false) the GUI
 *
 */
void Gui::setVisible(const bool& visible)
{
    if(visible)
    {
        loadGuiSheet(ingameMenu);
    }
    else
    {
        loadGuiSheet(hideGui);
    }
}

/* These constants are used to access the GUI element
 * NOTE: when add/remove/rename a GUI element, don't forget to change it here
 */
const std::string Gui::ROOT = "Root";
const std::string Gui::DISPLAY_GOLD = "Root/GoldDisplay";
const std::string Gui::DISPLAY_MANA = "Root/ManaDisplay";
const std::string Gui::DISPLAY_TERRITORY = "Root/TerritoryDisplay";
const std::string Gui::MINIMAP = "Root/MiniMap";
const std::string Gui::MESSAGE_WINDOW = "Root/MessagesDisplayWindow";
const std::string Gui::MAIN_TABCONTROL = "Root/MainTabControl";
const std::string Gui::TAB_ROOMS = "Root/MainTabControl/Rooms";
const std::string Gui::BUTTON_QUARTERS = "Root/MainTabControl/Rooms/QuartersButton";
const std::string Gui::BUTTON_FORGE = "Root/MainTabControl/Rooms/ForgeButton";
const std::string Gui::BUTTON_DOJO = "Root/MainTabControl/Rooms/DojoButton";
const std::string Gui::BUTTON_TREASURY = "Root/MainTabControl/Rooms/TreasuryButton";
const std::string Gui::TAB_TRAPS = "Root/MainTabControl/Traps";
const std::string Gui::BUTTON_CANNON = "Root/MainTabControl/Traps/CannonButton";
const std::string Gui::TAB_SPELLS = "Root/MainTabControl/Spells";
const std::string Gui::TAB_CREATURES = "Root/MainTabControl/Creatures";
const std::string Gui::TAB_COMBAT = "Root/MainTabControl/Combat";
const std::string Gui::TAB_SYSTEM = "Root/MainTabControl/System";
const std::string Gui::BUTTON_HOST = "Root/MainTabControl/System/HostButton";
const std::string Gui::BUTTON_QUIT = "Root/MainTabControl/System/QuitButton";
const std::string Gui::MM = "MainMenu";
const std::string Gui::MM_WELCOME_MESSAGE = "MainMenu/WelcomeMessage";
const std::string Gui::MM_BUTTON_START_NEW_GAME = "MainMenu/StartNewGameButton";
const std::string Gui::MM_BUTTON_MAPEDITOR = "MainMenu/MapEditorButton";
const std::string Gui::MM_BUTTON_QUIT = "MainMenu/QuitButton";
