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
#include "utils/Helper.h"

#include <CEGUI/CEGUI.h>
#include <CEGUI/widgets/Combobox.h>
#include <CEGUI/widgets/ToggleButton.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/WindowManager.h>

#include <OgreRoot.h>
#include <OgreRenderWindow.h>

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

    initConfig();
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

void SettingsWindow::initConfig()
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

    // Available renderers
    const Ogre::RenderSystemList& rdrList = ogreRoot->getAvailableRenderers();
    Ogre::RenderSystem* renderSystem = ogreRoot->getRenderSystem();

    CEGUI::Combobox* rdrCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/RendererCombobox"));
    rdrCb->setReadOnly(true);
    rdrCb->setSortingEnabled(true);
    uint32_t i = 0;
    for (Ogre::RenderSystem* rdr : rdrList)
    {
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(rdr->getName(), i);
        item->setSelectionBrushImage(selImg);
        rdrCb->addItem(item);

        if (rdr == renderSystem)
        {
            rdrCb->setItemSelectState(item, true);
            rdrCb->setText(item->getText());
        }
        ++i;
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

    // VSync
    it = options.find("VSync");
    if (it != options.end())
    {
        const Ogre::ConfigOption& vsync = it->second;
        CEGUI::ToggleButton* vsCheckBox = static_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VSyncCheckbox"));
        vsCheckBox->setSelected((vsync.currentValue == "Yes"));
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
    initConfig();
    hide();
    return true;
}

bool SettingsWindow::applySettings(const CEGUI::EventArgs&)
{
    // Note: For now, only video settings will be stored thanks to Ogre config.
    // But later, we may want to centralize everything in a common config file,
    // And save volume values and other options.

    // Video
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();

    // Changing Ogre renderer needs a restart to allow to load shaders and requested stuff
    CEGUI::Combobox* rdrCb = static_cast<CEGUI::Combobox*>(
    mRootWindow->getChild("SettingsWindow/MainTabControl/Video/RendererCombobox"));
    std::string rendererName = rdrCb->getSelectedItem()->getText().c_str();
    if (rendererName != renderer->getName())
    {
        renderer = ogreRoot->getRenderSystemByName(rendererName);
        if (renderer == nullptr)
        {
            const Ogre::RenderSystemList& renderers = ogreRoot->getAvailableRenderers();
            if (renderers.empty())
            {
                OD_LOG_ERR("No valid renderer found while searching for " + std::string(rdrCb->getSelectedItem()->getText().c_str()));
                return false;
            }
            renderer = *renderers.begin();
            OD_LOG_WRN("Wanted renderer : " + std::string(rdrCb->getSelectedItem()->getText().c_str()) + " not found. Using the first available: " + renderer->getName());
        }
        ogreRoot->setRenderSystem(renderer);
        ogreRoot->saveConfig();
        // If render changed, we need to restart game.
        // Note that we do not change values according to the others inputs. The reason
        // is that we don't know if the given values are acceptable for the selected renderer
        OD_LOG_INF("Changed Ogre renderer to " + rendererName + ". We need to restart");
        exit(0);
    }

    CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/ResolutionCombobox"));
    renderer->setConfigOption("Video Mode", resCb->getSelectedItem()->getText().c_str());

    CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/FullscreenCheckbox"));
    renderer->setConfigOption("Full Screen", (fsCheckBox->isSelected() ? "Yes" : "No"));

    CEGUI::ToggleButton* vsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VSyncCheckbox"));
    renderer->setConfigOption("VSync", (vsCheckBox->isSelected() ? "Yes" : "No"));

    ogreRoot->saveConfig();

    // Apply config
    Ogre::RenderWindow* win = ogreRoot->getAutoCreatedWindow();
    std::vector<std::string> resVtr = Helper::split(resCb->getSelectedItem()->getText().c_str(), 'x');
    if (resVtr.size() == 2)
    {
        uint32_t width = static_cast<uint32_t>(Helper::toInt(resVtr[0]));
        uint32_t height = static_cast<uint32_t>(Helper::toInt(resVtr[1]));
        win->setFullscreen(fsCheckBox->isSelected(), width, height);

        // In windowed mode, the window needs resizing through other means
        // NOTE: Doesn't work when the window is maximized on certain Composers.
        if (!fsCheckBox->isSelected())
            win->resize(width, height);
    }

    // Audio - TODO: Save in config and reload at start.
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/MusicSlider"));
    sf::Listener::setGlobalVolume(volumeSlider->getCurrentValue());

    hide();
    return true;
}
