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

#include "network/ChatEventMessage.h"

#include "game/Player.h"
#include "game/Seat.h"

#include "utils/ConfigManager.h"
#include "utils/Helper.h"

ChatMessage::ChatMessage(Player* player, const std::string& message) :
    mMessage(message),
    mPlayer(player)
{
}

std::string ChatMessage::getMessageAsString()
{
    std::string colorId = mPlayer ? (mPlayer->getSeat() ? mPlayer->getSeat()->getColorId() : "") : "";
    Ogre::ColourValue colorValue = ConfigManager::getSingleton().getColorFromId(colorId);
    const std::string formatSeatColor = "[colour='" + Helper::getCEGUIColorFromOgreColourValue(colorValue) + "']";
    const std::string formatWhiteColor = "[colour='FFFFFFFF']";
    std::string playerNickname = mPlayer ? mPlayer->getNick() : "";
    std::string messageStr = formatSeatColor + playerNickname + formatWhiteColor + ": " + getMessage()  + "\n";
    return messageStr;
}

EventMessage::EventMessage(const std::string& message, EventShortNoticeType type):
    mMessage(message),
    mType(type)
{
}

bool EventMessage::isMessageTooOld(float maxTimeDisplay) const
{
    return mClockCreation.getElapsedTime().asSeconds() > maxTimeDisplay;
}

std::string EventMessage::getMessageAsString()
{
    std::string eventType;
    const std::string formatWhiteColor = "[colour='FFFFFFFF']";
    switch(mType)
    {
        case EventShortNoticeType::genericGameInfo:
            eventType = "[image-size='w:16 h:16'][image='OpenDungeonsIcons/HelpIcon'] [colour='FF00EE00']";
            break;
        case EventShortNoticeType::majorGameEvent:
            eventType = "[image-size='w:16 h:16'][image='OpenDungeonsIcons/SeatIcon'] [colour='FFEE0000']";
            break;
        case EventShortNoticeType::aboutCreatures:
            eventType = "[image-size='w:16 h:16'][image='OpenDungeonsIcons/CreaturesIcon'] [colour='FF11AA22']";
            break;
        default:
        case EventShortNoticeType::aboutSkills:
            eventType = "[image-size='w:16 h:16'][image='OpenDungeonsIcons/SkillIcon'] [colour='FFEEEE00']";
            break;
        case EventShortNoticeType::aboutObjectives:
            eventType = "[image-size='w:16 h:16'][image='OpenDungeonsIcons/ObjectivesIcon'] [colour='FF0000EE']";
            break;
    }
    return eventType + mMessage + formatWhiteColor + "\n";
}
