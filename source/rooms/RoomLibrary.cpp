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

#include "rooms/RoomLibrary.h"

#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/RenderedMovableEntity.h"
#include "entities/SkillEntity.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "game/Skill.h"
#include "game/Seat.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const std::string RoomLibraryName = "Library";
const std::string RoomLibraryNameDisplay = "Library room";
const RoomType RoomLibrary::mRoomType = RoomType::library;

namespace
{
class RoomLibraryFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomLibrary::mRoomType; }

    const std::string& getName() const override
    { return RoomLibraryName; }

    const std::string& getNameReadable() const override
    { return RoomLibraryNameDisplay; }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomLibrary::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomLibrary::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomLibrary* room = new RoomLibrary(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomLibrary::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomLibrary* room = new RoomLibrary(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomLibrary* room = new RoomLibrary(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomLibrary::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomLibrary* room = new RoomLibrary(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomLibraryFactory);
}

const Ogre::Real OFFSET_CREATURE = 0.3;
const Ogre::Real OFFSET_SPOT = 0.3;

RoomLibrary::RoomLibrary(GameMap* gameMap) :
    Room(gameMap),
    mSkillPoints(0)
{
    setMeshName("Library");
}

RenderedMovableEntity* RoomLibrary::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            RoomLibraryTileData* roomLibraryTileData = static_cast<RoomLibraryTileData*>(mTileData[tile]);
            roomLibraryTileData->mCanHaveSkillEntity = false;

            Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
            Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
            y += OFFSET_SPOT;
            mUnusedSpots.push_back(tile);
            if (Random::Int(0, 100) > 50)
                return loadBuildingObject(getGameMap(), "Podium", tile, x, y, 45.0, false);
            else
                return loadBuildingObject(getGameMap(), "Bookcase", tile, x, y, 45.0, false);
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            return loadBuildingObject(getGameMap(), "Bookshelf", tile, 180.0, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomLibrary::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }

    Room::absorbRoom(r);

    RoomLibrary* roomAbs = static_cast<RoomLibrary*>(r);
    mUnusedSpots.insert(mUnusedSpots.end(), roomAbs->mUnusedSpots.begin(), roomAbs->mUnusedSpots.end());
    roomAbs->mUnusedSpots.clear();
    // mCreaturesSpots should be empty on the absorbed room
    OD_ASSERT_TRUE_MSG(roomAbs->mCreaturesSpots.empty(), "room=" + getName() + ", roomAbs=" + roomAbs->getName());
}

void RoomLibrary::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    if(place != ActiveSpotPlace::activeSpotCenter)
        return;

    for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
    {
        Tile* tmpTile = p.second;
        if(tmpTile == tile)
        {
            Creature* creature = p.first;
            creature->clearActionQueue();
            // clearActionQueue should have released mCreaturesSpots[creature]. Now, we just need to release the unused spot
            break;
        }
    }

    std::vector<Tile*>::iterator itEr = std::find(mUnusedSpots.begin(), mUnusedSpots.end(), tile);
    if(itEr == mUnusedSpots.end())
    {
        OD_LOG_ERR("name=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }
    mUnusedSpots.erase(itEr);
}

bool RoomLibrary::hasOpenCreatureSpot(Creature* c)
{
    // If there is no need, we do not allow creature to work
    if(getSeat()->getPlayer() == nullptr)
        return false;

    if(!getSeat()->isSkilling())
        return false;

    // We accept all creatures as soon as there are free active spots
    uint32_t nbItems = countSkillItemsOnRoom();
    if(nbItems >= (getNumActiveSpots() - mCreaturesSpots.size()))
        return false;

    return !mUnusedSpots.empty();
}

bool RoomLibrary::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    int index = Random::Int(0, mUnusedSpots.size() - 1);
    Tile* tileSpot = mUnusedSpots[index];
    mUnusedSpots.erase(mUnusedSpots.begin() + index);
    mCreaturesSpots[creature] = tileSpot;
    const Ogre::Vector3& creaturePosition = creature->getPosition();
    Ogre::Real wantedX = -1;
    Ogre::Real wantedY = -1;
    getCreatureWantedPos(creature, tileSpot, wantedX, wantedY);
    if(creaturePosition.x != wantedX ||
       creaturePosition.y != wantedY)
    {
        // We move to the good tile
        std::list<Tile*> pathToSpot = getGameMap()->path(creature, tileSpot);
        if(pathToSpot.empty())
        {
            OD_LOG_ERR("unexpected empty pathToSpot");
            return true;
        }

        std::vector<Ogre::Vector3> path;
        Creature::tileToVector3(pathToSpot, path, true, 0.0);
        // We add the last step to take account of the offset
        Ogre::Vector3 dest(wantedX, wantedY, 0.0);
        path.push_back(dest);
        creature->setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, path);
    }

    return true;
}

void RoomLibrary::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    if(mCreaturesSpots.count(c) > 0)
    {
        Tile* tileSpot = mCreaturesSpots[c];
        if(tileSpot == nullptr)
        {
            OD_LOG_ERR("unexpected null tileSpot");
            return;
        }
        mUnusedSpots.push_back(tileSpot);
        mCreaturesSpots.erase(c);
    }
}

