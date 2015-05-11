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

#include "render/CreatureOverlayStatus.h"

#include "entities/Creature.h"
#include "game/Seat.h"
#include "render/MovableTextOverlay.h"
#include "render/RenderManager.h"
#include "utils/Helper.h"

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
    mOverlayIds(std::vector<uint32_t>(static_cast<uint32_t>(CreatureOverlays::nbCreatureOverlays), 0))
{
    mMovableTextOverlay = new MovableTextOverlay(creature->getName(),
        ent, cam);

    uint32_t healthId = mMovableTextOverlay->createChildOverlay("MedievalSharp", 16, Ogre::ColourValue::White, "");
    mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)] = healthId;
    mMovableTextOverlay->setCaption(healthId, Helper::toString(creature->getLevel()));
    mMovableTextOverlay->forceTextArea(healthId, 42,42);
    mMovableTextOverlay->displayOverlay(healthId, 0);

    updateHealth(mCreature->getSeat(), mCreature->getOverlayHealthValue(), mCreature->getLevel());
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

void CreatureOverlayStatus::updateHealth(Seat* seat, uint32_t value, unsigned int level)
{
    // We adapt the material
    if((mHealthValue != value) ||
        (mSeat != seat))
    {
        mHealthValue = value;
        mSeat = seat;
        std::string material = RenderManager::getSingleton().rrBuildSkullFlagMaterial(
        "CreatureOverlay" + Helper::toString(mHealthValue),
        mSeat->getColorValue());
        uint32_t healthId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)];
        mMovableTextOverlay->setMaterialName(healthId, material);
    }

    if(mLevel != level)
    {
        mLevel = level;
        uint32_t healthId = mOverlayIds[static_cast<uint32_t>(CreatureOverlays::health)];
        mMovableTextOverlay->setCaption(healthId, Helper::toString(mLevel));
    }
}

void CreatureOverlayStatus::update(Ogre::Real timeSincelastFrame)
{
    if((mCreature->getOverlayHealthValue() != mHealthValue) ||
       (mCreature->getSeat() != mSeat) ||
       (mCreature->getLevel() != mLevel))
    {
        updateHealth(mCreature->getSeat(), mCreature->getOverlayHealthValue(), mCreature->getLevel());
    }

    mMovableTextOverlay->update(timeSincelastFrame);
}
