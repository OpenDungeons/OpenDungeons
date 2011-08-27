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


#ifndef BASEAI_H
#define BASEAI_H

#include <OgreSharedPtr.h>
#include <string>

#include "AIWrapper.h"
#include "AIManager.h"

class BaseAI
{
public:
    BaseAI(GameMap& gameMap, Player& player, const std::string& parameters = "");
     /** \brief This is the function that will be called each turn for the ai.
     *  This is the function that will be called each turn for the ai.
     *  For custom AI's this should be overridden and return true on a
     *  successful call.
     */
    virtual bool doTurn(double frameTime) = 0;
    virtual inline AIManager::AIType getType() const {return aiType;}
    virtual ~BaseAI();
protected:
    AIWrapper aiWrapper;
    virtual bool initialize(const std::string& parameters);
private:
    BaseAI(const BaseAI& other);
    
    //virtual BaseAI& operator=(const BaseAI& other);
    AIManager::AIType aiType;
};

#endif // BASEAI_H
