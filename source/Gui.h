/*!
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Header for class Gui containing all the stuff for the GUI,
 *         including translation.
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

#ifndef GUI_H_
#define GUI_H_

#include <CEGUI/Window.h>
#include <CEGUI/EventArgs.h>
#include <CEGUI/InputEvent.h>

#include <OISMouse.h>
#include <OgreSingleton.h>

#include <string>
#include <map>

class ModeManager;
class GameMap;

//! \brief This class holds all GUI related functions
class Gui : public Ogre::Singleton<Gui>
{
    friend class MiniMap;

public:
    enum guiSheet
    {
        hideGui,
        mainMenu,
        optionsMenu,
        inGameMenu,
        editorMenu
    };

    /*! \brief Constructor that initializes the whole CEGUI system
     *  including renderer, system, resource provider, setting defaults,
     *  loading all sheets, assigning all event handler
     */
    Gui();

    ~Gui();

    //! \brief loads the specified gui sheet
    void loadGuiSheet(const guiSheet& newSheet);

    //! \brief A required function to pass input to the OIS system.
    CEGUI::MouseButton convertButton (const OIS::MouseButtonID& buttonID);

    CEGUI::Window* getGuiSheet(const guiSheet&);

    //Access names of the GUI elements
    static const std::string ROOT;
    static const std::string DISPLAY_GOLD;
    static const std::string DISPLAY_MANA;
    static const std::string DISPLAY_TERRITORY;
    static const std::string MINIMAP;
    static const std::string MESSAGE_WINDOW;
    static const std::string MAIN_TABCONTROL;
    static const std::string TAB_ROOMS;
    static const std::string BUTTON_QUARTERS;
    static const std::string BUTTON_FORGE;
    static const std::string BUTTON_DOJO;
    static const std::string BUTTON_LIBRARY;
    static const std::string BUTTON_TREASURY;
    static const std::string TAB_TRAPS;
    static const std::string BUTTON_CANNON;
    static const std::string TAB_SPELLS;
    static const std::string TAB_CREATURES;
    static const std::string TAB_COMBAT;
    static const std::string TAB_SYSTEM;
    static const std::string BUTTON_HOST;
    static const std::string BUTTON_QUIT;
    static const std::string MM_BACKGROUND;
    static const std::string MM_WELCOME_MESSAGE;
    static const std::string MM_BUTTON_START_NEW_GAME;
    static const std::string MM_BUTTON_MAPEDITOR;
    static const std::string MM_BUTTON_QUIT;
    static const std::string EDITOR;
    static const std::string EDITOR_LAVA_BUTTON;
    static const std::string EDITOR_GOLD_BUTTON;
    static const std::string EDITOR_ROCK_BUTTON;
    static const std::string EDITOR_WATER_BUTTON;
    static const std::string EDITOR_DIRT_BUTTON;
    static const std::string EDITOR_CLAIMED_BUTTON;
    static const std::string EDITOR_FULLNESS;
    static const std::string EDITOR_CURSOR_POS;
    static const std::string EXIT_CONFIRMATION_POPUP;
    static const std::string EXIT_CONFIRMATION_POPUP_YES_BUTTON;
    static const std::string EXIT_CONFIRMATION_POPUP_NO_BUTTON;


private:
    //! \brief Assigns all event handlers to the GUI elements
    void assignEventHandlers();

    std::map<guiSheet, CEGUI::Window*> sheets;

    // Button handlers main menu
    //! \brief What happens after a click on New Game in the main menu
    static bool mMNewGameButtonPressed  (const CEGUI::EventArgs& e);
    static bool mMMapEditorButtonPressed(const CEGUI::EventArgs& e);

    //! \brief What happens after a click on Load Game in the main menu
    static bool mMLoadButtonPressed     (const CEGUI::EventArgs& e);

    //! \brief What happens after a click on Options in the main menu
    static bool mMOptionsButtonPressed  (const CEGUI::EventArgs& e);

    //! \brief What happens after a click on Quit in the main menu
    static bool mMQuitButtonPressed     (const CEGUI::EventArgs& e);

    // Button handlers game UI
    static bool miniMapclicked          (const CEGUI::EventArgs& e);
    static bool quitButtonPressed       (const CEGUI::EventArgs& e);
    static bool quartersButtonPressed   (const CEGUI::EventArgs& e);
    static bool treasuryButtonPressed   (const CEGUI::EventArgs& e);
    static bool forgeButtonPressed      (const CEGUI::EventArgs& e);
    static bool dojoButtonPressed       (const CEGUI::EventArgs& e);
    static bool libraryButtonPressed    (const CEGUI::EventArgs& e);
    static bool cannonButtonPressed     (const CEGUI::EventArgs& e);
    static bool serverButtonPressed     (const CEGUI::EventArgs& e);
    static bool confirmExitYesButtonPressed     (const CEGUI::EventArgs& e);
    static bool confirmExitNoButtonPressed      (const CEGUI::EventArgs& e);

    // Editor Buttons
    static bool editorLavaButtonPressed(const CEGUI::EventArgs& e);
    static bool editorGoldButtonPressed(const CEGUI::EventArgs& e);
    static bool editorRockButtonPressed(const CEGUI::EventArgs& e);
    static bool editorWaterButtonPressed(const CEGUI::EventArgs& e);
    static bool editorDirtButtonPressed(const CEGUI::EventArgs& e);
    static bool editorClaimedButtonPressed(const CEGUI::EventArgs& e);
};

#endif // GUI_H_
