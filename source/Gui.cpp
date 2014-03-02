/*
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Class Gui containing all the stuff for the GUI, including translation.
 */

//TODO: The event handlers should be call functions to AS instead of hardcoded so that we can
//      script the GUI actions. Maybe even register CEGUI to AS to make it fully scripted?
//      Then we could easily adjust the GUI without recompiling.

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/Renderer.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/System.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/Event.h>


#include "MapLoader.h"
#include "ODFrameListener.h"
#include "GameMap.h"
#include "Player.h"
#include "Trap.h"
#include "TextRenderer.h"
#include "ODApplication.h"
#include "Functions.h"
#include "CameraManager.h"
#include "MiniMap.h"
#include "Gui.h"
#include "ModeManager.h"
#include "EditorMode.h"



template<> Gui* Ogre::Singleton<Gui>::msSingleton = 0;

/*! \brief Constructor that initializes the whole CEGUI system
 *  including renderer, system, resource provider, setting defaults,
 *  loading all sheets, assigning all event handler
 */
Gui::Gui() :
        activeSheet(hideGui)
{
    CEGUI::OgreRenderer::bootstrapSystem();
    //CEGUI::SchemeManager::getSingleton().create("OpenDungeonsSkin.scheme");
    CEGUI::SchemeManager::getSingleton().createFromFile("OpenDungeonsSkin.scheme");

    // CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setImage("OpenDungeons/MouseArrow");
    // CEGUI::System::getSingleton().getDefaultGUIContext().getMouseCursor().setVisible(true);
    CEGUI::System::getSingleton().getDefaultGUIContext().setDefaultTooltipObject(new CEGUI::Tooltip("OD","Tooltip"));

    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
    CEGUI::Window* myHide = wmgr->createWindow( "DefaultWindow", "DummyWindow" );
    sheets[hideGui] = myHide;
    sheets[inGameMenu] = wmgr->loadLayoutFromFile("OpenDungeons.layout");
    sheets[mainMenu] = wmgr->loadLayoutFromFile("OpenDungeonsMainMenu.layout");
    sheets[editorToolBox] =  wmgr->loadLayoutFromFile("OpenDungeonsEditorToolBox.layout");

    mainMenuMode = false;

    assignEventHandlers();
    loadGuiSheet(mainMenu);
}

Gui::~Gui()
{
    CEGUI::OgreRenderer::destroySystem();
}




/*! \brief A required function to pass input to the OIS system.
 *
 */
CEGUI::MouseButton Gui::convertButton(const OIS::MouseButtonID& buttonID)
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
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().setRootWindow(sheets[newSheet]);
    //This shouldn't be needed, but the gui seems to not allways change when using hideGui without it.
    CEGUI::System::getSingletonPtr()->getDefaultGUIContext().markAsDirty() ;
}

CEGUI::Window* Gui::getGuiSheet(const guiSheet& sheet)
{
    if (sheets.find(sheet) != sheets.end())
    {
        return sheets[sheet];
    }
    return NULL;
}


/*! \brief Assigns all event handlers to the GUI elements
 */
