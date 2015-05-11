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

#include "gamemap/GameMap.h"

#include "game/Seat.h"

#include "modes/MenuModeConfigureSeats.h"
#include "modes/ModeManager.h"

#include "render/Gui.h"
#include "render/ODFrameListener.h"

#include "network/ODServer.h"
#include "network/ODClient.h"

#include "sound/MusicPlayer.h"

#include "utils/ConfigManager.h"
#include "utils/LogManager.h"
#include "utils/Helper.h"

#include <CEGUI/CEGUI.h>

#include <boost/filesystem.hpp>

const std::string TEXT_SEAT_ID_PREFIX = "TextSeat";
const std::string COMBOBOX_TEAM_ID_PREFIX = "ComboTeam";
const std::string COMBOBOX_PLAYER_FACTION_PREFIX = "ComboPlayerFactionSeat";
const std::string COMBOBOX_PLAYER_PREFIX = "ComboPlayerSeat";

MenuModeConfigureSeats::MenuModeConfigureSeats(ModeManager* modeManager):
    AbstractApplicationMode(modeManager, ModeManager::MENU_CONFIGURE_SEATS),
    mIsActivePlayerConfig(false)
{
    CEGUI::Window* window = modeManager->getGui().getGuiSheet(Gui::guiSheet::configureSeats);
    addEventConnection(
        window->getChild(Gui::CSM_BUTTON_LAUNCH)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::launchSelectedButtonPressed, this)
        )
    );
    addEventConnection(
        window->getChild(Gui::CSM_BUTTON_BACK)->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::goBack, this)
        )
    );

    addEventConnection(
        window->getChild("ListPlayers/__auto_closebutton__")->subscribeEvent(
            CEGUI::PushButton::EventClicked,
            CEGUI::Event::Subscriber(&MenuModeConfigureSeats::goBack, this)
        )
    );
}

MenuModeConfigureSeats::~MenuModeConfigureSeats()
{
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    for(int seatId : mSeatIds)
    {
        std::string name;
        name = TEXT_SEAT_ID_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        tmpWin->destroyChild(name);
    }
}

