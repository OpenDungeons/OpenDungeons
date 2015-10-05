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

#ifndef CREATUREMOODMANAGER_H
#define CREATUREMOODMANAGER_H

#include <cstdint>
#include <iosfwd>
#include <string>

class CreatureMood;

enum class CreatureMoodType;

//! \brief Factory class to register a new mood modifier
class CreatureMoodFactory
{
public:
    virtual ~CreatureMoodFactory()
    {}

    virtual CreatureMood* createCreatureMood() const = 0;

    virtual CreatureMoodType getCreatureMoodType() const = 0;

    virtual const std::string& getCreatureMoodName() const = 0;
};

class CreatureMoodManager
{
friend class CreatureMoodRegister;

public:
    CreatureMoodManager()
    {}

    virtual ~CreatureMoodManager()
    {}

    static CreatureMood* load(std::istream& defFile);

private:
    static void registerFactory(const CreatureMoodFactory* factory);
    static void unregisterFactory(const CreatureMoodFactory* factory);
};

class CreatureMoodRegister
{
public:
    CreatureMoodRegister(const CreatureMoodFactory* factoryToRegister) :
        mCreatureMoodFactory(factoryToRegister)
    {
        CreatureMoodManager::registerFactory(mCreatureMoodFactory);
    }
    ~CreatureMoodRegister()
    {
        CreatureMoodManager::unregisterFactory(mCreatureMoodFactory);
        delete mCreatureMoodFactory;
    }

private:
    const CreatureMoodFactory* mCreatureMoodFactory;
};

#endif // CREATUREMOODMANAGER_H
