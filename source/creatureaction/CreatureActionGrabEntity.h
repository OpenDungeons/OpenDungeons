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

#ifndef CREATUREACTIONGRABENTITY_H
#define CREATUREACTIONGRABENTITY_H

#include "creatureaction/CreatureAction.h"
#include "entities/GameEntity.h"

class GameEntity;

class CreatureActionGrabEntity : public CreatureAction, public GameEntityListener
{
public:
    CreatureActionGrabEntity(Creature& creature, GameEntity& entityToCarry);
    virtual ~CreatureActionGrabEntity();

    CreatureActionType getType() const override
    { return CreatureActionType::grabEntity; }

    std::function<bool()> action() override;

    std::string getListenerName() const override;
    bool notifyDead(GameEntity* entity) override;
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override;
    bool notifyDropped(GameEntity* entity) override;

    static bool handleGrabEntity(Creature& creature, GameEntity* entityToCarry);

private:
    GameEntity* mEntityToCarry;
};

#endif // CREATUREACTIONGRABENTITY_H
