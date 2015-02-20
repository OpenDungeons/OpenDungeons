/*!
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Header for class Gui containing all the stuff for the GUI,
 *         including translation.
 *
 *  Copyright (C) 2011-2015  OpenDungeons Team
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

#include <string>
#include <map>

class ModeManager;
class GameMap;
class SoundEffectsManager;

//! \brief This class holds all GUI related functions
class Gui
{
public:
    enum guiSheet
    {
        hideGui,
        mainMenu,
        skirmishMenu,
        multiplayerClientMenu,
        multiplayerServerMenu,
        editorMenu,
        editorModeGui,
        optionsMenu,
        inGameMenu,
        replayMenu,
        configureSeats,
        console
    };

    /*! \brief Constructor that initializes the whole CEGUI system
     *  including renderer, system, resource provider, setting defaults,
     *  loading all sheets, assigning all event handler
     */
    Gui(SoundEffectsManager* soundEffectsManager, const std::string& ceguiLogFileName);

    ~Gui();

    //! \brief loads the specified gui sheet
    void loadGuiSheet(const guiSheet& newSheet);

    //! \brief A required function to pass input to the OIS system.
    static CEGUI::MouseButton convertButton (OIS::MouseButtonID buttonID);

    CEGUI::Window* getGuiSheet(const guiSheet&);

    //Access names of the GUI elements
    static const std::string ROOT;
    static const std::string DISPLAY_GOLD;
    static const std::string DISPLAY_MANA;
    static const std::string DISPLAY_TERRITORY;
    static const std::string MINIMAP;
    static const std::string OBJECTIVE_TEXT;
    static const std::string MAIN_TABCONTROL;
    static const std::string TAB_ROOMS;
    static const std::string BUTTON_DORMITORY;
    static const std::string BUTTON_FORGE;
    static const std::string BUTTON_TRAININGHALL;
    static const std::string BUTTON_LIBRARY;
    static const std::string BUTTON_HATCHERY;
    static const std::string BUTTON_TREASURY;
    static const std::string BUTTON_CRYPT;
    static const std::string BUTTON_TEMPLE;
    static const std::string BUTTON_PORTAL;
    static const std::string BUTTON_DESTROY_ROOM;
    static const std::string TAB_TRAPS;
    static const std::string BUTTON_TRAP_CANNON;
    static const std::string BUTTON_TRAP_SPIKE;
    static const std::string BUTTON_TRAP_BOULDER;
    static const std::string BUTTON_DESTROY_TRAP;
    static const std::string TAB_SPELLS;
    static const std::string BUTTON_SPELL_SUMMON_WORKER;
    static const std::string BUTTON_SPELL_CALLTOWAR;
    static const std::string TAB_CREATURES;
    static const std::string BUTTON_CREATURE_WORKER;
    static const std::string BUTTON_CREATURE_FIGHTER;
    static const std::string TAB_COMBAT;
    static const std::string MM_BACKGROUND;
    static const std::string MM_WELCOME_MESSAGE;
    static const std::string MM_BUTTON_START_SKIRMISH;
    static const std::string MM_BUTTON_START_REPLAY;
    static const std::string MM_BUTTON_START_MULTIPLAYER_CLIENT;
    static const std::string MM_BUTTON_START_MULTIPLAYER_SERVER;
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
    static const std::string EDITOR_SEAT_ID;
    static const std::string EDITOR_CREATURE_SPAWN;
    static const std::string EDITOR_MAPLIGHT_BUTTON;
    static const std::string EXIT_CONFIRMATION_POPUP;
    static const std::string EXIT_CONFIRMATION_POPUP_YES_BUTTON;
    static const std::string EXIT_CONFIRMATION_POPUP_NO_BUTTON;
    static const std::string SKM_TEXT_LOADING;
    static const std::string SKM_BUTTON_LAUNCH;
    static const std::string SKM_BUTTON_BACK;
    static const std::string SKM_LIST_LEVELS;
    static const std::string MPM_TEXT_LOADING;
    static const std::string MPM_BUTTON_SERVER;
    static const std::string MPM_BUTTON_CLIENT;
    static const std::string MPM_BUTTON_BACK;
    static const std::string MPM_LIST_LEVELS;
    static const std::string MPM_EDIT_IP;
    static const std::string MPM_EDIT_NICK;
    static const std::string EDM_TEXT_LOADING;
    static const std::string EDM_BUTTON_LAUNCH;
    static const std::string EDM_BUTTON_BACK;
    static const std::string EDM_LIST_LEVELS;
    static const std::string CSM_BUTTON_LAUNCH;
    static const std::string CSM_BUTTON_BACK;
    static const std::string REM_TEXT_LOADING;
    static const std::string REM_BUTTON_LAUNCH;
    static const std::string REM_BUTTON_DELETE;
    static const std::string REM_BUTTON_BACK;
    static const std::string REM_LIST_REPLAYS;

