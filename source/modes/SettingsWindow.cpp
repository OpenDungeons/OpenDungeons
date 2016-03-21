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

    // Music volume slider
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
        mSettingsWindow->getChild("MainTabControl/Audio/AudioSP/MusicSlider"));
    addEventConnection(
        volumeSlider->subscribeEvent(
            CEGUI::Slider::EventValueChanged,
            CEGUI::Event::Subscriber(&SettingsWindow::onMusicVolumeChanged, this)
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
    ConfigManager& config = ConfigManager::getSingleton();

    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");

    // Game
    CEGUI::Editbox* nicknameEb = static_cast<CEGUI::Editbox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Game/GameSP/NicknameEdit"));
    std::string nickname = config.getGameValue(Config::NICKNAME, std::string(), false);
    if (!nickname.empty())
        nicknameEb->setText(reinterpret_cast<const CEGUI::utf8*>(nickname.c_str()));

    CEGUI::Combobox* keeperVoiceCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Game/GameSP/KeeperVoice"));
    keeperVoiceCb->resetList();
    std::string keeperVoice = config.getGameValue(Config::KEEPERVOICE, ConfigManager::DEFAULT_KEEPER_VOICE, false);
    uint32_t cptVoice = 0;
    for(const std::string& keeperVoiceAvailable : config.getKeeperVoices())
    {
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(keeperVoiceAvailable, cptVoice);
        item->setSelectionBrushImage(selImg);
        keeperVoiceCb->addItem(item);
        if(keeperVoiceAvailable == keeperVoice)
        {
            keeperVoiceCb->setText(item->getText());
            keeperVoiceCb->setItemSelectState(item, true);
        }
        ++cptVoice;
    }

    // Input
    CEGUI::ToggleButton* keyboardGrabCheckbox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Input/InputSP/KeyboardGrabCheckbox"));
    keyboardGrabCheckbox->setSelected(config.getInputValue(Config::KEYBOARD_GRAB, "No", false) == "Yes");

    CEGUI::ToggleButton* mouseGrabCheckbox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Input/InputSP/MouseGrabCheckbox"));
    mouseGrabCheckbox->setSelected(config.getInputValue(Config::MOUSE_GRAB, "No", false) == "Yes");

    // Audio
    std::string volumeStr = config.getAudioValue(Config::MUSIC_VOLUME, std::string(), false);
    float volume = volumeStr.empty() ? sf::Listener::getGlobalVolume() : Helper::toFloat(volumeStr);
    setMusicVolumeValue(volume);

    // Video
    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();
    Ogre::ConfigOptionMap& options = renderer->getConfigOptions();

    // Get the video settings.

    // Available renderers
    const Ogre::RenderSystemList& rdrList = ogreRoot->getAvailableRenderers();
    Ogre::RenderSystem* renderSystem = ogreRoot->getRenderSystem();

    CEGUI::Combobox* rdrCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/RendererCombobox"));
    rdrCb->setReadOnly(true);
    rdrCb->setSortingEnabled(true);
    rdrCb->resetList();
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

    // Resolution
    Ogre::ConfigOptionMap::const_iterator it = options.find(Config::VIDEO_MODE);
    if (it != options.end())
    {
        const Ogre::ConfigOption& video = it->second;
        CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/ResolutionCombobox"));
        resCb->setReadOnly(true);
        resCb->setSortingEnabled(true);
        resCb->resetList();
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

    it = options.find(Config::FULL_SCREEN);
    if (it != options.end())
    {
        const Ogre::ConfigOption& fullscreen = it->second;
        CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/FullscreenCheckbox"));
        fsCheckBox->setSelected((fullscreen.currentValue == "Yes"));
    }

    it = options.find(Config::VSYNC);
    if (it != options.end())
    {
        const Ogre::ConfigOption& vsync = it->second;
        CEGUI::ToggleButton* vsCheckBox = static_cast<CEGUI::ToggleButton*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/VSyncCheckbox"));
        vsCheckBox->setSelected((vsync.currentValue == "Yes"));
    }

    //! First of all, clear up the previously created windows.
    CEGUI::WindowManager& winMgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* parentWindow = mSettingsWindow->getChild("MainTabControl/Video/VideoSP/");
    for (CEGUI::Window* win : mCustomVideoComboBoxes)
    {
        parentWindow->removeChild(win);
        winMgr.destroyWindow(win);
    }
    mCustomVideoComboBoxes.clear();
    for (CEGUI::Window* win : mCustomVideoTexts)
    {
        parentWindow->removeChild(win);
        winMgr.destroyWindow(win);
    }
    mCustomVideoTexts.clear();

    // Find every other config options and add them to the config
    CEGUI::Window* videoTab = mSettingsWindow->getChild("MainTabControl/Video/VideoSP/");
    uint32_t offset = 0;
    for (std::pair<Ogre::String, Ogre::ConfigOption> option : options)
    {
        std::string optionName = option.first;
        // Skip the main options already set.
        if (optionName == Config::VSYNC || optionName == Config::FULL_SCREEN
            || optionName == Config::VIDEO_MODE)
            continue;

        Ogre::ConfigOption& config = option.second;
        // If the option is immutable, we can't change it and shouldn't see it. (at least for now)
        if (config.immutable || config.possibleValues.empty())
            continue;

        // The text next to the combobox
        CEGUI::DefaultWindow* videoCbText = static_cast<CEGUI::DefaultWindow*>(videoTab->createChild("OD/StaticText", optionName + "_Text"));
        videoCbText->setArea(CEGUI::UDim(0, 20), CEGUI::UDim(0, 155 + offset), CEGUI::UDim(0.4, 0), CEGUI::UDim(0, 30));
        videoCbText->setText(optionName + ": ");
        videoCbText->setProperty("FrameEnabled", "False");
        videoCbText->setProperty("BackgroundEnabled", "False");

        CEGUI::Combobox* videoCb = static_cast<CEGUI::Combobox*>(videoTab->createChild("OD/Combobox", optionName));
        videoCb->setArea(CEGUI::UDim(0.5, 0), CEGUI::UDim(0, 160 + offset), CEGUI::UDim(0.5, -20),
                         CEGUI::UDim(0, config.possibleValues.size() * 17 + 30));
        videoCb->setReadOnly(true);
        videoCb->setSortingEnabled(true);

        // Register the widgets for potential later deletion.
        mCustomVideoTexts.push_back(videoCbText);
        mCustomVideoComboBoxes.push_back(videoCb);

        // Fill the combobox with possible values.
        uint32_t cbIndex = 0;
        for(const std::string& value : config.possibleValues)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(value, cbIndex);
            item->setSelectionBrushImage(selImg);
            videoCb->addItem(item);
            // Set the combobox to the current value
            if(value == config.currentValue)
            {
                videoCb->setItemSelectState(item, true);
                videoCb->setText(item->getText());
            }
            ++cbIndex;
        }
        offset += 30;
    }
}

void SettingsWindow::saveConfig()
{
    // Note: For now, only video settings will be stored thanks to Ogre config.
    // But later, we may want to centralize everything in a common config file,
    // And save volume values and other options.

    Ogre::Root* ogreRoot = Ogre::Root::getSingletonPtr();
    ConfigManager& config = ConfigManager::getSingleton();

    // Save config
    // Game
    CEGUI::Editbox* usernameEb = static_cast<CEGUI::Editbox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Game/GameSP/NicknameEdit"));
    config.setGameValue(Config::NICKNAME, usernameEb->getText().c_str());

    CEGUI::Combobox* keeperVoiceCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Game/GameSP/KeeperVoice"));
    CEGUI::ListboxItem* keeperVoiceItem = keeperVoiceCb->getSelectedItem();
    if(keeperVoiceItem != nullptr)
        config.setGameValue(Config::KEEPERVOICE, keeperVoiceItem->getText().c_str());
    else
        config.setGameValue(Config::KEEPERVOICE, "");

    // Input
    CEGUI::ToggleButton* keyboardGrabCheckbox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Input/InputSP/KeyboardGrabCheckbox"));
    config.setInputValue(Config::KEYBOARD_GRAB, keyboardGrabCheckbox->isSelected() ? "Yes" : "No");

    CEGUI::ToggleButton* mouseGrabCheckbox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Input/InputSP/MouseGrabCheckbox"));
    config.setInputValue(Config::MOUSE_GRAB, mouseGrabCheckbox->isSelected() ? "Yes" : "No");

    // Audio
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/AudioSP/MusicSlider"));
    config.setAudioValue(Config::MUSIC_VOLUME, Helper::toString(volumeSlider->getCurrentValue()));

    // Video
    Ogre::RenderSystem* renderer = ogreRoot->getRenderSystem();

    // Changing Ogre renderer needs a restart to allow to load shaders and requested stuff
    CEGUI::Combobox* rdrCb = static_cast<CEGUI::Combobox*>(
    mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/RendererCombobox"));
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
        config.setVideoValue(Config::RENDERER, renderer->getName());
        config.saveUserConfig();

        // If render changed, we need to restart game.
        // Note that we do not change values according to the others inputs. The reason
        // is that we don't know if the given values are acceptable for the selected renderer
        OD_LOG_INF("Changed Ogre renderer to " + rendererName + ". We need to restart");
        exit(0);
    }
    config.setVideoValue(Config::RENDERER, renderer->getName());

    // Set renderer-dependent options now we know it didn't change.
    CEGUI::ToggleButton* fsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/FullscreenCheckbox"));
    renderer->setConfigOption(Config::FULL_SCREEN, (fsCheckBox->isSelected() ? "Yes" : "No"));
    config.setVideoValue(Config::FULL_SCREEN, fsCheckBox->isSelected() ? "Yes" : "No");

    CEGUI::Combobox* resCb = static_cast<CEGUI::Combobox*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/ResolutionCombobox"));
    renderer->setConfigOption(Config::VIDEO_MODE, resCb->getSelectedItem()->getText().c_str());
    config.setVideoValue(Config::VIDEO_MODE, resCb->getSelectedItem()->getText().c_str());

    // Stores the renderer dependent options
    CEGUI::ToggleButton* vsCheckBox = static_cast<CEGUI::ToggleButton*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/VSyncCheckbox"));
    renderer->setConfigOption(Config::VSYNC, (vsCheckBox->isSelected() ? "Yes" : "No"));
    config.setVideoValue(Config::VSYNC, fsCheckBox->isSelected() ? "Yes" : "No");

    // Save renderer dependent settings and apply them.
    for (CEGUI::Window* combo : mCustomVideoComboBoxes)
    {
        std::string optionName = combo->getName().c_str();
        std::string optionValue = combo->getText().c_str();
        renderer->setConfigOption(optionName, optionValue);
        config.setVideoValue(optionName, optionValue);
    }

    config.saveUserConfig();

    // Apply config

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
    mRootWindow->getChild("SettingsWindow/MainTabControl/Video/VideoSP/RendererCombobox"));
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

bool SettingsWindow::onMusicVolumeChanged(const CEGUI::EventArgs&)
{
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
        mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/AudioSP/MusicSlider"));
    setMusicVolumeValue(volumeSlider->getCurrentValue());
    return true;
}

void SettingsWindow::setMusicVolumeValue(float volume)
{
    sf::Listener::setGlobalVolume(volume);

    // Set the slider position
    CEGUI::Slider* volumeSlider = static_cast<CEGUI::Slider*>(
            mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/AudioSP/MusicSlider"));
    volumeSlider->setCurrentValue(volume);

    // Set the music volume text
    CEGUI::Window* volumeText = mRootWindow->getChild("SettingsWindow/MainTabControl/Audio/AudioSP/MusicText");
    volumeText->setText("Music: " + Helper::toString(static_cast<int32_t>(volume)) + "%");
}
