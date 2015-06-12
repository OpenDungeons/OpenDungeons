/*
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

#ifndef GAMEEDITORMODEBASE_H
#define GAMEEDITORMODEBASE_H

#include "modes/AbstractApplicationMode.h"

#include "game/PlayerSelection.h"
#include "gamemap/MiniMapCamera.h"

class EntityBase;
class GameEditorModeConsole;

namespace CEGUI
{
    class Window;
}

class ChatMessage;
class EventMessage;

enum class RoomType;
enum class TrapType;

//! \brief Class containing common functionality between editor and game modes.
class GameEditorModeBase : public AbstractApplicationMode
{
public:
    GameEditorModeBase(ModeManager *modeManager, ModeManager::ModeType modeType, CEGUI::Window* rootWindow);
    ~GameEditorModeBase();

    void onFrameStarted(const Ogre::FrameEvent& evt) override;

    //! \brief Receive and display some chat text
    void receiveChat(ChatMessage* message);

    //! \brief Receive and display some event text
    void receiveEventShortNotice(EventMessage* event);

    //! \brief Called when the mode is activated.
    virtual void activate() = 0;

    //! \brief Called when the mode is activated.
    void deactivate() override;

    //! \brief Leave the console.
    void leaveConsole();
protected:
    enum InputMode
    {
        InputModeNormal,
        InputModeChat,
        InputModeConsole
    };

    //! \brief The current input mode.
    //! If in chat mode, then the game keyboard keys are interpreted as regular keys.
    InputMode mCurrentInputMode;

    void connectGuiAction(const std::string& buttonName, AbstractApplicationMode::GuiAction action);

    //! \brief Update the chat and event messages seen.
    void updateMessages(Ogre::Real update_time);

    //! \brief The main CEGUI window.
    CEGUI::Window* mRootWindow;

    //! \brief A reference to the game map used by the game mode
    //! For now, handled by the frame listener, don't delete it from here.
    GameMap* mGameMap;

    //! \brief The minimap used in this mode
    MiniMapCamera mMiniMap;

    PlayerSelection mPlayerSelection;

    //! \brief Set the tab button tooltip according to the pane tooltip for every tabs
    //! in the 'tabControlName' widget.
    //! \param tabControlName The tab control widget name. Eg: "parentWidget/tabControlName"
    //! \note This permits to workaround a CEGUI design issue.
    void syncTabButtonTooltips(const CEGUI::String& tabControlName);

    //! \brief Enter the console.
    void enterConsole();

    //! \brief Get the console component.
    GameEditorModeConsole* getConsole()
    { return mConsole.get(); }
private:
    //! \brief The game event messages in queue.
    std::vector<EventMessage*> mEventMessages;

    //! \brief The console instance.
    std::unique_ptr<GameEditorModeConsole> mConsole;

    //! \brief Minimap click event handler
    bool onMinimapClick(const CEGUI::EventArgs& arg);

    //! \brief helper functions to connect buttons to entities
    void connectRoomSelect(const std::string& buttonName, RoomType roomType);
    void connectTrapSelect(const std::string& buttonName, TrapType trapType);
};

#endif // GAMEEDITORMODEBASE_H
