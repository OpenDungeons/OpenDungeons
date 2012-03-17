/*!
 * \file   Gui.cpp
 * \date   05 April 2011
 * \author StefanP.MUC
 * \brief  Header for class Gui containing all the stuff for the GUI,
 *         including translation.
 */

#ifndef GUI_H_
#define GUI_H_

#include <string>
#include <map>

#include <CEGUI.h>
#include <OIS.h>
#include <OgreSingleton.h>

class GameMap;

/*! \brief This class holds all GUI related functions
 */
class Gui : public Ogre::Singleton<Gui>
{
    public:
        enum guiSheet
        {
            hideGui,
            mainMenu,
            optionsMenu,
            ingameMenu
        };

        Gui();
        ~Gui();

        void                loadGuiSheet    (const guiSheet& newSheet);
        CEGUI::MouseButton  convertButton   (const OIS::MouseButtonID& buttonID);
        void                setVisible      (bool visible);
        void                toggleGui       ();

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
        static const std::string BUTTON_TREASURY;
        static const std::string TAB_TRAPS;
        static const std::string BUTTON_CANNON;
        static const std::string TAB_SPELLS;
        static const std::string TAB_CREATURES;
        static const std::string TAB_COMBAT;
        static const std::string TAB_SYSTEM;
        static const std::string BUTTON_HOST;
        static const std::string BUTTON_QUIT;
        static const std::string MM;
        static const std::string MM_WELCOME_MESSAGE;
        static const std::string MM_BUTTON_START_NEW_GAME;
        static const std::string MM_BUTTON_MAPEDITOR;
        static const std::string MM_BUTTON_QUIT;

    private:
        Gui(const Gui&);

        void assignEventHandlers();

        std::map<guiSheet, CEGUI::Window*> sheets;

        guiSheet activeSheet;

        //Button handlers game UI
        static bool quitButtonPressed       (const CEGUI::EventArgs& e);
        static bool quartersButtonPressed   (const CEGUI::EventArgs& e);
        static bool treasuryButtonPressed   (const CEGUI::EventArgs& e);
        static bool forgeButtonPressed      (const CEGUI::EventArgs& e);
        static bool dojoButtonPressed       (const CEGUI::EventArgs& e);
        static bool cannonButtonPressed     (const CEGUI::EventArgs& e);
        static bool serverButtonPressed     (const CEGUI::EventArgs& e);

        //Button handlers main menu
        static bool mMNewGameButtonPressed  (const CEGUI::EventArgs& e);
        static bool mMMapEditorButtonPressed(const CEGUI::EventArgs& e);
        static bool mMLoadButtonPressed     (const CEGUI::EventArgs& e);
        static bool mMOptionsButtonPressed  (const CEGUI::EventArgs& e);
        static bool mMQuitButtonPressed     (const CEGUI::EventArgs& e);
};

#endif /* GUI_H_ */
