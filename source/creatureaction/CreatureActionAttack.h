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

#ifndef CREATUREACTIONATTACK_H
#define CREATUREACTIONATTACK_H

#include "creatureaction/CreatureAction.h"
#include "entities/GameEntity.h"

class CreatureSkillData;
class GameEntity;
class Tile;

class CreatureActionAttack : public CreatureAction, public GameEntityListener
{
public:
    CreatureActionAttack(Creature& creature, Tile& tileAttack, CreatureSkillData& skillData, GameEntity& entityAttack, bool ko);

    virtual ~CreatureActionAttack();

    CreatureActionType getType() const override
    { return CreatureActionType::attackObject; }

    std::function<bool()> action() override;

    std::string getListenerName() const override;
    bool notifyDead(GameEntity* entity) override;
    bool notifyRemovedFromGameMap(GameEntity* entity) override;
    bool notifyPickedUp(GameEntity* entity) override;
    bool notifyDropped(GameEntity* entity) override;

    static bool handleAttack(Creature& creature, Tile& tileAttack, CreatureSkillData& skillData,
        GameEntity* entityAttack, bool ko);

private:
    Tile& mTileAttack;
    CreatureSkillData& mSkillData;
    GameEntity* mEntityAttack;
    bool mKo;
};

#endif // CREATUREACTIONATTACK_H