void RoomLibrary::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    if(getSeat()->getPlayer() == nullptr)
        return;

    // If there is nothing to do, we remove the working creatures if any
    if(!getSeat()->isSkilling())
    {
        if(mCreaturesSpots.empty())
            return;

        // We remove the creatures working here
        std::vector<Creature*> creatures;
        for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
        {
            creatures.push_back(p.first);
        }

        for(Creature* creature : creatures)
        {
            creature->clearActionQueue();
        }
        return;
    }

    int32_t skillEntityPoints = ConfigManager::getSingleton().getRoomConfigInt32("LibrarySkillPointsBook");
    for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
    {
        Creature* creature = p.first;
        Tile* tileSpot = p.second;
        Tile* tileCreature = creature->getPositionTile();
        if(tileCreature == nullptr)
            continue;

        Ogre::Real wantedX = -1;
        Ogre::Real wantedY = -1;
        getCreatureWantedPos(creature, tileSpot, wantedX, wantedY);

        RenderedMovableEntity* ro = getBuildingObjectFromTile(tileSpot);
        if(ro == nullptr)
        {
            OD_LOG_ERR("unexpected null building object");
            continue;
        }
        // We consider that the creature is in the good place if it is in the expected tile and not moving
        Tile* expectedDest = getGameMap()->getTile(Helper::round(wantedX), Helper::round(wantedY));
        if(expectedDest == nullptr)
        {
            OD_LOG_ERR("room=" + getName() + ", creature=" + creature->getName());
            continue;
        }
        if((tileCreature == expectedDest) &&
           !creature->isMoving())
        {
            if (!creature->decreaseJobCooldown())
            {
                creature->setAnimationState(EntityAnimation::idle_anim);
            }
            else
            {
                Ogre::Vector3 walkDirection(ro->getPosition().x - creature->getPosition().x, ro->getPosition().y - creature->getPosition().y, 0);
                walkDirection.normalise();
                creature->setAnimationState(EntityAnimation::attack_anim, false, walkDirection);

                ro->setAnimationState("Triggered", false);

                const CreatureRoomAffinity& creatureRoomAffinity = creature->getDefinition()->getRoomAffinity(getType());
                OD_ASSERT_TRUE_MSG(creatureRoomAffinity.getRoomType() == getType(), "name=" + getName() + ", creature=" + creature->getName()
                    + ", creatureRoomAffinityType=" + Helper::toString(static_cast<int>(creatureRoomAffinity.getRoomType())));

                int32_t pointsEarned = static_cast<int32_t>(creatureRoomAffinity.getEfficiency() * ConfigManager::getSingleton().getRoomConfigDouble("LibraryPointsPerWork"));
                creature->jobDone(ConfigManager::getSingleton().getRoomConfigDouble("LibraryWakefulnessPerWork"));
                creature->setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("LibraryCooldownWorkMin"),
                    ConfigManager::getSingleton().getRoomConfigUInt32("LibraryCooldownWorkMax")));

                // We check if we have enough points to create a skill entity
                mSkillPoints += pointsEarned;
                if(mSkillPoints < skillEntityPoints)
                    continue;

                mSkillPoints -= skillEntityPoints;
                // We check if there is an empty tile to release the skillEntity
                Tile* spawnTile = checkIfAvailableSpot();
                if(spawnTile == nullptr)
                {
                    OD_LOG_ERR("room=" + getName());
                    return;
                }

                SkillEntity* skillEntity = new SkillEntity(getGameMap(), true, getName(), skillEntityPoints);
                skillEntity->setSeat(getSeat());
                skillEntity->addToGameMap();
                Ogre::Vector3 spawnPosition(static_cast<Ogre::Real>(spawnTile->getX()), static_cast<Ogre::Real>(spawnTile->getY()), static_cast<Ogre::Real>(0.0));
                skillEntity->createMesh();
                skillEntity->setPosition(spawnPosition);
            }
        }
    }

    uint32_t nbItems = countSkillItemsOnRoom();
    if(nbItems > (getNumActiveSpots() - mCreaturesSpots.size()))
    {
        // There is no available space. We remove a creature working here if there is one.
        // If there is none, it means the library is full
        if(mCreaturesSpots.empty())
            return;

        for(const std::pair<Creature* const,Tile*>& p : mCreaturesSpots)
        {
            Creature* creature = p.first;
            creature->clearActionQueue();
            break;
        }
    }
}

uint32_t RoomLibrary::countSkillItemsOnRoom()
{
    uint32_t nbItems = 0;
    for(Tile* t : mCoveredTiles)
    {
        nbItems += t->countEntitiesOnTile(GameEntityType::skillEntity);
    }

    return nbItems;
}

Tile* RoomLibrary::checkIfAvailableSpot()
{
    for(std::pair<Tile* const, TileData*>& p : mTileData)
    {
        RoomLibraryTileData* roomLibraryTileData = static_cast<RoomLibraryTileData*>(p.second);
        if(!roomLibraryTileData->mCanHaveSkillEntity)
            continue;

        // If the tile contains no item, we can add a new one
        uint32_t nbItems = p.first->countEntitiesOnTile(GameEntityType::skillEntity);
        if(nbItems > 0)
            continue;

        return p.first;
    }

    return nullptr;
}

void RoomLibrary::getCreatureWantedPos(Creature* creature, Tile* tileSpot,
    Ogre::Real& wantedX, Ogre::Real& wantedY)
{
    wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    wantedY = static_cast<Ogre::Real>(tileSpot->getY());
    wantedY -= OFFSET_CREATURE;
}

RoomLibraryTileData* RoomLibrary::createTileData(Tile* tile)
{
    return new RoomLibraryTileData;
}

void RoomLibrary::exportToStream(std::ostream& os) const
{
    Room::exportToStream(os);
    os << mSkillPoints << "\n";
}

bool RoomLibrary::importFromStream(std::istream& is)
{
    if(!Room::importFromStream(is))
        return false;
    if(!(is >> mSkillPoints))
        return false;

    return true;
}
