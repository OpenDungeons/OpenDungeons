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

#ifndef GAMEMODE_H
#define GAMEMODE_H

#include "GameEditorModeBase.h"

#include "modes/InputCommand.h"
#include "modes/InputBridge.h"
#include "modes/SettingsWindow.h"

#include "utils/ConfigManager.h"
#include <CEGUI/EventArgs.h>

namespace CEGUI
{
class Window;
}

enum class SpellType;
enum class SkillType;

//! \brief utility class to store and display the current skill completion. To make clearer
//! if it fills up or down, we fill it smoothly each time the player opens the skill tree
class SkillCurrentCompletion
{
public:
    SkillCurrentCompletion()
    {
        resetValue();
    }

    void resetValue()
    {
        mProgressBar = nullptr;
        mCompleteness = 0;
        mCompletenessDisplayed = 0;
    }

    void setValue(CEGUI::ProgressBar* progressBar, float completeness)
    {
        mProgressBar = progressBar;
        mCompleteness = completeness;
        mCompletenessDisplayed = 0;
    }

    //! \brief The progressbar to update at each frame update
    CEGUI::ProgressBar* mProgressBar;
    //! \brief The completeness
    float mCompleteness;
    //! \brief Value currently displayed [0-completeness]
    float mCompletenessDisplayed;
};

class GameMode final : public GameEditorModeBase, public InputCommand
{
 public:
    GameMode(ModeManager*);

    virtual ~GameMode();

    /*! \brief Process the mouse movement event.
     *
     * The function does a raySceneQuery to determine what object the mouse is over
     * to handle things like dragging out selections of tiles and selecting
     * creatures.
     */
    virtual bool mouseMoved     (const OIS::MouseEvent &arg) override;

