/*
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Class Gui containing all the stuff for the GUI, including translation.
 */

#include <RendererModules/Ogre/CEGUIOgreRenderer.h>

#include "Globals.h"
#include "Functions.h"
#include "ExampleFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"
#include "TextRenderer.h"

#include "Gui.h"

template<> Gui* Ogre::Singleton<Gui>::ms_Singleton = 0;

/*! \brief Returns access to the singleton instance of Gui
 */
Gui& Gui::getSingleton()
{
    assert(ms_Singleton);
    return(*ms_Singleton);
}

/*! \brief Returns access to the pointer to the singleton instance of Gui
 */
Gui* Gui::getSingletonPtr()
{
    return ms_Singleton;
}

/*! \brief Constructor that initializes the whole CEGUI system
 *  including renderer, system, resource provider, setting defaults,
 *  loading all sheets, assigning all event handler
 */
Gui::Gui()
{
    CEGUI::OgreRenderer::bootstrapSystem();
    CEGUI::SchemeManager::getSingleton().create("OpenDungeonsSkin.scheme");
    CEGUI::System::getSingleton().setDefaultMouseCursor("OpenDungeons", "MouseArrow");
    CEGUI::System::getSingleton().setDefaultTooltip("OD/Tooltip");

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    sheets[ingameMenu] = wmgr->loadWindowLayout("OpenDungeons.layout");
    sheets[mainMenu] = wmgr->loadWindowLayout("OpenDungeonsMainMenu.layout");

    assignEventHandlers();
}

Gui::~Gui()
{
    CEGUI::OgreRenderer::destroySystem();
}

/*! \brief loads the specified gui sheet
 */
void Gui::loadGuiSheet(const guiSheet& newSheet)
{
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

    wmgr->getWindow(MM_BUTTON_QUIT)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&mMQuitButtonPressed));
}

bool Gui::quitButtonPressed(const CEGUI::EventArgs& e)
{
    writeGameMapToFile(std::string("levels/Test.level") + std::string(".out"));
    exampleFrameListener->mContinue = false;
    return true;
}

bool Gui::quartersButtonPressed(const CEGUI::EventArgs& e)
{
    gameMap.me->newRoomType = Room::quarters;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "quarters");
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    gameMap.me->newRoomType = Room::treasury;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "treasury");
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    gameMap.me->newRoomType = Room::forge;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "forge");
    return true;
}

bool Gui::dojoButtonPressed(const CEGUI::EventArgs& e)
{
    gameMap.me->newRoomType = Room::dojo;
    gameMap.me->newTrapType = Trap::nullTrapType;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "dojo");
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    gameMap.me->newRoomType = Room::nullRoomType;
    gameMap.me->newTrapType = Trap::cannon;
    TextRenderer::getSingleton().setText(POINTER_INFO_STRING, "cannon");
    return true;
}

bool Gui::serverButtonPressed(const CEGUI::EventArgs& e)
{
    return startServer();
}

//TODO: after the main menu loading is done, give some code to these handlers
//! \brief What happens after a click on New Game in the main menu
bool Gui::mMNewGameButtonPressed(const CEGUI::EventArgs& e)
{
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
    return true;
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
const std::string Gui::MM_BUTTON_QUIT = "MainMenu/QuitButton";
