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

#include "creatureaction/CreatureActionEatChicken.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/ChickenEntity.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

CreatureActionEatChicken::CreatureActionEatChicken(Creature& creature, ChickenEntity& chicken) :
    CreatureAction(creature),
    mChicken(&chicken)
{
    mChicken->addGameEntityListener(this);
    mChicken->setLockEat(mCreature, true);
}

CreatureActionEatChicken::~CreatureActionEatChicken()
{
    if(mChicken != nullptr)
    {
        mChicken->setLockEat(mCreature, false);
        mChicken->removeGameEntityListener(this);
    }
}

std::function<bool()> CreatureActionEatChicken::action()
{
    return std::bind(&CreatureActionEatChicken::handleEatChicken,
        std::ref(mCreature), mChicken);
}

bool CreatureActionEatChicken::handleEatChicken(Creature& creature, ChickenEntity* chicken)
{
    if(!creature.decreaseJobCooldown())
        return false;

    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", position=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    if(chicken == nullptr)
    {
        creature.popAction();
        return false;
    }

    Tile* chickenTile = chicken->getPositionTile();
    if(chickenTile == nullptr)
    {
        OD_LOG_ERR("name=" + creature.getName() + ", chicken=" + chicken->getName() + ", chicken position=" + Helper::toString(chicken->getPosition()));
        creature.popAction();
        return false;
    }

    float dist = Pathfinding::squaredDistanceTile(*myTile, *chickenTile);
    if(dist > 1)
    {
        // We walk to the chicken
        std::list<Tile*> pathToChicken = creature.getGameMap()->path(&creature, chickenTile);
        if(pathToChicken.empty())
        {
            OD_LOG_ERR("creature=" + creature.getName() + " posTile=" + Tile::displayAsString(myTile) + " empty path to chicken tile=" + Tile::displayAsString(chickenTile));
            creature.popAction();
            return true;
        }

        // We make sure we don't go too far as the chicken is also moving
        if(pathToChicken.size() > 2)
        {
            // We only keep 80% of the path
            int nbTiles = 8 * pathToChicken.size() / 10;
            pathToChicken.resize(nbTiles);
        }

        std::vector<Ogre::Vector3> path;
        creature.tileToVector3(pathToChicken, path, true, 0.0);
        creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path);
        creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
        return false;
    }

    // We can eat the chicken
    chicken->eatChicken(&creature);
    creature.foodEaten(ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHungerPerChicken"));
    creature.setJobCooldown(Random::Int(ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMin"),
        ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMax")));
    creature.setHP(creature.getHP() + ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHpRecoveredPerChicken"));
    creature.computeCreatureOverlayHealthValue();
    Ogre::Vector3 walkDirection = Ogre::Vector3(chickenTile->getX(), chickenTile->getY(), 0) - creature.getPosition();
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
    return false;
}

std::string CreatureActionEatChicken::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionEatChicken::notifyDead(GameEntity* entity)
{
    if(entity == mChicken)
    {
        mChicken->setLockEat(mCreature, false);
        mChicken = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionEatChicken::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mChicken)
    {
        mChicken->setLockEat(mCreature, false);
        mChicken = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionEatChicken::notifyPickedUp(GameEntity* entity)
{
    if(entity == mChicken)
    {
        mChicken->setLockEat(mCreature, false);
        mChicken = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionEatChicken::notifyDropped(GameEntity* entity)
{
    // That should not happen because we should have stopped listening when the creature was picked up
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", chicken=" + mChicken->getName());
    return true;
}