void Gui::assignEventHandlers()
{
    std::cout << "Gui::assignEventHandlers()" << std::endl;

    CEGUI::Window* rootWindow = sheets[inGameMenu];
    // CEGUI::Window* rootWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();

    if (rootWindow != NULL)
    {
        // std::cout << " root window children count: " << CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChildCount() << std::endl;

        // for (int k = 0; k < rootWindow->getChildCount(); k++)
        // {
        //     CEGUI::Window* childWindow = rootWindow->getChildAtIdx(k);
        //     std::cout << " * " << k << ": " << childWindow->getName() << " -- " << childWindow->getNamePath() << std::endl;
        // }

        sheets[inGameMenu]->getChild(BUTTON_QUARTERS)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&quartersButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_TREASURY)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&treasuryButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_FORGE)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&forgeButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_DOJO)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&dojoButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_CANNON)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&cannonButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_HOST)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&serverButtonPressed));

        sheets[inGameMenu]->getChild(BUTTON_QUIT)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&quitButtonPressed));
    }
    else
    {
        std::cerr << "ERROR: No Root window pointer!!!" << std::endl;
    }

    // rootWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    if (rootWindow != NULL)
    {
        // for (int k = 0; k < rootWindow->getChildCount(); k++)
        // {
        //     CEGUI::Window* childWindow = rootWindow->getChildAtIdx(k);
        //     std::cout << " * " << k << ": " << childWindow->getName() << " -- " << childWindow->getNamePath() << std::endl;

        //     for (int l = 0; l < childWindow->getChildCount(); l++)
        //     {
        //         CEGUI::Window* childWindow2 = childWindow->getChildAtIdx(l);
        //         std::cout << "   - " << l << ": " << childWindow2->getName() << " -- " << childWindow2->getNamePath() << std::endl;
        //     }
        // }

        sheets[mainMenu]->getChild(MM_BUTTON_START_NEW_GAME)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&mMNewGameButtonPressed));

        sheets[mainMenu]->getChild(MM_BUTTON_MAPEDITOR)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&mMMapEditorButtonPressed));

        sheets[mainMenu]->getChild(MM_BUTTON_QUIT)->subscribeEvent(
                CEGUI::PushButton::EventClicked,
                CEGUI::Event::Subscriber(&mMQuitButtonPressed));
    }
    else
    {
        std::cerr << "ERROR: No Root window pointer!!!" << std::endl;
    }

    // rootWindow = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    if (rootWindow != NULL)
    {
        // for (int k = 0; k < rootWindow->getChildCount(); k++)
        // {
        //     CEGUI::Window* childWindow = rootWindow->getChildAtIdx(k);
        //     std::cout << " * " << k << ": " << childWindow->getName() << " -- " << childWindow->getNamePath() << std::endl;
        // }

        sheets[inGameMenu]->getChild(MINIMAP)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&miniMapclicked));

        sheets[editorToolBox]->getChild(TOOLSPALETE_LAVA_BUTTON)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&tpLavaButtonPressed));

        sheets[editorToolBox]->getChild(TOOLSPALETE_GOLD_BUTTON)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&tpGoldButtonPressed));

        sheets[editorToolBox]->getChild(TOOLSPALETE_ROCK_BUTTON)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&tpRockButtonPressed));

        sheets[editorToolBox]->getChild(TOOLSPALETE_WATER_BUTTON)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&tpWaterButtonPressed));

        sheets[editorToolBox]->getChild(TOOLSPALETE_DIRT_BUTTON)->subscribeEvent(
            CEGUI:: Window::EventMouseClick,
            CEGUI::Event::Subscriber(&tpDirtButtonPressed));
    }
    else
    {
        std::cerr << "ERROR: No Root window pointer!!!" << std::endl;
    }

}

bool Gui::miniMapclicked(const CEGUI::EventArgs& e)
{
    CEGUI::MouseEventArgs& ee = (CEGUI::MouseEventArgs&)e;

    Ogre::Vector2 cc = ODFrameListener::getSingleton().getGameMap()->getMiniMap()->camera_2dPositionFromClick((int)ee.position.d_x,
                                                                                                              (int)ee.position.d_y);

    modeManager->mMc->frameListener->cm->onMiniMapClick(cc);

    //std::cerr<< xx <<" "<< yy << " " <<std::endl;
    return true;
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
    gameMap->getLocalPlayer()->setNewRoomType(Room::quarters);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "quarters");
    return true;
}

bool Gui::treasuryButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::treasury);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "treasury");
    return true;
}

bool Gui::forgeButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::forge);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "forge");
    return true;
}

bool Gui::dojoButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::dojo);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::nullTrapType);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "dojo");
    return true;
}

bool Gui::cannonButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    gameMap->getLocalPlayer()->setNewRoomType(Room::nullRoomType);
    gameMap->getLocalPlayer()->setNewTrapType(Trap::cannon);
    TextRenderer::getSingleton().setText(ODApplication::POINTER_INFO_STRING, "cannon");
    return true;
}

