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

#include "creatureaction/CreatureActionDigTile.h"

#include "creatureaction/CreatureActionGrabEntity.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "entities/TreasuryObject.h"
#include "game/Player.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "rooms/Room.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/MakeUnique.h"
#include "utils/LogManager.h"

CreatureActionDigTile::CreatureActionDigTile(Creature& creature, Tile& tileDig, Tile& tilePos) :
    CreatureAction(creature),
    mTileDig(tileDig),
    mTilePos(tilePos)
{
    mTileDig.addWorkerDigging(mCreature, mTilePos);
}

CreatureActionDigTile::~CreatureActionDigTile()
{
    mTileDig.removeWorkerDigging(mCreature, mTilePos);
}

std::function<bool()> CreatureActionDigTile::action()
{
    return std::bind(&CreatureActionDigTile::handleDigTile,
        std::ref(mCreature), std::ref(mTileDig), std::ref(mTilePos));
}

bool CreatureActionDigTile::handleDigTile(Creature& creature, Tile& tileDig, Tile& tilePos)
{
    Tile* myTile = creature.getPositionTile();
    if(myTile == nullptr)
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        creature.popAction();
        return false;
    }

    // Check if the tile should still be dug (it may have been dug by another worker)
    if(!tileDig.getMarkedForDigging(creature.getSeat()->getPlayer()) ||
       !tileDig.isDiggable(creature.getSeat()))
    {
        creature.popAction();
        return true;
    }

    // We go to the tile we locked
    if(&tilePos != myTile )
    {
        if(!creature.setDestination( &tilePos))
        {
            OD_LOG_ERR("creature=" + creature.getName() + ", myTile=" + Tile::displayAsString(myTile) + ", tileDig=" + Tile::displayAsString(&tileDig) + ", tilePos=" + Tile::displayAsString(&tilePos));
            creature.popAction();
        }
        return true;
    }
    else if(!creature.parkedBit)
    {
        creature.parkToWallTile(&tileDig, &tilePos);
        return true;
    }

    // Dig out the tile by decreasing the tile's fullness.
    const Ogre::Vector3& pos = creature.getPosition();
    Ogre::Vector3 walkDirection(tileDig.getX() - pos.x, tileDig.getY() - pos.y, 0);
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::dig_anim, true, walkDirection);
    double amountDug = tileDig.digOut(creature.getDigRate());
    if(amountDug > 0.0)
    {
        creature.receiveExp(1.5 * creature.getDigRate() / 20.0);

        // If the tile is a gold tile accumulate gold for this creature.
        switch(tileDig.getType())
        {
            case TileType::gold:
            {
                static const double digCoefGold = ConfigManager::getSingleton().getDigCoefGold();
                double tempDouble = digCoefGold * amountDug;
                creature.addGoldCarried(static_cast<int>(tempDouble));
                creature.getSeat()->addGoldMined(static_cast<int>(tempDouble));
                // Receive experience for digging gold
                creature.receiveExp(creature.getDigRate() / 20.0);
                break;
            }
            case TileType::gem:
            {
                static const double digCoefGem = ConfigManager::getSingleton().getDigCoefGem();
                double tempDouble = digCoefGem * amountDug;
                creature.addGoldCarried(static_cast<int>(tempDouble));
                creature.getSeat()->addGoldMined(static_cast<int>(tempDouble));
                // Receive experience for digging
                creature.receiveExp(creature.getDigRate() / 20.0);
                break;
            }
            default:
                break;
        }

        // If the tile has been dug out, move into that tile and try to continue digging.
        if (tileDig.getFullness() <= 0.0)
        {
            creature.popAction();
            creature.receiveExp(2.5);
            creature.setDestination(&tileDig);
        }
        //Set sound position and play dig sound.
        creature.fireCreatureSound(CreatureSound::Dig);
    }
    else
    {
        //We tried to dig a tile we are not able to
        //Completely bail out if this happens.
        creature.clearActionQueue();
    }

    // Check to see if we are carrying the maximum amount of gold we can carry, and if so, try to take it to a treasury.
    if (creature.getGoldCarried() >= creature.getDefinition()->getMaxGoldCarryable())
    {
        // We create the treasury object and push action to deposit it
        TreasuryObject* obj = new TreasuryObject(creature.getGameMap(), creature.getGoldCarried());
        creature.resetGoldCarried();
        Ogre::Vector3 pos(static_cast<Ogre::Real>(myTile->getX()), static_cast<Ogre::Real>(myTile->getY()), 0.0f);
        obj->addToGameMap();
        obj->createMesh();
        obj->setPosition(pos);

        bool isTreasuryAvailable = false;
        for(Room* room : creature.getGameMap()->getRooms())
        {
            if(room->getSeat() != creature.getSeat())
                continue;

            if(room->getTotalGoldStorage() <= 0)
                continue;

            if(room->getTotalGoldStored() >= room->getTotalGoldStorage())
                continue;

            if(room->numCoveredTiles() <= 0)
                continue;

            Tile* tile = room->getCoveredTile(0);
            if(!creature.getGameMap()->pathExists(&creature, myTile, tile))
                continue;

            isTreasuryAvailable = true;
            break;
        }

        if(isTreasuryAvailable)
        {
            // We do not push CreatureActionType::searchEntityToCarry because we want
            // this worker to be count as digging, not as carrying stuff
            creature.pushAction(Utils::make_unique<CreatureActionGrabEntity>(creature, *obj));
            return true;
        }

        if((creature.getSeat()->getPlayer() != nullptr) &&
            creature.getSeat()->getPlayer()->getIsHuman() &&
            !creature.getSeat()->getPlayer()->getHasLost())
        {
            creature.getSeat()->getPlayer()->notifyNoTreasuryAvailable();
        }
    }

    return false;
}
