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

#include "creatureaction/CreatureActionFight.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"

CreatureActionFight::CreatureActionFight(Creature& creature, GameEntity* entityAttack) :
    CreatureAction(creature),
    mEntityAttack(entityAttack)
{
    if(mEntityAttack != nullptr)
        mEntityAttack->addGameEntityListener(this);
}

CreatureActionFight::~CreatureActionFight()
{
    if(mEntityAttack != nullptr)
        mEntityAttack->removeGameEntityListener(this);
}

std::function<bool()> CreatureActionFight::action()
{
    return std::bind(&CreatureActionFight::handleFight,
        std::ref(mCreature), mEntityAttack);
}

bool CreatureActionFight::handleFight(Creature& creature, GameEntity* entityAttack)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    std::vector<GameEntity*> enemyPrioritaryTargets;
    std::vector<GameEntity*> enemySecondaryTargets;
    // If we have a particular target, we attack it. Otherwise, we search for one
    if(entityAttack != nullptr)
    {
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

        enemyPrioritaryTargets.push_back(entityAttack);
    }
    else
    {
        // If there are no more enemies, stop fighting.
        if (creature.getVisibleEnemyObjects().empty())
        {
            creature.popAction();
            return true;
        }

        // We try to attack creatures first
        for(GameEntity* entity : creature.getVisibleEnemyObjects())
        {
            switch(entity->getObjectType())
            {
                case GameEntityType::creature:
                {
                    Creature* enemyCreature = static_cast<Creature*>(entity);
                    if(!enemyCreature->isAlive())
                        continue;

                    // Workers should attack workers only
                    if(creature.getDefinition()->isWorker() && !enemyCreature->getDefinition()->isWorker())
                        continue;

                    enemyPrioritaryTargets.push_back(enemyCreature);
                    break;
                }
                default:
                {
                    // Workers can attack workers only
                    if(creature.getDefinition()->isWorker())
                        continue;

                    enemySecondaryTargets.push_back(entity);
                }
            }
        }
    }

    if(!enemyPrioritaryTargets.empty())
    {
        GameEntity* entityAttack = nullptr;
        Tile* tileAttack = nullptr;
        Tile* tilePosition = nullptr;
        CreatureSkillData* skillData = nullptr;
        if(creature.searchBestTargetInList(enemyPrioritaryTargets, entityAttack, tileAttack, tilePosition, skillData))
        {
            if((myTile == tilePosition) &&
               (entityAttack != nullptr) &&
               (tileAttack != nullptr) &&
               (skillData != nullptr))
            {
                // We can attack
                bool ko = creature.getSeat()->getKoCreatures();
                creature.useAttack(*skillData, *entityAttack, *tileAttack, ko);
                return false;
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
    }

    if(!enemySecondaryTargets.empty())
    {
        GameEntity* entityAttack = nullptr;
        Tile* tileAttack = nullptr;
        Tile* tilePosition = nullptr;
        CreatureSkillData* skillData = nullptr;
        if(creature.searchBestTargetInList(enemySecondaryTargets, entityAttack, tileAttack, tilePosition, skillData))
        {
            if((myTile == tilePosition) &&
               (entityAttack != nullptr) &&
               (tileAttack != nullptr) &&
               (skillData != nullptr))
            {
                // We can attack
                bool ko = creature.getSeat()->getKoCreatures();
                creature.useAttack(*skillData, *entityAttack, *tileAttack, ko);
                return false;
            }

            // We need to move to the entity
            std::list<Tile*> result = creature.getGameMap()->path(&creature, tilePosition);
            if(result.empty())
            {
                OD_LOG_ERR("name" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + ", dest=" + Tile::displayAsString(tilePosition));
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
    }

    // No suitable target
    creature.popAction();
    return true;
}

std::string CreatureActionFight::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionFight::notifyDead(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFight::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFight::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionFight::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
