/*!
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

#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <CEGUI/Event.h>
#include <CEGUI/Window.h>

#include <vector>

//! \brief This class is creating a setting window gui child to the current gui context
//! and populates its widgets with the current game, video, audio values.
//! It also permits to change them.
class SettingsWindow
{
public:
    //! \brief Settings window constructor
    //! \param rootWindow The main CEGUI window used as background to the current mode.
    //! Used to load and later show the settings window.
    SettingsWindow(CEGUI::Window* rootWindow);

    ~SettingsWindow();

    void show();

    void hide();

    bool isVisible() const
    {
        if (mSettingsWindow)
            return mSettingsWindow->isVisible();
        return false;
    }

    //! \brief Called when pushing the cancel button on the settings window.
    bool onCancelSettings(const CEGUI::EventArgs& e = {});

private:
    //! \brief Vector of cegui event bindings to be cleared on exiting the mode
    std::vector<CEGUI::Event::Connection> mEventConnections;

    //! \brief The Settings window.
    CEGUI::Window* mSettingsWindow;

    //! \brief The apply change pop-up
    CEGUI::Window* mApplyWindow;

    //! \brief The root window.
    CEGUI::Window* mRootWindow;

    //! \brief The temporary video comboboxes and texts created depending on the video settings.
    std::vector<CEGUI::Window*> mCustomVideoComboBoxes;
    std::vector<CEGUI::Window*> mCustomVideoTexts;

    //! \brief Set the different widget values according to current config.
    void initConfig();

    //! \brief Save the config, potentially stopping the application if it needs to.
    void saveConfig();

    //! \brief Adds an event binding to be cleared on exiting the mode.
    inline void addEventConnection(CEGUI::Event::Connection conn)
    {
        mEventConnections.emplace_back(conn);
    }

    //! \brief Called when pushing the apply button on the settings window.
    bool onApplySettings(const CEGUI::EventArgs&);

    //! \brief Called when pushing the cancel button on the change appliance popup.
    bool onPopupCancelApplySettings(const CEGUI::EventArgs&);

    //! \brief Called when pushing the apply button on the change appliance popup.
    bool onPopupApplySettings(const CEGUI::EventArgs&);

    //! \brief Called when changing the music volume value.
    bool onMusicVolumeChanged(const CEGUI::EventArgs&);

    //! \brief Set the volume value in the volume setting text and slider.
    void setMusicVolumeValue(float volume);

    //! \brief Called when changing the ambient light factor value.
    bool onLightFactorChanged(const CEGUI::EventArgs&);

    //! \brief Set the volume value in the ambient light factor setting text and slider.
    void setLightFactorValue(float lightFactor);
};

#endif // SETTINGSWINDOW_H
