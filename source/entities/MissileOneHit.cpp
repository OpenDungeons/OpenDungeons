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

#include "entities/MissileOneHit.h"

#include "entities/Building.h"
#include "gamemap/GameMap.h"
#include "network/ODPacket.h"
#include "utils/LogManager.h"
#include "utils/Random.h"

#include <iostream>

MissileOneHit::MissileOneHit(GameMap* gameMap, bool isOnServerMap, Seat* seat, const std::string& senderName, const std::string& meshName,
        const std::string& particleScript, const Ogre::Vector3& direction, double speed, double physicalDamage, double magicalDamage,
        double elementDamage, GameEntity* entityTarget, bool damageAllies, bool koEnemyCreature) :
    MissileObject(gameMap, isOnServerMap, seat, senderName, meshName, direction, speed, entityTarget, damageAllies, koEnemyCreature),
    mPhysicalDamage(physicalDamage),
    mMagicalDamage(magicalDamage),
    mElementDamage(elementDamage)
{
    if(!particleScript.empty())
    {
        MissileParticuleEffectClient* effect = new MissileParticuleEffectClient(nextParticleSystemsName(), particleScript, -1);
        mEntityParticleEffects.push_back(effect);
    }
}

MissileOneHit::MissileOneHit(GameMap* gameMap, bool isOnServerMap) :
        MissileObject(gameMap, isOnServerMap),
    mPhysicalDamage(0.0),
    mMagicalDamage(0.0),
    mElementDamage(0.0)
{
}

bool MissileOneHit::hitCreature(Tile* tile, GameEntity* entity)
{
    entity->takeDamage(this, 0.0, mPhysicalDamage, mMagicalDamage, mElementDamage, tile, getKoEnemyCreature());
    return false;
}

void MissileOneHit::hitTargetEntity(Tile* tile, GameEntity* entityTarget)
{
    entityTarget->takeDamage(this, 0.0, mPhysicalDamage, mMagicalDamage, mElementDamage, tile, getKoEnemyCreature());
}

MissileOneHit* MissileOneHit::getMissileOneHitFromStream(GameMap* gameMap, std::istream& is)
{
    MissileOneHit* obj = new MissileOneHit(gameMap, true);
    obj->importFromStream(is);
    return obj;
}

MissileOneHit* MissileOneHit::getMissileOneHitFromPacket(GameMap* gameMap, ODPacket& is)
{
    MissileOneHit* obj = new MissileOneHit(gameMap, false);
    obj->importFromPacket(is);
    return obj;
}

void MissileOneHit::exportToStream(std::ostream& os) const
{
    MissileObject::exportToStream(os);
    os << mPhysicalDamage << "\t";
    os << mMagicalDamage << "\t";
    os << mElementDamage << "\t";

    uint32_t nbEffects = mEntityParticleEffects.size();
    os << "\t" << nbEffects;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        os << "\t" << effect->mScript;
    }
}

bool MissileOneHit::importFromStream(std::istream& is)
{
    if(!MissileObject::importFromStream(is))
        return false;
    if(!(is >> mPhysicalDamage))
        return false;
    if(!(is >> mMagicalDamage))
        return false;
    if(!(is >> mElementDamage))
        return false;
    uint32_t nbEffects;
    if(!(is >> nbEffects))
        return false;
    while(nbEffects > 0)
    {
        --nbEffects;
        std::string effectScript;
        if(!(is >> effectScript))
            return false;
        MissileParticuleEffectClient* effect = new MissileParticuleEffectClient(nextParticleSystemsName(), effectScript, -1);
        mEntityParticleEffects.push_back(effect);
    }

    return true;
}

void MissileOneHit::exportToPacket(ODPacket& os, const Seat* seat) const
{
    MissileObject::exportToPacket(os, seat);
    uint32_t nbEffects = mEntityParticleEffects.size();
    os << nbEffects;
    for(EntityParticleEffect* effect : mEntityParticleEffects)
    {
        os << effect->mName;
        os << effect->mScript;
        os << effect->mNbTurnsEffect;
    }
}

void MissileOneHit::importFromPacket(ODPacket& is)
{
    MissileObject::importFromPacket(is);
    uint32_t nbEffects;
    OD_ASSERT_TRUE(is >> nbEffects);

    while(nbEffects > 0)
    {
        --nbEffects;

        std::string effectName;
        std::string effectScript;
        uint32_t nbTurns;
        OD_ASSERT_TRUE(is >> effectName >> effectScript >> nbTurns);
        MissileParticuleEffectClient* effect = new MissileParticuleEffectClient(effectName, effectScript, nbTurns);
        mEntityParticleEffects.push_back(effect);
    }
}