private:
    //! \brief Assigns all event handlers to the GUI elements
    void assignEventHandlers();

    bool playButtonClickSound(const CEGUI::EventArgs& e);

    std::map<guiSheet, CEGUI::Window*> mSheets;

    // Button handlers main menu

    static bool dormitoryButtonPressed  (const CEGUI::EventArgs& e);
    static bool treasuryButtonPressed   (const CEGUI::EventArgs& e);
    static bool destroyRoomButtonPressed(const CEGUI::EventArgs& e);
    static bool forgeButtonPressed      (const CEGUI::EventArgs& e);
    static bool trainingHallButtonPressed (const CEGUI::EventArgs& e);
    static bool libraryButtonPressed    (const CEGUI::EventArgs& e);
    static bool hatcheryButtonPressed   (const CEGUI::EventArgs& e);
    static bool cryptButtonPressed      (const CEGUI::EventArgs& e);
    static bool templeButtonPressed     (const CEGUI::EventArgs& e);
    static bool portalButtonPressed     (const CEGUI::EventArgs& e);

    static bool cannonButtonPressed     (const CEGUI::EventArgs& e);
    static bool spikeTrapButtonPressed  (const CEGUI::EventArgs& e);
    static bool boulderTrapButtonPressed(const CEGUI::EventArgs& e);
    static bool destroyTrapButtonPressed(const CEGUI::EventArgs& e);

    static bool spellSummonWorkerPressed        (const CEGUI::EventArgs& e);

    static bool spellCallToWarPressed           (const CEGUI::EventArgs& e);

    static bool workerCreatureButtonPressed     (const CEGUI::EventArgs& e);
    static bool fighterCreatureButtonPressed    (const CEGUI::EventArgs& e);

    static bool confirmExitYesButtonPressed     (const CEGUI::EventArgs& e);
    static bool confirmExitNoButtonPressed      (const CEGUI::EventArgs& e);

    static bool helpButtonPressed(const CEGUI::EventArgs& e);
    static bool objectivesButtonPressed(const CEGUI::EventArgs& e);
    static bool optionsButtonPressed(const CEGUI::EventArgs& e);
    static bool hideObjectivesWindow(const CEGUI::EventArgs& e);

    static bool cancelSettings(const CEGUI::EventArgs& e);

    // Editor Buttons
    static bool editorLavaButtonPressed(const CEGUI::EventArgs& e);
    static bool editorGoldButtonPressed(const CEGUI::EventArgs& e);
    static bool editorRockButtonPressed(const CEGUI::EventArgs& e);
    static bool editorWaterButtonPressed(const CEGUI::EventArgs& e);
    static bool editorDirtButtonPressed(const CEGUI::EventArgs& e);
    static bool editorClaimedButtonPressed(const CEGUI::EventArgs& e);
    static bool editorMapLightButtonPressed(const CEGUI::EventArgs& e);

    SoundEffectsManager* mSoundEffectsManager;
};

#endif // GUI_H_