void MenuModeConfigureSeats::activate()
{
    // Loads the corresponding Gui sheet.
    getModeManager().getGui().loadGuiSheet(Gui::guiSheet::configureSeats);

    giveFocus();

    // Play the main menu music
    // TODO: Make this configurable.
    MusicPlayer::getSingleton().play("OpenDungeonsMainTheme_pZi.ogg");

    // We use the client game map to allow everybody to see how the server is configuring seats
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    gameMap->setGamePaused(true);

    CEGUI::WindowManager& winMgr = CEGUI::WindowManager::getSingleton();
    CEGUI::Window* tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    CEGUI::Window* msgWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("LoadingText");
    msgWin->setText("Loading...");
    msgWin->setVisible(false);

    tmpWin->setText(reinterpret_cast<const CEGUI::utf8*>(std::string("Configure map : " + gameMap->getLevelName()).c_str()));

    const std::vector<std::string>& factions = ConfigManager::getSingleton().getFactions();
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    const std::vector<Seat*>& seats = gameMap->getSeats();

    ServerMode serverMode = ODServer::getSingleton().getServerMode();
    int offset = 0;
    int bestSeatHumanSeatId = -1;
    // We start by searching for the most suitable seat for local player. If we find a seat
    // for human only, it is the one. If not, we use the first a human can use
    for(Seat* seat : seats)
    {
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_HUMAN) == 0)
        {
            bestSeatHumanSeatId = seat->getId();
            break;
        }
        else if((bestSeatHumanSeatId == -1) &&
                (seat->getPlayerType().compare(Seat::PLAYER_TYPE_CHOICE) == 0))
        {
            bestSeatHumanSeatId = seat->getId();
        }
    }

    bool enabled = false;

    for(Seat* seat : seats)
    {
        // We do not add the rogue creatures seat
        if(seat->isRogueSeat())
            continue;

        mSeatIds.push_back(seat->getId());
        std::string name;
        CEGUI::Combobox* combo;

        name = TEXT_SEAT_ID_PREFIX + Ogre::StringConverter::toString(seat->getId());
        CEGUI::DefaultWindow* textSeatId = static_cast<CEGUI::DefaultWindow*>(winMgr.createWindow("OD/StaticText", name));
        tmpWin->addChild(textSeatId);
        Ogre::ColourValue seatColor = seat->getColorValue();
        seatColor.a = 1.0f; // Restore the color opacity
        textSeatId->setArea(CEGUI::UDim(0,20), CEGUI::UDim(0,65 + offset), CEGUI::UDim(0.3,0), CEGUI::UDim(0,30));
        textSeatId->setText("[colour='" + Helper::getCEGUIColorFromOgreColourValue(seatColor) + "']Seat "  + Ogre::StringConverter::toString(seat->getId()));
        textSeatId->setProperty("FrameEnabled", "False");
        textSeatId->setProperty("BackgroundEnabled", "False");;

        name = COMBOBOX_PLAYER_FACTION_PREFIX + Ogre::StringConverter::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(0,100), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0.3,0), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        if(seat->getFaction().compare(Seat::PLAYER_FACTION_CHOICE) == 0)
        {
            uint32_t cptFaction = 0;
            for(const std::string& faction : factions)
            {
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(faction, cptFaction);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
                // At creation, we set the combo to the first available choice
                if(cptFaction == 0)
                {
                    combo->setItemSelectState(item, true);
                    combo->setText(item->getText());
                }
                ++cptFaction;
            }
        }
        else
        {
            uint32_t cptFaction = 0;
            for(const std::string& faction : factions)
            {
                if(seat->getFaction().compare(faction) == 0)
                    break;

                ++cptFaction;
            }
            // If the faction is not found, we set it to the first defined
            if(cptFaction >= factions.size())
                cptFaction = 0;

            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(factions[cptFaction], cptFaction);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        name = COMBOBOX_PLAYER_PREFIX + Ogre::StringConverter::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(0.5,10), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0.3,0), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(seat->getPlayerType(), 0);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(seat->getPlayerType(), 1);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        else if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_CHOICE) == 0)
        {
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Seat::PLAYER_TYPE_AI, 1);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);

            // If we are not in multiplayer mode, we set all the players to AI except the best we could find
            // The unset players will be set when a player connects
            // Note that when loading a game, this is useless because no seat with choice should be possible
            switch(serverMode)
            {
                case ServerMode::ModeGameMultiPlayer:
                case ServerMode::ModeGameLoaded:
                    break;
                default:
                    if(seat->getId() != bestSeatHumanSeatId)
                    {
                        combo->setItemSelectState(item, true);
                        combo->setText(item->getText());
                    }
                    break;
            }
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        name = COMBOBOX_TEAM_ID_PREFIX + Ogre::StringConverter::toString(seat->getId());
        combo = static_cast<CEGUI::Combobox*>(winMgr.createWindow("OD/Combobox", name));
        tmpWin->addChild(combo);
        combo->setArea(CEGUI::UDim(1,-80), CEGUI::UDim(0,70 + offset), CEGUI::UDim(0,60), CEGUI::UDim(0,200));
        combo->setReadOnly(true);
        combo->setEnabled(enabled);
        combo->setSortingEnabled(true);
        const std::vector<int>& availableTeamIds = seat->getAvailableTeamIds();
        OD_ASSERT_TRUE_MSG(!availableTeamIds.empty(), "Empty availableTeamIds for seat id="
            + Ogre::StringConverter::toString(seat->getId()));
        if(availableTeamIds.size() > 1)
        {
            uint32_t cptTeamId = 0;
            for(int teamId : availableTeamIds)
            {
                CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Ogre::StringConverter::toString(teamId), teamId);
                item->setSelectionBrushImage(selImg);
                combo->addItem(item);
                // At creation, we set the combo to the first available choice
                if(cptTeamId == 0)
                {
                    combo->setItemSelectState(item, true);
                    combo->setText(item->getText());
                }
                ++cptTeamId;
            }
        }
        else if(!availableTeamIds.empty())
        {
            int teamId = availableTeamIds[0];
            CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(Ogre::StringConverter::toString(teamId), teamId);
            item->setSelectionBrushImage(selImg);
            combo->addItem(item);
            combo->setText(item->getText());
            combo->setEnabled(false);
        }
        combo->subscribeEvent(CEGUI::Combobox::EventListSelectionAccepted, CEGUI::SubscriberSlot(&MenuModeConfigureSeats::comboChanged, this));

        offset += 30;
    }

    tmpWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers/LaunchGameButton");
    tmpWin->setEnabled(enabled);

    // We notify the server we are ready to recceive players and configure them
    ODClient::getSingleton().queueClientNotification(ClientNotificationType::readyForSeatConfiguration);
}

bool MenuModeConfigureSeats::launchSelectedButtonPressed(const CEGUI::EventArgs&)
{
    // We send to the server the associations faction/seat/player
    // It will be responsible to disconnect the unselected players
    if(!mIsActivePlayerConfig)
        return true;

    fireSeatConfigurationToServer(true);
    return true;
}

bool MenuModeConfigureSeats::goBack(const CEGUI::EventArgs&)
{
    // We disconnect client and, if we are server, the server
    ODClient::getSingleton().disconnect();
    if(ODServer::getSingleton().isConnected())
    {
        ODServer::getSingleton().stopServer();
    }

    getModeManager().requestPreviousMode();
    return true;
}

