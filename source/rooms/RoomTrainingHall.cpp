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

#include "rooms/RoomTrainingHall.h"

#include "entities/BuildingObject.h"
#include "entities/Creature.h"
#include "entities/CreatureDefinition.h"
#include "entities/Tile.h"
#include "game/Player.h"
#include "gamemap/GameMap.h"
#include "rooms/RoomManager.h"
#include "utils/ConfigManager.h"
#include "utils/Helper.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

const std::string RoomTrainingHallName = "TrainingHall";
const std::string RoomTrainingHallNameDisplay = "Training hall room";
const RoomType RoomTrainingHall::mRoomType = RoomType::trainingHall;

namespace
{
class RoomTrainingHallFactory : public RoomFactory
{
    RoomType getRoomType() const override
    { return RoomTrainingHall::mRoomType; }

    const std::string& getName() const override
    { return RoomTrainingHallName; }

    const std::string& getNameReadable() const override
    { return RoomTrainingHallNameDisplay; }

    int getCostPerTile() const override
    { return ConfigManager::getSingleton().getRoomConfigInt32("TrainHallCostPerTile"); }

    void checkBuildRoom(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefault(gameMap, RoomTrainingHall::mRoomType, inputManager, inputCommand);
    }

    bool buildRoom(GameMap* gameMap, Player* player, ODPacket& packet) const override
    {
        std::vector<Tile*> tiles;
        if(!getRoomTilesDefault(tiles, gameMap, player, packet))
            return false;

        int32_t pricePerTarget = RoomManager::costPerTile(RoomTrainingHall::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTrainingHall* room = new RoomTrainingHall(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }

    void checkBuildRoomEditor(GameMap* gameMap, const InputManager& inputManager, InputCommand& inputCommand) const override
    {
        checkBuildRoomDefaultEditor(gameMap, RoomTrainingHall::mRoomType, inputManager, inputCommand);
    }

    bool buildRoomEditor(GameMap* gameMap, ODPacket& packet) const override
    {
        RoomTrainingHall* room = new RoomTrainingHall(gameMap);
        return buildRoomDefaultEditor(gameMap, room, packet);
    }

    Room* getRoomFromStream(GameMap* gameMap, std::istream& is) const override
    {
        RoomTrainingHall* room = new RoomTrainingHall(gameMap);
        if(!Room::importRoomFromStream(*room, is))
        {
            OD_LOG_ERR("Error while building a room from the stream");
        }
        return room;
    }

    bool buildRoomOnTiles(GameMap* gameMap, Player* player, const std::vector<Tile*>& tiles) const override
    {
        int32_t pricePerTarget = RoomManager::costPerTile(RoomTrainingHall::mRoomType);
        int32_t price = static_cast<int32_t>(tiles.size()) * pricePerTarget;
        if(!gameMap->withdrawFromTreasuries(price, player->getSeat()))
            return false;

        RoomTrainingHall* room = new RoomTrainingHall(gameMap);
        return buildRoomDefault(gameMap, room, player->getSeat(), tiles);
    }
};

// Register the factory
static RoomRegister reg(new RoomTrainingHallFactory);
}

static const Ogre::Real OFFSET_CREATURE = 0.3;
static const Ogre::Real OFFSET_DUMMY = 0.3;

RoomTrainingHall::RoomTrainingHall(GameMap* gameMap) :
    Room(gameMap),
    nbTurnsNoChangeDummies(0)
{
    setMeshName("Dojo");
}

void RoomTrainingHall::absorbRoom(Room *r)
{
    if(r->getType() != getType())
    {
        OD_LOG_ERR("Trying to merge incompatible rooms: " + getName() + ", type=" + RoomManager::getRoomNameFromRoomType(getType()) + ", with " + r->getName() + ", type=" + RoomManager::getRoomNameFromRoomType(r->getType()));
        return;
    }

    Room::absorbRoom(r);

    RoomTrainingHall* roomAbs = static_cast<RoomTrainingHall*>(r);
    mUnusedDummies.insert(mUnusedDummies.end(), roomAbs->mUnusedDummies.begin(), roomAbs->mUnusedDummies.end());
    roomAbs->mUnusedDummies.clear();

    OD_ASSERT_TRUE_MSG(roomAbs->mCreaturesDummies.empty(), "room=" + getName() + ", roomAbs=" + roomAbs->getName());
}

BuildingObject* RoomTrainingHall::notifyActiveSpotCreated(ActiveSpotPlace place, Tile* tile)
{
    Ogre::Real x = static_cast<Ogre::Real>(tile->getX());
    Ogre::Real y = static_cast<Ogre::Real>(tile->getY());
    Ogre::Real z = 0;
    switch(place)
    {
        case ActiveSpotPlace::activeSpotCenter:
        {
            y += OFFSET_DUMMY;
            mUnusedDummies.push_back(tile);
            switch(Random::Int(1, 4))
            {
                case 1:
                    return new BuildingObject(getGameMap(), *this, "TrainingDummy1", tile, x, y, z, 0.0, false);
                case 2:
                    return new BuildingObject(getGameMap(), *this, "TrainingDummy2", tile, x, y, z, 0.0, false);
                case 3:
                    return new BuildingObject(getGameMap(), *this, "TrainingDummy3", tile, x, y, z, 0.0, false);
                case 4:
                    return new BuildingObject(getGameMap(), *this, "TrainingDummy4", tile, x, y, z, 0.0, false);
                default:
                    break;
            }
        }
        case ActiveSpotPlace::activeSpotLeft:
        {
            x -= OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 90.0, false);
        }
        case ActiveSpotPlace::activeSpotRight:
        {
            x += OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 270.0, false);
        }
        case ActiveSpotPlace::activeSpotTop:
        {
            y += OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 0.0, false);
        }
        case ActiveSpotPlace::activeSpotBottom:
        {
            y -= OFFSET_DUMMY;
            std::string meshName = Random::Int(1, 2) > 1 ? "WeaponShield2" : "WeaponShield1";
            return new BuildingObject(getGameMap(), *this, meshName, tile, x, y, z, 180.0, false);
        }
        default:
            break;
    }
    return nullptr;
}

