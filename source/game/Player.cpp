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

#include "game/Player.h"

#include "game/Seat.h"
#include "rooms/RoomType.h"
#include "spells/SpellManager.h"
#include "spells/SpellType.h"
#include "traps/Trap.h"
#include "network/ODPacket.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "ODApplication.h"

#include <cmath>

Player::Player(GameMap* gameMap, int32_t id) :
    mId(id),
    mGameMap(gameMap),
    mSeat(nullptr),
    mIsHuman(false),
    mNoResearchInQueueTime(0.0f),
    mNoTreasuryAvailableTime(0.0f),
    mHasLost(false),
    mSpellsCooldown(std::vector<std::pair<uint32_t, float>>(static_cast<uint32_t>(SpellType::nbSpells), std::pair<uint32_t, float>(0,0.0f)))
{
}

unsigned int Player::numCreaturesInHand(const Seat* seat) const
{
    unsigned int cpt = 0;
    for(GameEntity* entity : mObjectsInHand)
    {
        if(entity->getObjectType() != GameEntityType::creature)
            continue;

        if(seat != nullptr && entity->getSeat() != seat)
            continue;

        ++cpt;
    }
    return cpt;
}

unsigned int Player::numObjectsInHand() const
{
    return mObjectsInHand.size();
}

void Player::addEntityToHand(GameEntity *entity)
{
    if (mObjectsInHand.empty())
    {
        mObjectsInHand.push_back(entity);
        return;
    }

    // creaturesInHand.push_front(c);
    // Since vectors have no push_front method,
    // we need to move all of the elements in the vector back one
    // and then add this one to the beginning.
    mObjectsInHand.push_back(nullptr);
    for (unsigned int j = mObjectsInHand.size() - 1; j > 0; --j)
        mObjectsInHand[j] = mObjectsInHand[j - 1];

    mObjectsInHand[0] = entity;
}

void Player::clearObjectsInHand()
{
    mObjectsInHand.clear();
}

ODPacket& operator<<(ODPacket& os, const PlayerEventType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, PlayerEventType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<PlayerEventType>(tmp);
    return is;
}

void Player::updateEvents(const std::vector<PlayerEvent*>& events)
{
    for(PlayerEvent* event : mEvents)
        delete event;

    mEvents = events;
}

const PlayerEvent* Player::getNextEvent(uint32_t& index) const
{
    if(mEvents.empty())
        return nullptr;

    index = (index + 1) % mEvents.size();

    return mEvents[index];
}

uint32_t Player::getSpellCooldownTurns(SpellType spellType) const
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return 0;
    }

    return mSpellsCooldown.at(spellIndex).first;
}

float Player::getSpellCooldownSmooth(SpellType spellType) const
{
    uint32_t spellIndex = static_cast<uint32_t>(spellType);
    if(spellIndex >= mSpellsCooldown.size())
    {
        OD_LOG_ERR("seatId=" + Helper::toString(getId()) + ", spellType=" + SpellManager::getSpellNameFromSpellType(spellType));
        return 0;
    }

    const std::pair<uint32_t, float>& cooldown = mSpellsCooldown.at(spellIndex);
    uint32_t coolDownTurns = cooldown.first;
    if(coolDownTurns <= 0)
        return 0.0f;

    uint32_t maxCoolDownTurns = SpellManager::getSpellCooldown(spellType);
    if(maxCoolDownTurns <= 0)
        return 0.0f;

    float coolDownTime = static_cast<float>(coolDownTurns) / ODApplication::turnsPerSecond;
    coolDownTime += cooldown.second;
    float maxCoolDownTime = static_cast<float>(maxCoolDownTurns) / ODApplication::turnsPerSecond;

    return coolDownTime / maxCoolDownTime;
}

void Player::decreaseSpellCooldowns()
{
    for(std::pair<uint32_t, float>& cooldown : mSpellsCooldown)
    {
        if(cooldown.first <= 0)
            continue;

        --cooldown.first;
        cooldown.second = 1.0f / ODApplication::turnsPerSecond;
    }
}

void Player::frameStarted(float timeSinceLastFrame)
{
    // Update the smooth spell cooldown
    for(std::pair<uint32_t, float>& cooldown : mSpellsCooldown)
    {
        if(cooldown.first <= 0)
            continue;
        if(timeSinceLastFrame > cooldown.second)
        {
            cooldown.second = 0.0f;
            continue;
        }

        cooldown.second -= timeSinceLastFrame;
    }
}