bool Gui::serverButtonPressed(const CEGUI::EventArgs& e)
{
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    return startServer(*gameMap);
}


bool Gui::tpGoldButtonPressed(const CEGUI::EventArgs& e)
{
    if (!(modeManager->getCurrentModeType() == ModeManager::EDITOR))
        return true;

    static_cast<EditorMode*>(modeManager->getCurrentMode())->mCurrentTileType = Tile::gold;
    return true;
}

bool Gui::tpLavaButtonPressed(const CEGUI::EventArgs& e)
{
    if (!(modeManager->getCurrentModeType() == ModeManager::EDITOR))
        return true;

    static_cast<EditorMode*>(modeManager->getCurrentMode())->mCurrentTileType = Tile::lava;
    return true;
}

bool Gui::tpRockButtonPressed(const CEGUI::EventArgs& e)
{
    if (!(modeManager->getCurrentModeType() == ModeManager::EDITOR))
        return true;

    static_cast<EditorMode*>(modeManager->getCurrentMode())->mCurrentTileType = Tile::rock;
    return true;
}

bool Gui::tpWaterButtonPressed(const CEGUI::EventArgs& e)
{
    if (!(modeManager->getCurrentModeType() == ModeManager::EDITOR))
        return true;

    static_cast<EditorMode*>(modeManager->getCurrentMode())->mCurrentTileType = Tile::water;
    return true;
}

bool Gui::tpDirtButtonPressed(const CEGUI::EventArgs& e)
{
    if (!(modeManager->getCurrentModeType() == ModeManager::EDITOR))
        return true;

    static_cast<EditorMode*>(modeManager->getCurrentMode())->mCurrentTileType = Tile::dirt;
    return true;
}

//! \brief What happens after a click on New Game in the main menu
bool Gui::mMNewGameButtonPressed(const CEGUI::EventArgs& e)
{
    Gui::getSingletonPtr()->loadGuiSheet(inGameMenu);
    modeManager->addGameMode(ModeManager::GAME);
    ODFrameListener::getSingleton().makeGameContext();
    GameMap* gameMap = ODFrameListener::getSingleton().getGameMap();
    return startServer(*gameMap);
}

bool Gui::mMMapEditorButtonPressed(const CEGUI::EventArgs& e)
{
    // CEGUI::Window *myWin =   CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow();
    Gui::getSingletonPtr()->loadGuiSheet(editorToolBox);

    // myWin->addChildWindow(sheets[editorToolBox]);
    modeManager->addGameMode(ModeManager::EDITOR);
    ODFrameListener::getSingleton().makeEditorContext();

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
      loadGuiSheet(inGameMenu);
    }
}

void Gui::switchGuiMode()
{
  if(mainMenuMode)
    {
      loadGuiSheet(inGameMenu);
    }
  else
    {
      loadGuiSheet(mainMenu);
    }
  mainMenuMode = ! mainMenuMode;
}






/*! \brief shows (true) or hides (false) the GUI
 *
 */
void Gui::setVisible(bool visible)
{
    loadGuiSheet(visible ? inGameMenu : hideGui);
}

/* These constants are used to access the GUI element
 * NOTE: when add/remove/rename a GUI element, don't forget to change it here
 */
//TODO: Probably these should be read from a file? Script file?

