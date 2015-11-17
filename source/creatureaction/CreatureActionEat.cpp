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

#include "creatureaction/CreatureActionEat.h"

#include "creatureaction/CreatureActionWalkToTile.h"
#include "entities/ChickenEntity.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/Room.h"
#include "rooms/RoomType.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/MakeUnique.h"
#include "utils/Random.h"

std::function<bool()> CreatureActionEat::action()
{
    return std::bind(&CreatureActionEat::handleEat,
        std::ref(mCreature), mForced);
}

bool CreatureActionEat::handleEat(Creature& creature, bool forced)
{
    // Current creature tile position
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName());
        creature.popAction();
        creature.stopEating();
        return true;
    }

    if(creature.getEatCooldown() > 0)
    {
        creature.decreaseEatCooldown();
        // We do nothing
        creature.setAnimationState(EntityAnimation::idle_anim);
        return false;
    }

    if (forced && creature.getHunger() < 5.0)
    {
        creature.popAction();

        creature.stopEating();
        return true;
    }

    if (!forced && (creature.getHunger() <= Random::Double(0.0, 15.0)))
    {
        creature.popAction();

        creature.stopEating();
        return true;
    }

    // If we are in a hatchery, we go to the closest chicken in it. If we are not
    // in a hatchery, we check if there is a free chicken and eat it if we see it
    Tile* closestChickenTile = nullptr;
    double closestChickenDist = 0.0;
    for(Tile* tile : creature.getTilesWithinSightRadius())
    {
        std::vector<GameEntity*> chickens;
        tile->fillWithChickenEntities(chickens);
        if(chickens.empty())
            continue;

        if((creature.getEatRoom() == nullptr) &&
           (tile->getCoveringRoom() != nullptr) &&
           (tile->getCoveringRoom()->getType() == RoomType::hatchery))
        {
            // We are not in a hatchery and the currently processed tile is a hatchery. We
            // cannot eat any chicken there
            continue;
        }

        if((creature.getEatRoom() != nullptr) && (tile->getCoveringBuilding() != creature.getEatRoom()))
            continue;

        double dist = std::pow(static_cast<double>(std::abs(tile->getX() - myTile->getX())), 2);
        dist += std::pow(static_cast<double>(std::abs(tile->getY() - myTile->getY())), 2);
        if((closestChickenTile == nullptr) ||
           (closestChickenDist > dist))
        {
            closestChickenTile = tile;
            closestChickenDist = dist;
        }
    }

    if(closestChickenTile != nullptr)
    {
        if(closestChickenDist <= 1.0)
        {
            // We eat the chicken
            std::vector<GameEntity*> chickens;
            closestChickenTile->fillWithChickenEntities(chickens);
            if(chickens.empty())
            {
                OD_LOG_ERR("name=" + creature.getName());
                return false;
            }
            ChickenEntity* chicken = static_cast<ChickenEntity*>(chickens.at(0));
            chicken->eatChicken(&creature);
            creature.foodEaten(ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHungerPerChicken"));
            creature.setEatCooldown(Random::Int(ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMin"),
                ConfigManager::getSingleton().getRoomConfigUInt32("HatcheryCooldownChickenMax")));
            creature.setHP(creature.getHP() + ConfigManager::getSingleton().getRoomConfigDouble("HatcheryHpRecoveredPerChicken"));
            creature.computeCreatureOverlayHealthValue();
            Ogre::Vector3 walkDirection = Ogre::Vector3(closestChickenTile->getX(), closestChickenTile->getY(), 0) - creature.getPosition();
            walkDirection.normalise();
            creature.setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
            return false;
        }

        // We walk to the chicken
        std::list<Tile*> pathToChicken = creature.getGameMap()->path(&creature, closestChickenTile);
        if(pathToChicken.empty())
        {
            OD_LOG_ERR("creature=" + creature.getName() + " posTile=" + Tile::displayAsString(myTile) + " empty path to chicken tile=" + Tile::displayAsString(closestChickenTile));
            creature.popAction();
            creature.stopEating();
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
        creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
        creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
        return false;
    }

    if(creature.getEatRoom() != nullptr)
        return false;

    // See if we are in a hatchery. If so, we try to add the creature. If it is ok, the room
    // will handle the creature from here to make it go where it should
    if((myTile->getCoveringRoom() != nullptr) &&
       (creature.getSeat()->canOwnedCreatureUseRoomFrom(myTile->getCoveringRoom()->getSeat())) &&
       (myTile->getCoveringRoom()->getType() == RoomType::hatchery) &&
       (myTile->getCoveringRoom()->hasOpenCreatureSpot(&creature)))
    {
        Room* tempRoom = myTile->getCoveringRoom();
        if(tempRoom->addCreatureUsingRoom(&creature))
        {
            creature.changeEatRoom(tempRoom);
            return false;
        }
    }

    // Get the list of hatchery controlled by our seat and make sure there is at least one.
    std::vector<Room*> tempRooms = creature.getGameMap()->getRoomsByTypeAndSeat(RoomType::hatchery, creature.getSeat());
    if (tempRooms.empty())
    {
        creature.popAction();
        creature.stopEating();
        return true;
    }

    // Pick a hatchery and try to walk to it.
    double maxDistance = 40.0;
    Room* tempRoom = nullptr;
    int nbTry = 5;
    do
    {
        int tempInt = Random::Uint(0, tempRooms.size() - 1);
        tempRoom = tempRooms[tempInt];
        tempRooms.erase(tempRooms.begin() + tempInt);
        double tempDouble = 1.0 / (maxDistance - Pathfinding::distanceTile(*myTile, *tempRoom->getCoveredTile(0)));
        if (Random::Double(0.0, 1.0) < tempDouble)
            break;
        --nbTry;
    } while (nbTry > 0 && !tempRoom->hasOpenCreatureSpot(&creature) && !tempRooms.empty());

    if (!tempRoom || !tempRoom->hasOpenCreatureSpot(&creature))
    {
        // The room is already being used, stop trying to eat
        creature.popAction();
        creature.stopEating();
        return true;
    }

    Tile* tempTile = tempRoom->getCoveredTile(Random::Uint(0, tempRoom->numCoveredTiles() - 1));
    std::list<Tile*> tempPath = creature.getGameMap()->path(&creature, tempTile);
    if (tempPath.size() < maxDistance)
    {
        std::vector<Ogre::Vector3> path;
        creature.tileToVector3(tempPath, path, true, 0.0);
        creature.setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
        creature.pushAction(Utils::make_unique<CreatureActionWalkToTile>(creature));
        return false;
    }
    else
    {
        // We could not find a room where we can eat so stop trying to find one.
        creature.popAction();
        creature.stopEating();
        return true;
    }
}
