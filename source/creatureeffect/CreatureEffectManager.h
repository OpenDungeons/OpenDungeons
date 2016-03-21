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

#ifndef CREATUREEFFECTMANAGER_H
#define CREATUREEFFECTMANAGER_H

#include <cstdint>
#include <iosfwd>
#include <string>

class CreatureEffect;

//! \brief Factory class to register a new modifier
class CreatureEffectFactory
{
public:
    virtual ~CreatureEffectFactory()
    {}

    virtual CreatureEffect* createCreatureEffect() const = 0;

    virtual const std::string& getCreatureEffectName() const = 0;
};

class CreatureEffectManager
{
friend class CreatureEffectRegister;

public:
    CreatureEffectManager()
    {}

    virtual ~CreatureEffectManager()
    {}

    static CreatureEffect* load(std::istream& defFile);
    static void write(const CreatureEffect& effect, std::ostream& os);

private:
    static void registerFactory(const CreatureEffectFactory* factory);
    static void unregisterFactory(const CreatureEffectFactory* factory);
};

class CreatureEffectRegister
{
public:
    CreatureEffectRegister(const CreatureEffectFactory* factoryToRegister) :
        mCreatureEffectFactory(factoryToRegister)
    {
        CreatureEffectManager::registerFactory(mCreatureEffectFactory);
    }
    ~CreatureEffectRegister()
    {
        CreatureEffectManager::unregisterFactory(mCreatureEffectFactory);
        delete mCreatureEffectFactory;
    }

private:
    const CreatureEffectFactory* mCreatureEffectFactory;
};

#endif // CREATUREEFFECTMANAGER_H
