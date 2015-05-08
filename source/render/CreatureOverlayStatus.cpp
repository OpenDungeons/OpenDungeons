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

CreatureOverlayStatus::CreatureOverlayStatus(Creature* creature, Ogre::Entity* ent,
        Ogre::Camera* cam) :
    mCreature(creature),
    mMovableTextOverlay(nullptr),
    mHealthValue(0),
    mLevel(0),
    mDisplay(false),
    mTimeToDisplay(0.0)
{
    mMovableTextOverlay = new MovableTextOverlay(creature->getName(),
        ent, cam, "MedievalSharp", 16, Ogre::ColourValue::White, "");
    mMovableTextOverlay->setCaption(Helper::toString(creature->getLevel()));
    mLevel = creature->getLevel();
    mSeat = creature->getSeat();
    mMovableTextOverlay->forceTextArea(42,42);
    mMovableTextOverlay->setDisplay(false);

    updateMaterial(mCreature->getSeat(), mCreature->getOverlayHealthValue());
}

CreatureOverlayStatus::~CreatureOverlayStatus()
{
    setDisplay(false);
    delete mMovableTextOverlay;
}

void CreatureOverlayStatus::setDisplay(bool display)
{
    mDisplay = display;
    mMovableTextOverlay->setDisplay(display);
}

void CreatureOverlayStatus::setTemporaryDisplayTime(Ogre::Real timeToDisplay)
{
    mTimeToDisplay = timeToDisplay;
    mMovableTextOverlay->setDisplay(true);
}

void CreatureOverlayStatus::updateMaterial(Seat* seat, uint32_t value)
{
   // We adapt the material
    mHealthValue = value;
    mSeat = seat;
    std::string material = RenderManager::getSingleton().rrBuildSkullFlagMaterial(
        "CreatureOverlay" + Helper::toString(mHealthValue),
        mSeat->getColorValue());
    mMovableTextOverlay->setMaterialName(material);
}

void CreatureOverlayStatus::update(Ogre::Real timeSincelastFrame)
{
    if((mCreature->getOverlayHealthValue() != mHealthValue) ||
       (mCreature->getSeat() != mSeat))
    {
        updateMaterial(mCreature->getSeat(), mCreature->getOverlayHealthValue());
    }

    if(mLevel != mCreature->getLevel())
    {
        mLevel = mCreature->getLevel();
        mMovableTextOverlay->setCaption(Helper::toString(mLevel));
    }

    if(mTimeToDisplay > 0.0)
    {
        if(mDisplay)
        {
            mTimeToDisplay = 0.0;
        }
        else if(timeSincelastFrame < mTimeToDisplay)
        {
            mTimeToDisplay -= timeSincelastFrame;
        }
        else
        {
            // We don't need to show the overlay anymore
            mTimeToDisplay = 0.0;
            mMovableTextOverlay->setDisplay(false);
        }
    }

    mMovableTextOverlay->update(timeSincelastFrame);
}