void RoomTrainingHall::notifyActiveSpotRemoved(ActiveSpotPlace place, Tile* tile)
{
    Room::notifyActiveSpotRemoved(place, tile);

    for(std::pair<Creature* const,Tile*>& p : mCreaturesDummies)
    {
        Tile* tmpTile = p.second;
        if(tmpTile == tile)
        {
            Creature* creature = p.first;
            creature->clearActionQueue();

            // clearActionQueue should have released mCreaturesDummies[creature]. Now, we just need to release the unused dummy
            break;
        }
    }

    std::vector<Tile*>::iterator it = std::find(mUnusedDummies.begin(), mUnusedDummies.end(), tile);
    if(it == mUnusedDummies.end())
    {
        // Dummies are on center tiles only
        OD_ASSERT_TRUE_MSG(place != ActiveSpotPlace::activeSpotCenter, "name=" + getName() + ", tile=" + Tile::displayAsString(tile));
        return;
    }

    mUnusedDummies.erase(it);
}

void RoomTrainingHall::refreshCreaturesDummies()
{
    mCreaturesDummies.clear();
    mUnusedDummies.clear();
    nbTurnsNoChangeDummies = 0;

    mUnusedDummies.insert(mUnusedDummies.end(), mCentralActiveSpotTiles.begin(), mCentralActiveSpotTiles.end());

    if(mUnusedDummies.size() == 0 || mCreaturesUsingRoom.size() == 0)
        return;

    OD_ASSERT_TRUE_MSG(mUnusedDummies.size() >= mCreaturesUsingRoom.size(),
        "mUnusedDummies.size()=" + Helper::toString(static_cast<int>(mUnusedDummies.size()))
        + "mCreaturesUsingRoom.size()=" + Helper::toString(static_cast<int>(mCreaturesUsingRoom.size())));

    for(Creature* creature : mCreaturesUsingRoom)
    {
        int index = Random::Int(0, mUnusedDummies.size() - 1);
        Tile* tileDummy = mUnusedDummies[index];
        mUnusedDummies.erase(mUnusedDummies.begin() + index);
        mCreaturesDummies[creature] = tileDummy;

        // Set destination to the newly affected dummies if there was a change
        const Ogre::Vector3& creaturePosition = creature->getPosition();
        Ogre::Real wantedX = static_cast<Ogre::Real>(tileDummy->getX());
        Ogre::Real wantedY = static_cast<Ogre::Real>(tileDummy->getY()) - OFFSET_CREATURE;

        if(creaturePosition.x != wantedX ||
           creaturePosition.y != wantedY)
        {
            // We move to the good tile
            std::list<Tile*> pathToDummy = getGameMap()->path(creature, tileDummy);
            if(pathToDummy.empty())
            {
                OD_LOG_ERR("unexpected empty pathToDummy");
                continue;
            }

            std::vector<Ogre::Vector3> path;
            Creature::tileToVector3(pathToDummy, path, true, 0.0);
            // We add the last step to take account of the offset
            Ogre::Vector3 dest(wantedX, wantedY, 0.0);
            path.push_back(dest);
            creature->setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path, true);
        }
    }
}

bool RoomTrainingHall::hasOpenCreatureSpot(Creature* c)
{
    if (c->getLevel() >= ConfigManager::getSingleton().getRoomConfigUInt32("TrainHallMaxTrainingLevel"))
        return false;

    // We accept all creatures as soon as there are free dummies
    return mUnusedDummies.size() > 0;
}

