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

#ifndef AIMANAGER_H
#define AIMANAGER_H

#include <map>
#include <list>

class BaseAI;
class GameMap;

class AIManager
{

public:
    enum AIType
    {
        invalidAI,
        nullAI,
        testAI
    };
    typedef std::list<BaseAI*> AIList;

    AIManager(GameMap& gameMap);
    virtual ~AIManager();

    bool assignAI(Player& player, const std::string& type, const std::string& params = std::string());
    bool doTurn(double frameTime);
    void clearAIList();

private:
    virtual AIManager& operator=(const AIManager& other);

    GameMap& gameMap;
    AIList aiList;
};

#endif // AIMANAGER_H
