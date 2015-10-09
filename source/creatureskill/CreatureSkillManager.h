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

#ifndef CREATURESKILLMANAGER_H
#define CREATURESKILLMANAGER_H

#include <cstdint>
#include <iosfwd>
#include <string>

class CreatureSkill;

//! \brief Factory class to register a new mood modifier
class CreatureSkillFactory
{
public:
    virtual ~CreatureSkillFactory()
    {}

    virtual CreatureSkill* createCreatureSkill() const = 0;

    virtual const std::string& getCreatureSkillName() const = 0;
};

class CreatureSkillManager
{
friend class CreatureSkillRegister;

public:
    CreatureSkillManager()
    {}

    virtual ~CreatureSkillManager()
    {}

    static CreatureSkill* clone(const CreatureSkill* skill);
    static CreatureSkill* load(std::istream& is);
    //! \brief Handles the Skill deletion
    static void dispose(const CreatureSkill* skill);
    static void write(const CreatureSkill& skill, std::ostream& os);
    static void getFormatString(const CreatureSkill& skill, std::string& format);
    static bool areEqual(const CreatureSkill& skill1, const CreatureSkill& skill2);

private:
    static void registerFactory(const CreatureSkillFactory* factory);
    static void unregisterFactory(const CreatureSkillFactory* factory);
};

class CreatureSkillRegister
{
public:
    CreatureSkillRegister(const CreatureSkillFactory* factoryToRegister) :
        mCreatureSkillFactory(factoryToRegister)
    {
        CreatureSkillManager::registerFactory(mCreatureSkillFactory);
    }
    ~CreatureSkillRegister()
    {
        CreatureSkillManager::unregisterFactory(mCreatureSkillFactory);
        delete mCreatureSkillFactory;
    }

private:
    const CreatureSkillFactory* mCreatureSkillFactory;
};

#endif // CREATURESKILLMANAGER_H
