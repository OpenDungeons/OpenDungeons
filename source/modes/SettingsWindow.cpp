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

#include "utils/ConfigManager.h"
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
    mApplyWindow(nullptr),
    mRootWindow(rootWindow)
{
    if (rootWindow == nullptr)
    {
        OD_LOG_ERR("Settings Window loaded without any main CEGUI window!!");
        return;
    }
    CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();

    mSettingsWindow = wmgr->loadLayoutFromFile("WindowSettings.layout");
    if (mSettingsWindow == nullptr)
    {
        OD_LOG_ERR("Couldn't load the Settings Window layout!!");
        return;
    }
    rootWindow->addChild(mSettingsWindow);
    mSettingsWindow->hide();

    mApplyWindow = wmgr->loadLayoutFromFile("WindowApplyChanges.layout");
    if (mApplyWindow == nullptr)
    {
        OD_LOG_ERR("Couldn't load the apply changes popup Window layout!!");
        return;
    }
    rootWindow->addChild(mApplyWindow);
    mApplyWindow->hide();

    // Events connections
    // Settings window
    addEventConnection(
        mSettingsWindow->getChild("CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onCancelSettings, this)
        )
    );
    addEventConnection(
        mSettingsWindow->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onCancelSettings, this)
        )
    );
    addEventConnection(
        mSettingsWindow->getChild("ApplyButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onApplySettings, this)
        )
    );

    // Apply Pop-up
    addEventConnection(
        mApplyWindow->getChild("CancelButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onPopupCancelApplySettings, this)
        )
    );
    addEventConnection(
        mApplyWindow->getChild("__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onPopupCancelApplySettings, this)
        )
    );
    addEventConnection(
        mApplyWindow->getChild("ApplyButton")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&SettingsWindow::onPopupApplySettings, this)
        )
    );

    initConfig();
}

SettingsWindow::~SettingsWindow()
{
    // Disconnects all event connections.
    for(CEGUI::Event::Connection& c : mEventConnections)
        c->disconnect();

    if (mSettingsWindow)
    {
        mSettingsWindow->hide();
        mSettingsWindow->setModalState(false);
        // Will be handled by the window manager. Don't do it.
        //mRootWindow->removeChild(mSettingsWindow->getID());
        CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
        wmgr->destroyWindow(mSettingsWindow);
    }

    if (mApplyWindow)
    {
        mApplyWindow->hide();
        mApplyWindow->setModalState(false);
        // Will be handled by the window manager. Don't do it.
        //mRootWindow->removeChild(mApplyWindow->getID());
        CEGUI::WindowManager* wmgr = CEGUI::WindowManager::getSingletonPtr();
        wmgr->destroyWindow(mApplyWindow);
    }
}

void SettingsWindow::initConfig()
{
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();
    Ogre::ConfigOptionMap& options = renderer->getConfigOptions();

    ConfigManager& config = ConfigManager::getSingleton();

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
    std::string volumeStr = config.getAudioValue("MusicVolume");
    float volume = volumeStr.empty() ? sf::Listener::getGlobalVolume() : Helper::toFloat(volumeStr);

    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/MusicSlider"));
    volumeSlider->setMaxValue(100);
    volumeSlider->setClickStep(10);
    volumeSlider->setCurrentValue(volume);
}

