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
 * \file   GameState.cpp
 * \date   02 May 2011
 * \author oln
 * \brief  Implementation for class GameState
 */

#include "GameState.h"


template<> GameState* Ogre::Singleton<GameState>::ms_Singleton = 0;

GameState::GameState()
:
    isServer(false),
    applicationState(GameState::MENU)
{
    
}