    /*! \brief Handle mouse clicks.
     *
     * This function does a ray scene query to determine what is under the mouse
     * and determines whether a creature or a selection of tiles, is being dragged.
     */
    virtual bool mousePressed   (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;

    /*! \brief Handle mouse button releases.
     *
     * Finalize the selection of tiles or drop a creature when the user releases the mouse button.
     */
    virtual bool mouseReleased  (const OIS::MouseEvent &arg, OIS::MouseButtonID id) override;

    //! \brief Handle the keyboard input.
    virtual bool keyPressed     (const OIS::KeyEvent &arg) override;

    /*! \brief Process the key up event.
     *
     * When a key is released during normal gamplay the camera movement may need to be stopped.
     */
    virtual bool keyReleased    (const OIS::KeyEvent &arg) override;

    /*! \brief defines what the hotkeys do
     *
     * currently the only thing the hotkeys do is moving the camera around.
     * If the shift key is pressed we store this hotkey location
     * otherwise we fly the camera to a stored position.
     */
    virtual void handleHotkeys  (OIS::KeyCode keycode) override;

    void onFrameStarted(const Ogre::FrameEvent& evt) override;
    void onFrameEnded(const Ogre::FrameEvent& evt) override;

    //! \brief Called when the game mode is activated
    //! Used to call the corresponding Gui Sheet.
    void activate() override;

    //! \brief Called when exit button is pressed
    void popupExit(bool pause);

    virtual void notifyGuiAction(GuiAction guiAction) override;

    //! \brief Shows/hides/toggles the help window
    bool showHelpWindow(const CEGUI::EventArgs& = {});
    bool hideHelpWindow(const CEGUI::EventArgs& = {});
    bool toggleHelpWindow(const CEGUI::EventArgs& = {});

    //! \brief Shows/hides/toggles the objectives window
    bool showObjectivesWindow(const CEGUI::EventArgs& = {});
    bool hideObjectivesWindow(const CEGUI::EventArgs& = {});
    bool toggleObjectivesWindow(const CEGUI::EventArgs& = {});

    //! \brief Shows/hides/toggles the player settings window
    bool showPlayerSettingsWindow(const CEGUI::EventArgs& = {});
    bool togglePlayerSettingsWindow(const CEGUI::EventArgs& = {});
    bool cancelPlayerSettings(const CEGUI::EventArgs& = {});
    bool applyPlayerSettings(const CEGUI::EventArgs& = {});
    //! \brief Synchronize settings displayed in the menu with settings in the seat
    void syncPlayerSettings();

    //! \brief Shows/hides/toggles the Skill window
    bool showSkillWindow(const CEGUI::EventArgs& = {});
    bool hideSkillWindow(const CEGUI::EventArgs& = {});
    bool toggleSkillWindow(const CEGUI::EventArgs& = {});
    bool applySkillWindow(const CEGUI::EventArgs& = {});
    bool unselectAllSkillWindow(const CEGUI::EventArgs& = {});
    bool autoFillSkillWindow(const CEGUI::EventArgs& = {});
    void closeSkillWindow(bool saveSkill);

    //! \brief Shows/hides/toggles the options window
    bool showOptionsWindow(const CEGUI::EventArgs& = {});
    bool hideOptionsWindow(const CEGUI::EventArgs& = {});
    bool toggleOptionsWindow(const CEGUI::EventArgs& = {});

    //! \brief Refreshes the player current goals.
    void refreshPlayerGoals(const std::string& goalsDisplayString);

    //! \brief Refreshed the main ui data, such as mana, gold, ...
    void refreshMainUI();

    void selectSquaredTiles(int tileX1, int tileY1, int tileX2, int tileY2) override;
    void selectTiles(const std::vector<Tile*> tiles) override;
    void unselectAllTiles() override;

    void displayText(const Ogre::ColourValue& txtColour, const std::string& txt) override;

    //! \brief Called when the skill window is displayed. This function will call the Seat to get
    //! the current skill tree and update it as the player clicks on the skill buttons by calling
    //! skillButtonTreeClicked
    void resetSkillTree();
    //! \brief Called when the player clicks a skill in the skill tree. Note that resetSkillTree
    //! should be called before calling skillButtonTreeClicked
    //! Returns true if the pending list have been changed and false otherwise
    bool skillButtonTreeClicked(SkillType type);
    //! \brief Called when the player closes the skill tree. If apply is true, the changes
    //! should be sent to the server. If false, the changes should be canceled.
    void endSkillTree(bool apply);

    //! \brief Called at each frame. It checks if the Gui should be refreshed (for example,
    //! if a skill is done) and, if yes, refreshes accordingly.
    //! \param forceRefresh Refresh the gui even if no changes was declared by the local player Seat.
    void refreshGuiSkill(bool forceRefresh = false);

    //! \brief Called at each frame. Updates spell cooldowns.
    void refreshSpellButtonCoolDowns();

protected:
    bool onClickYesQuitMenu(const CEGUI::EventArgs& /*arg*/);

    //! \brief The different Game Options Menu handlers
    bool showQuitMenuFromOptions(const CEGUI::EventArgs& e = {});
    bool showObjectivesFromOptions(const CEGUI::EventArgs& e = {});
    bool showSkillFromOptions(const CEGUI::EventArgs& e = {});
    bool saveGame(const CEGUI::EventArgs& e = {});
    bool showSettingsFromOptions(const CEGUI::EventArgs& e = {});

    //! \brief Handle the keyboard input in normal mode
    virtual bool keyPressedNormal   (const OIS::KeyEvent &arg);

    //! \brief Handle the keyboard input when chat is activated
    virtual bool keyPressedChat     (const OIS::KeyEvent &arg);

    //! \brief Handle the keyboard input in normal mode
    virtual bool keyReleasedNormal  (const OIS::KeyEvent &arg);

private:
    //! \brief Sets whether a tile must marked or unmarked for digging.
    //! this value is based on the first marked flag tile selected.
    bool mDigSetBool;

    //! \brief Index of the event in the game event queue (for zooming automatically)
    uint32_t mIndexEvent;

    //! \brief The settings window.
    SettingsWindow mSettings;

    //! \brief Skills pending (Client side). This is copied from the seat for temporary changes while the
    //! player clicks on the skill tree window
    std::vector<SkillType> mSkillPending;

    SkillCurrentCompletion mSkillCurrentCompletion;

    bool mIsSkillWindowOpen;

    SkillType mCurrentSkillType;
    float mCurrentSkillProgress;

    //! \brief Seats playing the game
    std::vector<int> mSeatIds;

    MouseMoveEvent mPreviousMousePosition;

    //! \brief Set the help window (quite long) text.
    void setHelpWindowText();

    //! \brief A sub-function called by mouseMoved()
    //! It will handle the potential mouse wheel logic
    void handleMouseWheel(const MouseWheelEvent& arg);

    //! \brief Set the state of the given skill button accordingly to the skill type given.
    //! \note: Called by refreshGuiSkill() for each skillType.
    void refreshSkillButtonState(const std::string& skillButton, const std::string& castButton,
        const std::string& skillProgressBar, SkillType resType);

    //! \brief Tells whether the latest mouse click was made on a relevant CEGUI widget,
    //! and thus, the game should ignore it.
    bool isMouseDownOnCEGUIWindow();
    bool isMouseWheelOnCEGUIWindow();

     //! \brief Whether the keyboard keys moving camera are pressed down
    bool directionKeyPressed;
    const ConfigManager &config;
    
    //! \brief Called when there is a mouse input change
    void checkInputCommand();
    void handlePlayerActionNone();
    void handlePlayerActionSelectTile();

    //! \brief Builds the player settings window
    void buildPlayerSettingsWindow();

};

#endif // GAMEMODE_H
