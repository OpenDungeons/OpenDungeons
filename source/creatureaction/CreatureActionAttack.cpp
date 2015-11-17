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

#include "creatureaction/CreatureActionAttack.h"

#include "creatureskill/CreatureSkill.h"
#include "entities/Creature.h"
#include "entities/Tile.h"
#include "gamemap/GameMap.h"
#include "gamemap/Pathfinding.h"
#include "utils/LogManager.h"

CreatureActionAttack::CreatureActionAttack(Creature& creature, Tile& tileAttack, CreatureSkillData& skillData, GameEntity& entityAttack, bool ko) :
    CreatureAction(creature),
    mTileAttack(tileAttack),
    mSkillData(skillData),
    mEntityAttack(&entityAttack),
    mKo(ko)
{
    mEntityAttack->addGameEntityListener(this);
}

CreatureActionAttack::~CreatureActionAttack()
{
    if(mEntityAttack != nullptr)
        mEntityAttack->removeGameEntityListener(this);
}

std::function<bool()> CreatureActionAttack::action()
{
    return std::bind(&CreatureActionAttack::handleAttack,
        std::ref(mCreature), std::ref(mTileAttack), std::ref(mSkillData), mEntityAttack, mKo);
}

bool CreatureActionAttack::handleAttack(Creature& creature, Tile& tileAttack, CreatureSkillData& skillData,
        GameEntity* entityAttack, bool ko)
{
    // We always pop action to make sure next time we will try to find if a closest foe is there
    // or if we need to hit and run
    creature.popAction();

    // actionItem.mEntity can be nullptr if the entity died between the time we started to chase it and the time we strike
    if(entityAttack == nullptr)
        return true;

    // We check what we are attacking.

    // Turn to face the creature we are attacking and set the animation state to Attack.
    const Ogre::Vector3& pos = creature.getPosition();
    Ogre::Vector3 walkDirection(tileAttack.getX() - pos.x, tileAttack.getY() - pos.y, 0);
    walkDirection.normalise();
    creature.setAnimationState(EntityAnimation::attack_anim, false, walkDirection);
    creature.fireCreatureSound(CreatureSound::Attack);
    creature.setNbTurnsWithoutBattle(0);

    // Calculate how much damage we do.
    Tile* myTile = creature.getPositionTile();
    float range = Pathfinding::distanceTile(*myTile, tileAttack);

    // We use the skill
    skillData.mSkill->tryUseFight(*creature.getGameMap(), &creature, range,
        entityAttack, &tileAttack, ko);
    skillData.mWarmup = skillData.mSkill->getWarmupNbTurns();
    skillData.mCooldown = skillData.mSkill->getCooldownNbTurns();

    // Fighting is tiring
    creature.decreaseWakefulness(0.5);
    // but gives experience
    creature.receiveExp(1.5);

    return false;
}

std::string CreatureActionAttack::getListenerName() const
{
    return toString(getType()) + ", creature=" + mCreature.getName();
}

bool CreatureActionAttack::notifyDead(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionAttack::notifyRemovedFromGameMap(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionAttack::notifyPickedUp(GameEntity* entity)
{
    if(entity == mEntityAttack)
    {
        mEntityAttack = nullptr;
        return false;
    }
    return true;
}

bool CreatureActionAttack::notifyDropped(GameEntity* entity)
{
    // That should not happen. For now, we only require events for attacked creatures. And when they
    // are picked up, we should have cleared the action queue
    OD_LOG_ERR(toString(getType()) + ", creature=" + mCreature.getName() + ", entity=" + entity->getName());
    return true;
}
