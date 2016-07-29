/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

//! \brief Enum to check if the chat message box should be displayed or not
//! It is used as a bit array
enum ChatMessageBoxDisplay
{
    hide                    = 0x00,
    showChatInput           = 0x01,
    showMessageReceived     = 0x02
};

class GameEditorModeConsole;
class GameMap;
class MiniMap;

namespace CEGUI
{
    class Window;
}

class ChatMessage;
class CullingManager;
class EventMessage;

enum class RoomType;
enum class TrapType;

//! \brief Functors for binding gui actions to spells/room/trap selection.
class RoomSelector
{
public:
    RoomSelector(RoomType roomType, PlayerSelection& playerSelection):
        mRoomType(roomType),
        mPlayerSelection(playerSelection)
    {
    }

    bool operator()(const CEGUI::EventArgs& e);

    RoomType mRoomType;
    PlayerSelection& mPlayerSelection;
};

class TrapSelector
{
public:
    TrapSelector(TrapType trapType, PlayerSelection& playerSelection):
        mTrapType(trapType),
        mPlayerSelection(playerSelection)
    {
    }

    bool operator()(const CEGUI::EventArgs& e);

    TrapType mTrapType;
    PlayerSelection& mPlayerSelection;
};

//! \brief Functor to select spell from gui
class SpellSelector
{
public:
    SpellSelector(SpellType spellType, PlayerSelection& playerSelection):
        mSpellType(spellType),
        mPlayerSelection(playerSelection)
    {
    }

    bool operator()(const CEGUI::EventArgs& e);

    SpellType mSpellType;
    PlayerSelection& mPlayerSelection;
};

//! \brief Class containing common functionality between editor and game modes.
class GameEditorModeBase : public AbstractApplicationMode
{
public:
    GameEditorModeBase(ModeManager *modeManager, ModeManager::ModeType modeType, CEGUI::Window* rootWindow);
    ~GameEditorModeBase();

    void onFrameStarted(const Ogre::FrameEvent& evt) override;

    void receiveChat(const ChatMessage& chat) override;

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

    //! \brief Show or hide the chat message box depending on mChatMessageBoxDisplay
    void refreshChatDisplay();

    //! \brief Counter to hide chat message box after some time with no
    //! new message
    Ogre::Real mChatMessageDisplayTime;
    //! \brief bit array to know if the chat display should be hiden or not
    uint32_t mChatMessageBoxDisplay;

    //! \brief The minimap used in this mode
    MiniMap* mMiniMap;

    //! \brief Culling manager for the main map
    CullingManager* mMainCullingManager;

    PlayerSelection mPlayerSelection;

    //! \brief This boolean is to be set by derived classes and will not erase the replay
    //! when the destructor is called if true
    bool mKeepReplayAtDisconnect;

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
};

#endif // GAMEEDITORMODEBASE_H
