/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "NullAI.h"
#include "AIWrapper.h"

#include "AIManager.h"

AIManager::AIManager(GameMap& gameMap)
    : gameMap(gameMap)
{

}

bool AIManager::assignAI(Player& player, AIManager::AIType aiType, const std::string& params)
{
    //NOTE: These factory functions could probably be done in a more elegang manner
    BaseAI* ai = NULL;
    switch(aiType)
    {
        case(nullAI):
        {
            ai = new NullAI(gameMap, player, aiType, params);
            break;
        }
        case(testAI):
        {
            ai = new NullAI(gameMap, player, aiType, params);
            break;
        }
        default:
        {
            break;
        }
    }
    if(ai == NULL)
    {
        return false;
    }
    else
    {
        aiList.push_back(ai);
        return true;
    }
}

bool AIManager::doTurn(double frameTime)
{
    for(AIList::iterator it = aiList.begin(); it != aiList.end(); ++it)
    {
        it->get()->doTurn(frameTime);
    }
    return true;
}

void AIManager::clearAIList()
{
    for(unsigned int i = 0; i < aiList.size(); ++i)
    {
        delete aiList[i];
    }
    aiList.clear();
}

/*
AIManager::AIManager(const AIManager& other)
{

}*/

AIManager::~AIManager()
{
    clearAIList();
}

AIManager& AIManager::operator=(const AIManager& other)
{
    return *this;
}