void SettingsWindow::saveConfig()
{
    // Note: For now, only video settings will be stored thanks to Ogre config.
    // But later, we may want to centralize everything in a common config file,
    // And save volume values and other options.

    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    ConfigManager& config = ConfigManager::getSingleton();

    // Save config
    // Audio
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/MusicSlider"));
    config.setAudioValue("MusicVolume", Helper::toString(volumeSlider->getCurrentValue()));

    // Video
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
                return;
            }
            renderer = *renderers.begin();
            OD_LOG_WRN("Wanted renderer : " + std::string(rdrCb->getSelectedItem()->getText().c_str()) + " not found. Using the first available: " + renderer->getName());
        }
        ogreRoot->setRenderSystem(renderer);
        //config.setVideoValue("Renderer", renderer->getName());
        ogreRoot->saveConfig();
        // If render changed, we need to restart game.
        // Note that we do not change values according to the others inputs. The reason
        // is that we don't know if the given values are acceptable for the selected renderer
        OD_LOG_INF("Changed Ogre renderer to " + rendererName + ". We need to restart");
        exit(0);
    }

    // Set renderer-dependent options now we know it didn't change.
    CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/FullscreenCheckbox"));
    renderer->setConfigOption("Full Screen", (fsCheckBox->isSelected() ? "Yes" : "No"));
    config.setVideoValue("FullScreen", fsCheckBox->isSelected() ? "Yes" : "No");

    CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/ResolutionCombobox"));
    renderer->setConfigOption("Video Mode", resCb->getSelectedItem()->getText().c_str());
    //config.setVideoValue("VideoMode", resCb->getSelectedItem()->getText().c_str());

    // Stores the renderer dependent options
    CEGUI::ToggleButton* vsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VSyncCheckbox"));
    renderer->setConfigOption("VSync", (vsCheckBox->isSelected() ? "Yes" : "No"));
    config.setVideoValue("VSync", fsCheckBox->isSelected() ? "Yes" : "No");

    ogreRoot->saveConfig();
    // TODO: Only save and handle custom config. Drop ogre config if possible.
    config.saveUserConfig();

    // Apply config

    // Audio
    sf::Listener::setGlobalVolume(volumeSlider->getCurrentValue());

    // Video
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
}

void SettingsWindow::show()
{
    if (mSettingsWindow)
    {
        // Input only allowed on this window when visible.
        mSettingsWindow->setModalState(true);
        mSettingsWindow->show();
    }
}

void SettingsWindow::hide()
{
    if (mSettingsWindow)
    {
        mSettingsWindow->setModalState(false);
        mSettingsWindow->hide();
    }
    if (mApplyWindow)
    {
        mApplyWindow->setModalState(false);
        mApplyWindow->hide();
    }
}

bool SettingsWindow::onCancelSettings(const CEGUI::EventArgs&)
{
    initConfig();
    hide();
    return true;
}

bool SettingsWindow::onApplySettings(const CEGUI::EventArgs&)
{
    // Check the renderer change and open the pop-up if needed.
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* currentRenderer = ogreRoot->getRenderSystem();

    // Changing Ogre renderer needs a restart to allow to load shaders and requested stuff
    CEGUI::Combobox* rdrCb = static_cast<CEGUI::Combobox*>(
    mRootWindow->getChild("SettingsWindow/MainTabControl/Video/RendererCombobox"));
    std::string rendererName = rdrCb->getSelectedItem()->getText().c_str();
    if (rendererName != currentRenderer->getName())
    {
        Ogre::RenderSystem* newRenderer = ogreRoot->getRenderSystemByName(rendererName);
        if (newRenderer == nullptr)
        {
            OD_LOG_ERR("No valid renderer found while searching for: "
                        + std::string(rdrCb->getSelectedItem()->getText().c_str())
                        + ". Restoring the previous value: " + std::string(currentRenderer->getName().c_str()));

            for (size_t i = 0; i < rdrCb->getItemCount(); ++i)
            {
                CEGUI::ListboxTextItem* item = static_cast<CEGUI::ListboxTextItem*>(rdrCb->getListboxItemFromIndex(i));

                if (item->getText() == std::string(currentRenderer->getName().c_str()))
                {
                    rdrCb->setItemSelectState(item, true);
                    rdrCb->setText(item->getText());
                    break;
                }
                ++i;
            }
            return true;
        }

        // If render changed, we need to restart game.
        mApplyWindow->show();
        // Input only allowed on this window when visible.
        mApplyWindow->setModalState(true);
        return true;
    }

    saveConfig();
    hide();
    return true;
}

bool SettingsWindow::onPopupCancelApplySettings(const CEGUI::EventArgs&)
{
    mApplyWindow->hide();
    mApplyWindow->setModalState(false);
    // Restore main settings window modal state
    mSettingsWindow->setModalState(true);
    return true;
}

bool SettingsWindow::onPopupApplySettings(const CEGUI::EventArgs&)
{
    hide();
    saveConfig();
    // Should restart right after that.
    return true;
}
