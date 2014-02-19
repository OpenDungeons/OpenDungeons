/*!
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#ifndef ABSTRACTBASEFACTORY_H
#define ABSTRACTBASEFACTORY_H

#include <string>
#include <map>
#include <vector>
#include <cstddef>
#include "BaseAI.h"

class AIFactory
{
public:

    typedef BaseAI* (*CreateAIFunc)(GameMap&, Player&, const std::string&);

    static BaseAI* createInstance(const std::string& className, GameMap& gameMap,
                                  Player& player, const std::string& parameters = std::string())
    {
        if (mTypeMap == NULL)
            return NULL;

        std::map<std::string, CreateAIFunc>::iterator it = mTypeMap->find(className);
        if(it != mTypeMap->end()) {
            return ((*it).second)(gameMap, player, parameters);
        }
        return NULL;
    }


private:
    template <typename T> friend class AIFactoryRegister;

    template <typename D>
    static BaseAI* createAI(GameMap& gameMap, Player& player,
                            const std::string& parameters = std::string())
    {
        return new D(gameMap, player, parameters);
    }

    static std::map<std::string, CreateAIFunc>& getMap()
    {
        if(!mTypeMap)
        {
            mTypeMap = new std::map<std::string, CreateAIFunc>();
        }
        return *mTypeMap;
    }

    static std::map<std::string, CreateAIFunc>* mTypeMap;
};

template <typename T>
class AIFactoryRegister
{
public:
    AIFactoryRegister(const std::string& name)
    {
        std::pair<std::string, AIFactory::CreateAIFunc> p =
            std::make_pair<std::string, AIFactory::CreateAIFunc>(std::string(name), &AIFactory::createAI<T>);
        AIFactory::getMap().insert(p);
    }

private:
    AIFactoryRegister(const AIFactoryRegister&);
};

#endif // ABSTRACTBASEFACTORY_H
