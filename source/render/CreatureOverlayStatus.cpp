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

#include "render/CreatureOverlayStatus.h"

#include "entities/Creature.h"
#include "game/Seat.h"
#include "render/MovableTextOverlay.h"
#include "render/RenderManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"

#include <OgreEntity.h>

const std::string CREATURE_OVERLAY_STATUS_PREFIX = "CreatureOverlayStatus_";

enum class CreatureOverlays
{
    health,
    status,
    nbCreatureOverlays
};

CreatureOverlayStatus::CreatureOverlayStatus(Creature* creature, Ogre::Entity* ent,
        Ogre::Camera* cam) :
    mCreature(creature),
    mMovableTextOverlay(nullptr),
    mHealthValue(0),
    mLevel(0),
    mTimeDisplayStatus(0),
    mStatus(0),
    mOverlayIds(std::vector<uint32_t>(static_cast<uint32_t>(CreatureOverlays::nbCreatureOverlays), 0))
{
    mMovableTextOverlay = new MovableTextOverlay(creature->getName(),
        ent, cam);

    uint32_t healthId = mMovableTextOverlay->createChildOverlay("MedievalSharp", 16, Ogre::ColourValue::White, "");
    mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)] = healthId;
    mMovableTextOverlay->setCaption(healthId, Helper::toString(creature->getLevel()));
    mMovableTextOverlay->forceTextArea(healthId, 42,42);
    mMovableTextOverlay->displayOverlay(healthId, 0);
    updateHealth();

    uint32_t statusId = mMovableTextOverlay->createChildOverlay("MedievalSharp", 16, Ogre::ColourValue::White, "");
    mOverlayIds[static_cast<uint32_t>(CreatureOverlays::status)] = statusId;
    mMovableTextOverlay->forceTextArea(statusId, 32,32);
    // Note: We set the material to the first status overlay material otherwise, materials
    // are not shown when we change then ingame
    mMovableTextOverlay->setMaterialName(statusId, CREATURE_OVERLAY_STATUS_PREFIX + "1");
    mMovableTextOverlay->displayOverlay(statusId, 0);

}

CreatureOverlayStatus::~CreatureOverlayStatus()
{
    delete mMovableTextOverlay;
}

void CreatureOverlayStatus::displayHealthOverlay(Ogre::Real timeToDisplay)
{
    uint32_t healthId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)];
    mMovableTextOverlay->displayOverlay(healthId, timeToDisplay);
}

void CreatureOverlayStatus::updateHealth()
{
    // We adapt the material
    if((mHealthValue != mCreature->getOverlayHealthValue()) ||
        (mSeat != mCreature->getSeat()))
    {
        mHealthValue = mCreature->getOverlayHealthValue();
        mSeat = mCreature->getSeat();
        std::string material = RenderManager::getSingleton().rrBuildSkullFlagMaterial(
            "CreatureOverlay" + Helper::toString(mHealthValue),
            mSeat->getColorValue());
        uint32_t healthId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)];
        mMovableTextOverlay->setMaterialName(healthId, material);
    }

    if(mLevel != mCreature->getLevel())
    {
        mLevel = mCreature->getLevel();
        uint32_t healthId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)];
        mMovableTextOverlay->setCaption(healthId, Helper::toString(mLevel));
    }
}

void CreatureOverlayStatus::updateStatus(Ogre::Real timeSincelastFrame)
{
    uint32_t moodValue = mCreature->getOverlayMoodValue();
    if((mStatus == 0) && (moodValue == 0))
        return;

    // If current status is still on, we change only if we have displayed the current one for too long
    if((mStatus & moodValue) != 0)
    {
        if(mTimeDisplayStatus > timeSincelastFrame)
        {
            mTimeDisplayStatus -= timeSincelastFrame;
            return;
        }

        // If the current status is the only one activated, no need to change
        if((moodValue & ~mStatus) == 0)
            return;
    }

    uint32_t statusId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::status)];
    uint32_t newStatus;
    if(moodValue == 0)
    {
        newStatus = 0;
    }
    else if(mStatus == 0)
    {
        // There is a status to display. We look for the first one
        newStatus = 1;
    }
    else
    {
        // We are already displaying a status and it should be displayed. We check if there is another one
        newStatus = mStatus << 1;
    }

    // We look for the next status (if we start from first position, no need to loop)
    bool hasLooped = newStatus <= 1;
    while(newStatus > 0)
    {
        if(newStatus > moodValue)
        {
            if(hasLooped)
            {
                // We have looped without finding a fitting status. That is not normal since creature mood
                // value is not 0 and we are supposed to have tried every bit
                OD_LOG_ERR("mStatus=" + Helper::toString(mStatus) + ", CreatureOverlayMoodValue=" + Helper::toString(mCreature->getOverlayMoodValue()));
                mStatus = 0;
                return;
            }

            newStatus = 1;
        }

        if((newStatus & moodValue) != 0)
            break;

        newStatus = newStatus << 1;
    }

    mStatus = newStatus;

    if(mStatus == 0)
    {
        mMovableTextOverlay->displayOverlay(statusId, 0);
        mTimeDisplayStatus = 0.0;
        return;
    }

    mTimeDisplayStatus = 1.0;
    std::string material = CREATURE_OVERLAY_STATUS_PREFIX + Helper::toString(mStatus);
    mMovableTextOverlay->setMaterialName(statusId, material);
    mMovableTextOverlay->displayOverlay(statusId, -1);
}

void CreatureOverlayStatus::update(Ogre::Real timeSincelastFrame)
{
    // If the creature is not on map, we do not display the overlays
    mMovableTextOverlay->setVisible(mCreature->getIsOnMap());

    updateHealth();
    updateStatus(timeSincelastFrame);

    mMovableTextOverlay->update(timeSincelastFrame);
}
