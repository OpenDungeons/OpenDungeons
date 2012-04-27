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
/*!
 * \file   GameState.h
 * \date   02 May 2011
 * \author oln
 * \brief  Header for class GameState
 */

#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <OgreSingleton.h>
#include <vector>
#include <AbstractGameState.h>

using std::vector;



/*! \brief Class to manage the state of the game. (if we are in the menu, gameMode etc.)
 *
 */
class GameStateManager :
    public Ogre::Singleton<GameStateManager>
{
public:
    enum ApplicationState {
        MENU,
        GAME,
        EDITOR
    };
    //add game-modes etc. here
    
    GameStateManager();

    ApplicationState getApplicationState(){return 
applicationState;}
    void setApplicationState(ApplicationState applicationState){this->applicationState = applicationState;}
    bool getIsServer(){return isServer;}
    void setIsServer(bool isServer){this->isServer = isServer;}
private:
    GameStateManager(const GameStateManager&);
    vector<AbstractGameState*> gameStateStack;
    bool isServer;
    ApplicationState applicationState;
};

#endif // GAMESTATE_H
