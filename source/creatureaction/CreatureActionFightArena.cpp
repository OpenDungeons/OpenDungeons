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

#include "creatureaction/CreatureActionFightArena.h"

#include "creatureaction/CreatureActionAttack.h"
#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

CreatureActionFightArena::CreatureActionFightArena(Creature& creature, GameEntity& entityAttack) :
    CreatureAction(creature),
    mEntityAttack(&entityAttack)
{
    mEntityAttack->addGameEntityListener(this);
}

CreatureActionFightArena::~CreatureActionFightArena()
{
    if(mEntityAttack != nullptr)
        mEntityAttack->removeGameEntityListener(this);
}

std::function<bool()> CreatureActionFightArena::action()
{
    return std::bind(&CreatureActionFightArena::handleFightArena,
        std::ref(mCreature), mEntityAttack);
}

bool CreatureActionFightArena::handleFightArena(Creature& creature, GameEntity* entityAttack)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // If we have no opponent, stop fighting
    if(entityAttack == nullptr)
    {
        creature.popAction();
        return true;
    }

    if(entityAttack->getHP(nullptr) <= 0)
    {
        creature.popAction();
        return true;
    }
    Tile* enemyTile = entityAttack->getPositionTile();
    if(enemyTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", enemy=" + entityAttack->getName() + ", enemyPos=" + Helper::toString(entityAttack->getPosition()));
        creature.popAction();
        return true;
    }
    if(!entityAttack->isAttackable(enemyTile, creature.getSeat()))
    {
        creature.popAction();
        return true;
    }

    GameEntity* entityToAttack = nullptr;
    Tile* tileAttack = nullptr;
    Tile* tilePosition = nullptr;
    CreatureSkillData* skillData = nullptr;
    std::vector<GameEntity*> enemy;
    enemy.push_back(entityAttack);
    if(creature.searchBestTargetInList(enemy, entityToAttack, tileAttack, tilePosition, skillData))
    {
        if((myTile == tilePosition) &&
           (entityToAttack != nullptr) &&
           (tileAttack != nullptr) &&
           (skillData != nullptr))
        {
            OD_ASSERT_TRUE_MSG(entityToAttack == entityAttack, "Unexpected different creature entityToAttack=" + entityToAttack->getName() + ", entityAttack=" + entityAttack->getName());

            // We can attack
            // In the Arena, we always ko the opponent
            bool ko = true;
            creature.pushAction(Utils::make_unique<CreatureActionAttack>(creature, *tileAttack, *skillData, *entityToAttack, ko));
            return true;
        }

        // We need to move
        std::list<Tile*> result = creature.getGameMap()->path(&creature, tilePosition);
        if(result.empty())
        {
            OD_LOG_ERR("name=" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + ", dest=" + Tile::displayAsString(tilePosition));
            return false;
        }

        if(result.size() > 3)
            result.resize(3);

        std::vector<Ogre::Vector3> path;
        creature.tileToVector3(result, path, true, 0.0);
        creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
        creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
        return false;
    }

    // No suitable target
    creature.popAction();
    return true;
}

std::string CreatureActionFightArena::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionFightArena::notifyDead(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFightArena::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFightArena::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFightArena::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