bool MenuModeConfigureSeats::comboChanged(const CEGUI::EventArgs& ea)
{
    if(!mIsActivePlayerConfig)
        return true;

    // If the combo changed is a player and he was already in another combo, we remove him from the combo
    CEGUI::Combobox* comboSel = static_cast<CEGUI::Combobox*>(static_cast<const CEGUI::WindowEventArgs&>(ea).window);
    CEGUI::ListboxItem* selItem = comboSel->getSelectedItem();
    if((selItem != nullptr) &&
       (comboSel->getName().compare(0, COMBOBOX_PLAYER_PREFIX.length(), COMBOBOX_PLAYER_PREFIX) == 0) &&
       (selItem->getID() != 0) && // Can be several inactive players
       (selItem->getID() != 1)) // Can be several AI players
    {
        GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
        CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
        for(int seatId : mSeatIds)
        {
            Seat* seat = gameMap->getSeatById(seatId);
            // We only add players to combos where a human player can play
            if((seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0) ||
               (seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0))
            {
                continue;
            }

            std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
            CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            if(combo == comboSel)
                continue;

            CEGUI::ListboxItem* item = combo->getSelectedItem();
            if(item == nullptr)
                continue;

            if(item->getID() != selItem->getID())
                continue;

            combo->setItemSelectState(item, false);
            combo->setText("");
        }
    }

    fireSeatConfigurationToServer(false);
    return true;
}

void MenuModeConfigureSeats::addPlayer(const std::string& nick, int32_t id)
{
    const CEGUI::Image* selImg = &CEGUI::ImageManager::getSingleton().get("OpenDungeonsSkin/SelectionBrush");
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    bool isPlayerSet = false;

    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    mPlayers.push_back(std::pair<std::string, int32_t>(nick, id));
    for(int seatId : mSeatIds)
    {
        Seat* seat = gameMap->getSeatById(seatId);
        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) == 0)
            continue;

        if(seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) == 0)
            continue;

        std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(reinterpret_cast<const CEGUI::utf8*>(nick.c_str()), id);
        item->setSelectionBrushImage(selImg);
        combo->addItem(item);

        // If a combo is empty, we set it to the player nick
        if(!isPlayerSet && (combo->getSelectedItem() == nullptr))
        {
            combo->setItemSelectState(item, true);
            combo->setText(item->getText());
            isPlayerSet = true;
        }
    }

    if(!mIsActivePlayerConfig)
        return;

    fireSeatConfigurationToServer(false);
}

void MenuModeConfigureSeats::removePlayer(int32_t id)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    for(std::vector<std::pair<std::string, int32_t> >::iterator it = mPlayers.begin(); it != mPlayers.end();)
    {
        std::pair<std::string, int32_t>& player = *it;
        if(player.second != id)
        {
            ++it;
            continue;
        }

        mPlayers.erase(it);
        for(int seatId : mSeatIds)
        {
            std::string name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
            CEGUI::Combobox* combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            for(uint32_t i = 0; i < combo->getItemCount();)
            {
                CEGUI::ListboxItem* selItem = combo->getListboxItemFromIndex(i);
                if(selItem->getID() == static_cast<CEGUI::uint>(id))
                {
                    if(selItem->isSelected())
                        combo->setText("");

                    combo->removeItem(selItem);
                }
                else
                    ++i;
            }
        }
        break;
    }

    if(!mIsActivePlayerConfig)
        return;

    fireSeatConfigurationToServer(false);
}

void MenuModeConfigureSeats::fireSeatConfigurationToServer(bool isFinal)
{
    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    CEGUI::Combobox* combo;
    CEGUI::ListboxItem*	selItem;
    if(isFinal)
    {
        // We start by checking that every seat is well configured.
        for(int seatId : mSeatIds)
        {
            std::string name;
            name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
            combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            selItem = combo->getSelectedItem();
            if(selItem == nullptr)
            {
                CEGUI::Window* msgWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("LoadingText");
                msgWin->setText("Faction is not well configured for seat " + Helper::toString(seatId));
                msgWin->setVisible(true);
                return;
            }
            name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
            combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            selItem = combo->getSelectedItem();
            if(selItem == nullptr)
            {
                CEGUI::Window* msgWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("LoadingText");
                msgWin->setText("Player is not well configured for seat " + Helper::toString(seatId));
                msgWin->setVisible(true);
                return;
            }
            name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
            combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
            selItem = combo->getSelectedItem();
            if(selItem == nullptr)
            {
                CEGUI::Window* msgWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("LoadingText");
                msgWin->setText("Player is not well configured for seat " + Helper::toString(seatId));
                msgWin->setVisible(true);
                return;
            }
        }
    }

    ClientNotification* notif;
    if(isFinal)
        notif = new ClientNotification(ClientNotificationType::seatConfigurationSet);
    else
        notif = new ClientNotification(ClientNotificationType::seatConfigurationRefresh);

    for(int seatId : mSeatIds)
    {
        std::string name;
        notif->mPacket << seatId;
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            uint32_t factionIndex = selItem->getID();
            notif->mPacket << true << factionIndex;
        }
        else
        {
            notif->mPacket << false;
        }

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            int32_t playerId = selItem->getID();
            notif->mPacket << true << playerId;
        }
        else
        {
            notif->mPacket << false;
        }

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        selItem = combo->getSelectedItem();
        if(selItem != nullptr)
        {
            int32_t teamId = selItem->getID();
            notif->mPacket << true << teamId;
        }
        else
        {
            notif->mPacket << false;
        }
    }
    ODClient::getSingleton().queueClientNotification(notif);
}