bool RoomTrainingHall::addCreatureUsingRoom(Creature* creature)
{
    if(!Room::addCreatureUsingRoom(creature))
        return false;

    int index = Random::Int(0, mUnusedDummies.size() - 1);
    Tile* tileDummy = mUnusedDummies[index];
    mUnusedDummies.erase(mUnusedDummies.begin() + index);
    mCreaturesDummies[creature] = tileDummy;

    const Ogre::Vector3& creaturePosition = creature->getPosition();
    Ogre::Real wantedX = static_cast<Ogre::Real>(tileDummy->getX());
    Ogre::Real wantedY = static_cast<Ogre::Real>(tileDummy->getY()) - OFFSET_CREATURE;
    if(creaturePosition.x != wantedX ||
       creaturePosition.y != wantedY)
    {
        // We move to the good tile
        std::list<Tile*> pathToDummy = getGameMap()->path(creature, tileDummy);
        if(pathToDummy.empty())
        {
            OD_LOG_ERR("unexpected empty pathToDummy");
            return true;
        }

        std::vector<Ogre::Vector3> path;
        Creature::tileToVector3(pathToDummy, path, true, 0.0);
        // We add the last step to take account of the offset
        Ogre::Vector3 dest(wantedX, wantedY, 0.0);
        path.push_back(dest);
        creature->setWalkPath(EntityAnimation::walk_anim, EntityAnimation::idle_anim, true, true, path, true);
    }

    return true;
}

void RoomTrainingHall::removeCreatureUsingRoom(Creature* c)
{
    Room::removeCreatureUsingRoom(c);
    std::map<Creature*,Tile*>::iterator it = mCreaturesDummies.find(c);
    if(it == mCreaturesDummies.end())
    {
        OD_LOG_ERR("creature=" + c->getName() + ", room=" + getName());
        return;
    }

    Tile* tileDummy = it->second;
    if(tileDummy == nullptr)
    {
        OD_LOG_ERR("unexpected null tileDummy room=" + getName());
        return;
    }
    mUnusedDummies.push_back(tileDummy);
    mCreaturesDummies.erase(c);
}

void RoomTrainingHall::doUpkeep()
{
    Room::doUpkeep();

    if (mCoveredTiles.empty())
        return;

    // We add a probability to change dummies so that creatures do not use the same during too much time
    if(mCreaturesDummies.size() > 0 && Random::Int(50,150) < ++nbTurnsNoChangeDummies)
        refreshCreaturesDummies();
}

bool RoomTrainingHall::useRoom(Creature& creature, bool forced)
{
    std::map<Creature*,Tile*>::iterator it = mCreaturesDummies.find(&creature);
    if(it == mCreaturesDummies.end())
    {
        OD_LOG_ERR("creature=" + creature.getName() + ", room=" + getName());
        return false;
    }

    Tile* tileSpot = it->second;
    Tile* tileCreature = creature.getPositionTile();
    if(tileCreature == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName() + ", pos=" + Helper::toString(creature.getPosition()));
        return false;
    }

    Ogre::Real wantedX = static_cast<Ogre::Real>(tileSpot->getX());
    Ogre::Real wantedY = static_cast<Ogre::Real>(tileSpot->getY()) - OFFSET_CREATURE;

    BuildingObject* ro = getBuildingObjectFromTile(tileSpot);
    if(ro == nullptr)
    {
        OD_LOG_ERR("unexpected null building object");
        return false;
    }
    // We consider that the creature is in the good place if it is in the expected tile and not moving
    Tile* expectedDest = getGameMap()->getTile(Helper::round(wantedX), Helper::round(wantedY));
    if(expectedDest == nullptr)
    {
        OD_LOG_ERR("room=" + getName() + ", creature=" + creature.getName());
        return false;
    }

    if(tileCreature != expectedDest)
    {
        creature.setDestination(expectedDest);
        return false;
    }

    Ogre::Vector3 walkDirection(ro->getPosition().x - creature.getPosition().x, ro->getPosition().y - creature.getPosition().y, 0);
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
    ro->setAnimationState("Triggered", false);
    const CreatureRoomAffinity& creatureRoomAffinity = creature.getDefinition()->getRoomAffinity(getType());
    OD_ASSERT_TRUE_MSG(creatureRoomAffinity.getRoomType() == getType(), "name=" + getName() + ", creature=" + creature.getName()
        + ", creatureRoomAffinityType=" + Helper::toString(static_cast<int>(creatureRoomAffinity.getRoomType())));

    // We add a bonus per wall active spots
    double coef = 1.0 + static_cast<double>(mNumActiveSpots - mCentralActiveSpotTiles.size()) * ConfigManager::getSingleton().getRoomConfigDouble("TrainHallBonusWallActiveSpot");
    double expReceived = creatureRoomAffinity.getEfficiency() * ConfigManager::getSingleton().getRoomConfigDouble("TrainHallXpPerAttack");
    expReceived *= coef;

    creature.receiveExp(expReceived);
    creature.jobDone(ConfigManager::getSingleton().getRoomConfigDouble("TrainHallWakefulnessPerAttack"));
    creature.setJobCooldown(Random::Uint(ConfigManager::getSingleton().getRoomConfigUInt32("TrainHallCooldownHitMin"),
        ConfigManager::getSingleton().getRoomConfigUInt32("TrainHallCooldownHitMax")));

    return false;
}
