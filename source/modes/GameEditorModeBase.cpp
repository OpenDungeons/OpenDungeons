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

#include "GameEditorModeBase.h"

#include "ConsoleMode.h"

#include "gamemap/GameMap.h"
#include "network/ChatEventMessage.h"
#include "render/Gui.h"
#include "render/ODFrameListener.h"
#include "rooms/RoomType.h"
#include "traps/TrapType.h"
#include "utils/Helper.h"
#include "utils/MakeUnique.h"

#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Scrollbar.h>

namespace {
    //Functors for binding gui actions to spells/room/trap selection.
    class RoomSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(SelectedAction::buildRoom);
            playerSelection.setNewRoomType(roomType);
            return true;
        }
        RoomType roomType;
        PlayerSelection& playerSelection;
    };
    class TrapSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(SelectedAction::buildTrap);
            playerSelection.setNewTrapType(trapType);
            return true;
        }
        TrapType trapType;
        PlayerSelection& playerSelection;
    };
    class ActionSelector
    {
    public:
        bool operator()(const CEGUI::EventArgs& e)
        {
            playerSelection.setCurrentAction(action);
            return true;
        }
        SelectedAction action;
        PlayerSelection& playerSelection;
    };
    //! \brief Functor for calling notifyGuiAction
    // only used in game and editor currently.
    class GuiNotifier
    {
    public:
        bool operator()(const CEGUI::EventArgs&)
        {
            mode.notifyGuiAction(action);
            return true;
        }
        AbstractApplicationMode::GuiAction action;
        GameEditorModeBase& mode;
    };
}

GameEditorModeBase::GameEditorModeBase(ModeManager* modeManager, ModeManager::ModeType modeType, CEGUI::Window* rootWindow) :
    AbstractApplicationMode(modeManager, modeType),
    mCurrentInputMode(InputModeNormal),
    mRootWindow(rootWindow),
    mGameMap(ODFrameListener::getSingletonPtr()->getClientGameMap()),
    mMiniMap(rootWindow->getChild(Gui::MINIMAP)),
    mConsoleMode(Utils::make_unique<ConsoleMode>(modeManager))
{
    addEventConnection(
        rootWindow->getChild(Gui::MINIMAP)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(&GameEditorModeBase::onMinimapClick, this)
    ));

    //Rooms
    addEventConnection(
        rootWindow->getChild(Gui::BUTTON_DESTROY_ROOM)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(ActionSelector{SelectedAction::destroyRoom, mPlayerSelection})
        )
    );
    connectRoomSelect(Gui::BUTTON_CRYPT, RoomType::crypt);
    connectRoomSelect(Gui::BUTTON_DORMITORY, RoomType::dormitory);
    connectRoomSelect(Gui::BUTTON_TEMPLE, RoomType::dungeonTemple);
    connectRoomSelect(Gui::BUTTON_WORKSHOP, RoomType::workshop);
    connectRoomSelect(Gui::BUTTON_HATCHERY, RoomType::hatchery);
    connectRoomSelect(Gui::BUTTON_LIBRARY, RoomType::library);
    connectRoomSelect(Gui::BUTTON_PORTAL, RoomType::portal);
    connectRoomSelect(Gui::BUTTON_TRAININGHALL, RoomType::trainingHall);
    connectRoomSelect(Gui::BUTTON_TREASURY, RoomType::treasury);

    // Traps
    addEventConnection(
        rootWindow->getChild(Gui::BUTTON_DESTROY_TRAP)->subscribeEvent(
            CEGUI::Window::EventMouseClick,
            CEGUI::Event::Subscriber(ActionSelector{SelectedAction::destroyTrap, mPlayerSelection})
        )
    );
    connectTrapSelect(Gui::BUTTON_TRAP_BOULDER, TrapType::boulder);
    connectTrapSelect(Gui::BUTTON_TRAP_CANNON, TrapType::cannon);
    connectTrapSelect(Gui::BUTTON_TRAP_SPIKE, TrapType::spike);

    // Creature buttons
    connectGuiAction(Gui::BUTTON_CREATURE_WORKER, AbstractApplicationMode::GuiAction::ButtonPressedCreatureWorker);
    connectGuiAction(Gui::BUTTON_CREATURE_FIGHTER, AbstractApplicationMode::GuiAction::ButtonPressedCreatureFighter);
}

GameEditorModeBase::~GameEditorModeBase()
{
    // Delete the potential pending event messages
    for (EventMessage* message : mEventMessages)
        delete message;

    // Clear up any events and chat messages.
    mRootWindow->getChild("GameChatWindow/GameChatText")->setText("");
    mRootWindow->getChild("GameEventText")->setText("");
}

void GameEditorModeBase::deactivate()
{
    // Delete the potential pending event messages
    for (EventMessage* message : mEventMessages)
        delete message;
    mEventMessages.clear();
}

void GameEditorModeBase::connectGuiAction(const std::string& buttonName, AbstractApplicationMode::GuiAction action)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(GuiNotifier{action, *this})
        )
    );
}