void MenuModeConfigureSeats::activatePlayerConfig()
{
    mIsActivePlayerConfig = true;
    GameMap* gameMap = ODFrameListener::getSingleton().getClientGameMap();
    CEGUI::Window* listPlayersWindow = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    listPlayersWindow->setText("Please configure map : " + gameMap->getLevelName());
    bool enabled = true;

    for(int seatId : mSeatIds)
    {
        Seat* seat = gameMap->getSeatById(seatId);

        std::string name;
        CEGUI::Combobox* combo;

        name = COMBOBOX_PLAYER_FACTION_PREFIX + Ogre::StringConverter::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(combo->getItemCount() > 1)
            combo->setEnabled(enabled);

        name = COMBOBOX_PLAYER_PREFIX + Ogre::StringConverter::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(enabled)
        {
            if((seat->getPlayerType().compare(Seat::PLAYER_TYPE_INACTIVE) != 0) &&
               (seat->getPlayerType().compare(Seat::PLAYER_TYPE_AI) != 0))
            {
                combo->setEnabled(enabled);
            }
        }
        else
            combo->setEnabled(enabled);

        name = COMBOBOX_TEAM_ID_PREFIX + Ogre::StringConverter::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(listPlayersWindow->getChild(name));
        if(combo->getItemCount() > 1)
            combo->setEnabled(enabled);
    }

    CEGUI::Window* startButton = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers/LaunchGameButton");
    startButton->setEnabled(enabled);
}

void MenuModeConfigureSeats::refreshSeatConfiguration(ODPacket& packet)
{
    // We don't refresh seats for the player configuring the game
    if(mIsActivePlayerConfig)
        return;

    CEGUI::Window* playersWin = getModeManager().getGui().getGuiSheet(Gui::guiSheet::configureSeats)->getChild("ListPlayers");
    CEGUI::Combobox* combo;
    bool isSelected;
    for(int seatId : mSeatIds)
    {
        int seatIdPacket;
        OD_ASSERT_TRUE(packet >> seatIdPacket);
        OD_ASSERT_TRUE(seatId == seatIdPacket);
        std::string name;
        name = COMBOBOX_PLAYER_FACTION_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        uint32_t factionIndex = 0;
        if(isSelected)
        {
            OD_ASSERT_TRUE(packet >> factionIndex);
        }
        for(uint32_t i = 0; i < combo->getItemCount(); ++i)
        {
            CEGUI::ListboxItem* selItem = combo->getListboxItemFromIndex(i);
            if(isSelected && selItem->getID() == factionIndex)
            {
                combo->setItemSelectState(selItem, true);
                combo->setText(selItem->getText());
            }
            else
                combo->setItemSelectState(selItem, false);
        }

        name = COMBOBOX_PLAYER_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        int32_t playerId = 0;
        if(isSelected)
        {
            OD_ASSERT_TRUE(packet >> playerId);
        }
        for(uint32_t i = 0; i < combo->getItemCount(); ++i)
        {
            CEGUI::ListboxItem* selItem = combo->getListboxItemFromIndex(i);
            if(isSelected && selItem->getID() == static_cast<uint32_t>(playerId))
            {
                combo->setItemSelectState(selItem, true);
                combo->setText(selItem->getText());
            }
            else
                combo->setItemSelectState(selItem, false);
        }

        name = COMBOBOX_TEAM_ID_PREFIX + Helper::toString(seatId);
        combo = static_cast<CEGUI::Combobox*>(playersWin->getChild(name));
        OD_ASSERT_TRUE(packet >> isSelected);
        int32_t teamId = 0;
        if(isSelected)
        {
            OD_ASSERT_TRUE(packet >> teamId);
        }
        for(uint32_t i = 0; i < combo->getItemCount(); ++i)
        {
            CEGUI::ListboxItem* selItem = combo->getListboxItemFromIndex(i);
            if(isSelected && selItem->getID() == static_cast<uint32_t>(teamId))
            {
                combo->setItemSelectState(selItem, true);
                combo->setText(selItem->getText());
            }
            else
                combo->setItemSelectState(selItem, false);
        }
    }
}