//const std::string Gui::DISPLAY_GOLD = "Root/GoldDisplay";
//const std::string Gui::DISPLAY_MANA = "Root/ManaDisplay";
//const std::string Gui::DISPLAY_TERRITORY = "Root/TerritoryDisplay";
//const std::string Gui::MINIMAP = "Root/MiniMap";
//const std::string Gui::MESSAGE_WINDOW = "Root/MessagesDisplayWindow";
//const std::string Gui::MAIN_TABCONTROL = "Root/MainTabControl";
//const std::string Gui::TAB_ROOMS = "Root/MainTabControl/Rooms";
//const std::string Gui::BUTTON_QUARTERS = "Root/MainTabControl/Rooms/QuartersButton";
//const std::string Gui::BUTTON_FORGE = "Root/MainTabControl/Rooms/ForgeButton";
//const std::string Gui::BUTTON_DOJO = "Root/MainTabControl/Rooms/DojoButton";
//const std::string Gui::BUTTON_TREASURY = "Root/MainTabControl/Rooms/TreasuryButton";
//const std::string Gui::TAB_TRAPS = "Root/MainTabControl/Traps";
//const std::string Gui::BUTTON_CANNON = "Root/MainTabControl/Traps/CannonButton";
//const std::string Gui::TAB_SPELLS = "Root/MainTabControl/Spells";
//const std::string Gui::TAB_CREATURES = "Root/MainTabControl/Creatures";
//const std::string Gui::TAB_COMBAT = "Root/MainTabControl/Combat";
//const std::string Gui::TAB_SYSTEM = "Root/MainTabControl/System";
//const std::string Gui::BUTTON_HOST = "Root/MainTabControl/System/HostButton";
//const std::string Gui::BUTTON_QUIT = "Root/MainTabControl/System/QuitButton";
const std::string Gui::DISPLAY_GOLD = "HorizontalPipe/GoldDisplay";
const std::string Gui::DISPLAY_MANA = "HorizontalPipe/ManaDisplay";
const std::string Gui::DISPLAY_TERRITORY = "HorizontalPipe/TerritoryDisplay";
const std::string Gui::MINIMAP = "MiniMap";
const std::string Gui::MESSAGE_WINDOW = "MessagesDisplayWindow";
const std::string Gui::MAIN_TABCONTROL = "MainTabControl";
const std::string Gui::TAB_ROOMS = "MainTabControl/Rooms";
const std::string Gui::BUTTON_QUARTERS = "MainTabControl/Rooms/QuartersButton";
const std::string Gui::BUTTON_FORGE = "MainTabControl/Rooms/ForgeButton";
const std::string Gui::BUTTON_DOJO = "MainTabControl/Rooms/DojoButton";
const std::string Gui::BUTTON_TREASURY = "MainTabControl/Rooms/TreasuryButton";
const std::string Gui::TAB_TRAPS = "MainTabControl/Traps";
const std::string Gui::BUTTON_CANNON = "MainTabControl/Traps/CannonButton";
const std::string Gui::TAB_SPELLS = "MainTabControl/Spells";
const std::string Gui::TAB_CREATURES = "MainTabControl/Creatures";
const std::string Gui::TAB_COMBAT = "MainTabControl/Combat";
const std::string Gui::TAB_SYSTEM = "MainTabControl/System";
const std::string Gui::BUTTON_HOST = "MainTabControl/System/HostButton";
const std::string Gui::BUTTON_QUIT = "MainTabControl/System/QuitButton";

const std::string Gui::MM = "MainMenu";
const std::string Gui::MM_WELCOME_MESSAGE = "Background/WelcomeMessage";
const std::string Gui::MM_BUTTON_START_NEW_GAME = "Background/StartNewGameButton";
const std::string Gui::MM_BUTTON_MAPEDITOR = "Background/MapEditorButton";
const std::string Gui::MM_BUTTON_QUIT = "Background/QuitButton";
const std::string Gui::TOOLSPALETE = "TOOLSPALETE";
const std::string Gui::CREATURESSHUFFLE = "TOOLSPALETE/CreaturesShuffle";


const std::string Gui::TOOLSPALETE_LAVA_BUTTON = "TOOLSPALETE/LavaButton";
const std::string Gui::TOOLSPALETE_GOLD_BUTTON = "TOOLSPALETE/GoldButton";
const std::string Gui::TOOLSPALETE_DIRT_BUTTON = "TOOLSPALETE/DirtButton";
const std::string Gui::TOOLSPALETE_WATER_BUTTON = "TOOLSPALETE/WaterButton";
const std::string Gui::TOOLSPALETE_ROCK_BUTTON = "TOOLSPALETE/RockButton";

ModeManager* Gui::modeManager = NULL;
