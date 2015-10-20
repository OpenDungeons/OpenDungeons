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

#ifndef CREATUREBEHAVIOURMANAGER_H
#define CREATUREBEHAVIOURMANAGER_H

#include <cstdint>
#include <iosfwd>
#include <string>

class CreatureBehaviour;

//! \brief Factory class to register a new mood modifier
class CreatureBehaviourFactory
{
public:
    virtual ~CreatureBehaviourFactory()
    {}

    virtual CreatureBehaviour* createCreatureBehaviour() const = 0;

    virtual const std::string& getCreatureBehaviourName() const = 0;
};

class CreatureBehaviourManager
{
friend class CreatureBehaviourRegister;

public:
    CreatureBehaviourManager()
    {}

    virtual ~CreatureBehaviourManager()
    {}

    static CreatureBehaviour* clone(const CreatureBehaviour* behaviour);
    static CreatureBehaviour* load(std::istream& is);
    //! \brief Handles the behaviour deletion
    static void dispose(const CreatureBehaviour* behaviour);
    static void write(const CreatureBehaviour& behaviour, std::ostream& os);
    static void getFormatString(const CreatureBehaviour& behaviour, std::string& format);
    static bool areEqual(const CreatureBehaviour& behaviour1, const CreatureBehaviour& behaviour2);

private:
    static void registerFactory(const CreatureBehaviourFactory* factory);
    static void unregisterFactory(const CreatureBehaviourFactory* factory);
};

class CreatureBehaviourRegister
{
public:
    CreatureBehaviourRegister(const CreatureBehaviourFactory* factoryToRegister) :
        mCreatureBehaviourFactory(factoryToRegister)
    {
        CreatureBehaviourManager::registerFactory(mCreatureBehaviourFactory);
    }
    ~CreatureBehaviourRegister()
    {
        CreatureBehaviourManager::unregisterFactory(mCreatureBehaviourFactory);
        delete mCreatureBehaviourFactory;
    }

private:
    const CreatureBehaviourFactory* mCreatureBehaviourFactory;
};

#endif // CREATUREBEHAVIOURMANAGER_H
