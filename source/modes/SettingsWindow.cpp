/*!
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

#include "modes/SettingsWindow.h"

#include "utils/LogManager.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/widgets/Combobox.h>
#include <CEGUI/widgets/ToggleButton.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/WindowManager.h>

#include <OgreRoot.h>

#include <SFML/Audio/Listener.hpp>

SettingsWindow::SettingsWindow(CEGUI::Window* rootWindow):
    mSettingsWindow(nullptr),
    mRootWindow(rootWindow)
{
    if (rootWindow == nullptr)
    {
        OD_LOG_ERR("Settings Window loaded without any main CEGUI window!!");
        return;
    }
    // Create the window child.
    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

     mSettingsWindow = wmgr->loadLayoutFromFile("WindowSettings.layout");
    if (mSettingsWindow == nullptr)
    {
        OD_LOG_ERR("Couldn't load the Settings Window layout!!");
        return;
    }
    rootWindow->addChild(mSettingsWindow);
    mSettingsWindow->hide();

    // Events connections
    addEventConnection(
        mSettingsWindow->getChild("CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::cancelSettings, this)
        )
    );
    addEventConnection(
        mSettingsWindow->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::cancelSettings, this)
        )
    );
    addEventConnection(
        mSettingsWindow->getChild("ApplyButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::applySettings, this)
        )
    );

    initWidgetConfig();
}

SettingsWindow::~SettingsWindow()
{
    // Disconnects all event connections.
    for(CEGUI::Event::Connection& c : mEventConnections)
    {
        c->disconnect();
    }

    if (mSettingsWindow)
    {
        mSettingsWindow->hide();
        // Will be handled by the window manager. Don't do it.
        //mRootWindow->removeChild(mSettingsWindow->getID());
        CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
        wmgr->destroyWindow(mSettingsWindow);
    }
}

void SettingsWindow::initWidgetConfig()
{
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();
    Ogre::ConfigOptionMap& options = renderer->getConfigOptions();

    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");

    // Get the video settings.
    // Resolution
    Ogre::ConfigOptionMap::const_iterator it = options.find("Video Mode");
    if (it != options.end())
    {
        const Ogre::ConfigOption& video = it->second;
        CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/ResolutionCombobox"));
        resCb->setReadOnly(true);
        resCb->setSortingEnabled(true);
        uint32_t i = 0;
        for (std::string res : video.possibleValues)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(res, i);
            item->setSelectionBrushImage(selImg);
            resCb->addItem(item);

            if (res == video.currentValue)
            {
                resCb->setItemSelectState(item, true);
                resCb->setText(item->getText());
            }
            ++i;
        }
    }

    // Fullscreen
    it = options.find("Full Screen");
    if (it != options.end())
    {
        const Ogre::ConfigOption& fullscreen = it->second;
        CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/FullscreenCheckbox"));
        fsCheckBox->setSelected((fullscreen.currentValue == "Yes"));
    }

    // The current volume level
    float volume = sf::Listener::getGlobalVolume();
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/MusicSlider"));
    volumeSlider->setMaxValue(100);
    volumeSlider->setClickStep(10);
    volumeSlider->setCurrentValue(volume);
}

bool SettingsWindow::cancelSettings(const CEGUI::EventArgs&)
{
    initWidgetConfig();
    hide();
    return true;
}

bool SettingsWindow::applySettings(const CEGUI::EventArgs&)
{
    // Note: For now, only video settings will be stored thanks to Ogre config.
    // But later, we may want to centralize everything in a common config file,
    // And save volume values and other options.

    // Video - TODO: Apply without a restart.
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();

    CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/ResolutionCombobox"));
    renderer->setConfigOption("Video Mode", resCb->getSelectedItem()->getText().c_str());

    CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/FullscreenCheckbox"));
    renderer->setConfigOption("Full Screen", (fsCheckBox->isSelected() ? "Yes" : "No"));

    ogreRoot->saveConfig();

    // Audio - TODO: Save in config and reload at start.
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/MusicSlider"));
    sf::Listener::setGlobalVolume(volumeSlider->getCurrentValue());

    hide();
    return true;
}
