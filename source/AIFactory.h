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

    static BaseAI* createInstance(const std::string& className, GameMap& gameMap, Player& player, const std::string& parameters = "")
    {
        std::map<std::string, CreateAIFunc>::iterator it = typeMap->find(className);
        if(it != typeMap->end()) {
            return ((*it).second)(gameMap, player, parameters);
        }
        return NULL;
    }


private:
     template <typename T> friend class AIFactoryRegister;

    template <typename D>
    static BaseAI* createAI(GameMap& gameMap, Player& player, const std::string& parameters = "")
    {
        return new D(gameMap, player, parameters);
    }

    static std::map<std::string, CreateAIFunc>& getMap()
    {
        if(!typeMap)
        {
            typeMap = new std::map<std::string, CreateAIFunc>();
        }
        return *typeMap;
    }

    //typedef std::map<std::string, CreateObjectFunc> FuncMap;

    //AbstractBaseFactory();
    //AbstractBaseFactory(const AbstractBaseFactory& other);
    static std::map<std::string, CreateAIFunc>* typeMap;
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
