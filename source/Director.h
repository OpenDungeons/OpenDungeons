/*!
 * \file   GameState.h
 * \date   02 May 2011
 * \author oln paul424
 *
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
 *
 * \brief  Header for class Director
 * This is supposedly the core of GamePlay, it should load Scenario
 * (an overset of GameMap level file) and direct the gameplay
 * as the scenario goes. It should also load and unload the resources as needed
 * when swaping between Scenarios.
 *
 * Most of the interaction and front objects from the end-user perspective
 * should go into hear, that is Opponent AI, Game Rules, Input  Modes, ect.
 *
 * It should  AVOID low level tinkertoy.
 * If we need some Resource wrapper is to be considered,
 * Ogre::ResourceManager would not trace all resources to run a gameplay.
 * It Scenario should be keept in the list/set of resources needed.
 *
 * Then in the language of sets ResourcestoUnload = OldScenarioResources \
 * NewScenarioResources.
 * ResourcesToLoad = NewScenarioResources \ OldScenarioResources.
 * And of course new Director starts with the empty set of resources.
 * Consider what else could go in here: GameBriefings, GamePlaysSummary,
 * Game in games cinematics :P ok ok too much of that ;)
 *
 * The Api is just a proposition, the exact method are to be established :)
 */

#ifndef DIRECTOR_H
#define DIRECTOR_H

#include <Goal.h>
#include <GameMap.h>
//#include <Scenario.h>
#include <OgreSingleton.h>
#include <AbstractApplicationMode.h>

#include <vector>
#include <string>
#include <set>
#include <stack>

#include <boost/shared_ptr.hpp>

//TODO
class Resource
{};
class Scenario
{};

class Director : public Ogre::Singleton<Director>
{
public:

    enum ApplicationModeId
    {
        MENU = 0,
        GAME,
        EDITOR,
        CONSOLE,
        FPP
    };

    Director();
    ~Director();

    // TODO: add game-modes etc. here

    inline AbstractApplicationMode& getCurrentState()
    {
        return *mGameStateStack.top();
    }

    inline ApplicationModeId getApplicationModeId() const
    {
        return mApplicationModeId;
    }

    inline void setApplicationState(ApplicationModeId applicationModeId)
    {
        mApplicationModeId = applicationModeId;
    }

    bool getIsServer() const
    {
        return mIsServer;
    }

    void setIsServer(bool isServer)
    {
        mIsServer = isServer;
    }

    int playNextScenario();
    int playScenario(int ss);
    int addScenario(const std::string& scenarioFileName);
    int addScenario(const std::string& scenarioFileName, int ss);
    int removeScenario();
    int removeScenario(int ss);
    int clearScenarios();

private:
    std::stack<AbstractApplicationMode*> mGameStateStack;
    std::vector<boost::shared_ptr<AbstractApplicationMode> > mGameStates;
    bool mIsServer;
    ApplicationModeId mApplicationModeId;

    std::vector<Scenario> mCurrentScenarios;
    std::set<Resource> mCurrentResources;

    int unloadResources();
    int loadResources();
};

#endif  // DIRECTOR_H