EntityBase* GameEditorModeBase::getEntityFromOgreName(const std::string& entityName)
{
    // We check the prefix to know the kind of object the user clicked. Then, we call the corresponding
    // GameMap function to retrieve the entity
    std::string::size_type index = entityName.find("-");
    if(index == std::string::npos)
        return nullptr;

    int32_t intGameEntityType = Helper::toInt(entityName.substr(0, index));
    GameEntityType type = static_cast<GameEntityType>(intGameEntityType);
    return mGameMap->getBaseEntityFromTypeAndName(type, entityName.substr(index + 1));
}


bool GameEditorModeBase::onMinimapClick(const CEGUI::EventArgs& arg)
{
    const CEGUI::MouseEventArgs& mouseEvt = static_cast<const CEGUI::MouseEventArgs&>(arg);

    ODFrameListener& frameListener = ODFrameListener::getSingleton();

    Ogre::Vector2 cc = mMiniMap.camera_2dPositionFromClick(static_cast<int>(mouseEvt.position.d_x),
        static_cast<int>(mouseEvt.position.d_y));
    frameListener.getCameraManager()->onMiniMapClick(cc);

    return true;
}

void GameEditorModeBase::connectRoomSelect(const std::string& buttonName, RoomType roomType)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(RoomSelector{roomType, mPlayerSelection})
        )
    );
}

void GameEditorModeBase::connectTrapSelect(const std::string& buttonName, TrapType trapType)
{
    addEventConnection(
        mRootWindow->getChild(buttonName)->subscribeEvent(
          CEGUI::PushButton::EventClicked,
          CEGUI::Event::Subscriber(TrapSelector{trapType, mPlayerSelection})
        )
    );
}

void GameEditorModeBase::onFrameStarted(const Ogre::FrameEvent& evt)
{
    updateMessages(evt.timeSinceLastFrame);
}

void GameEditorModeBase::receiveChat(ChatMessage* message)
{
    // Adds the message right away
    CEGUI::Window* chatTextBox = mRootWindow->getChild("GameChatWindow/GameChatText");
    chatTextBox->appendText(reinterpret_cast<const CEGUI::utf8*>(message->getMessageAsString().c_str()));

    // Ensure the latest text is shown
    CEGUI::Scrollbar* scrollBar = reinterpret_cast<CEGUI::Scrollbar*>(chatTextBox->getChild("__auto_vscrollbar__"));
    scrollBar->setScrollPosition(scrollBar->getDocumentSize());

    // Delete it now we don't need it anymore.
    delete message;
}

void GameEditorModeBase::receiveEventShortNotice(EventMessage* event)
{
    mEventMessages.emplace_back(event);

    // Adds the message right away
    CEGUI::Window* shortNoticeText = mRootWindow->getChild("GameEventText");
    shortNoticeText->appendText(reinterpret_cast<const CEGUI::utf8*>(event->getMessageAsString().c_str()));

    // Ensure the latest text is shown
    CEGUI::Scrollbar* scrollBar = reinterpret_cast<CEGUI::Scrollbar*>(shortNoticeText->getChild("__auto_vscrollbar__"));
    scrollBar->setScrollPosition(scrollBar->getDocumentSize());
}

void GameEditorModeBase::updateMessages(Ogre::Real update_time)
{
    float maxChatTimeDisplay = ODFrameListener::getSingleton().getEventMaxTimeDisplay();

    // Update the event message seen if necessary.
    CEGUI::Window* shortNoticeText = mRootWindow->getChild("GameEventText");
    CEGUI::Scrollbar* scrollBar = static_cast<CEGUI::Scrollbar*>(shortNoticeText->getChild("__auto_vscrollbar__"));
    float scrollPosition = scrollBar->getScrollPosition();

    // Update the chat message seen if necessary.
    bool messageDisplayUpdate = false;
    CEGUI::String ceguiStr;
    for (auto it = mEventMessages.begin(); it != mEventMessages.end();)
    {
        EventMessage* event = *it;
        if (event->isMessageTooOld(maxChatTimeDisplay))
        {
            delete event;
            it = mEventMessages.erase(it);
            messageDisplayUpdate = true;
        }
        else
        {
            ceguiStr += reinterpret_cast<const CEGUI::utf8*>(event->getMessageAsString().c_str());
            ++it;
        }
    }

    if (messageDisplayUpdate)
    {
        shortNoticeText->setText(ceguiStr);
        scrollBar->setScrollPosition(scrollPosition);
    }
}

CEGUI::Window* GameEditorModeBase::getTabButtonWidget(const std::string& tabName)
{
    CEGUI::Window* win = nullptr;
    // NOTE: The mother tab widget must be named MainTabControl for this to work.
    std::string buttonName = "MainTabControl/__auto_TabPane__Buttons/__auto_btn" + tabName;
    try {
        win = mRootWindow->getChild(buttonName);
    }
    catch (std::exception& e)
    {
    }
    return win;
}

void GameEditorModeBase::setTabButtonToolTip(const std::string& buttonName, const std::string& tooltip)
{
    CEGUI::Window* win = getTabButtonWidget(buttonName);
    if (win != nullptr)
        win->setTooltipText(tooltip);
}

void GameEditorModeBase::enterConsole()
{
    // We use a unique console instance.
    mCurrentInputMode = InputModeConsole;
    mConsoleMode->activate();
}

void GameEditorModeBase::leaveConsole()
{
    // We're no more in console mode.
    mCurrentInputMode = InputModeNormal;
    activate();
}
