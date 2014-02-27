/*
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

#include "ModeManager.h"

#include "ModeContext.h"
#include "MenuMode.h"
#include "GameMode.h"
#include "EditorMode.h"
#include "ConsoleMode.h"
#include "FppMode.h"

ModeManager::ModeManager(GameMap* gameMap,MiniMap* miniMap, Console* console)
{
    mMc = new ModeContext(gameMap, miniMap);
    mModesArray[0]= new MenuMode(mMc);
    mModesArray[1]= new GameMode(mMc);
    mModesArray[2]= new EditorMode(mMc);
    mModesArray[3]= new ConsoleMode(mMc, console);
    mModesArray[4]= new FppMode(mMc);
    mModesStack.push(MENU);
    mModesArray[mModesStack.top()]->giveFocus();
}

ModeManager::~ModeManager()
{
    delete mModesArray[0];
    delete mModesArray[1];
    delete mModesArray[2];
    delete mModesArray[3];
    delete mModesArray[4];
    delete mMc;
}

AbstractApplicationMode* ModeManager::getCurrentMode()
{
    return mModesArray[mModesStack.top()] ;
}

AbstractApplicationMode* ModeManager:: progressMode(ModeType mm)
{
    mModesStack.push(mm);
    mModesArray[mModesStack.top()]->giveFocus();
    return mModesArray[mModesStack.top()];
}

AbstractApplicationMode* ModeManager::regressMode()
{
    if( mModesStack.size() > 1)
    {
        mModesStack.pop();
        mModesArray[mModesStack.top()]->giveFocus();
        return mModesArray[mModesStack.top()];
    }
    else
        return mModesArray[mModesStack.top()];
}

int ModeManager::lookForNewMode(){

    if(mMc->changed)
    {
        mMc->changed = false;

        if(mMc->nextMode == PREV)
            regressMode();
        else
            progressMode(mMc->nextMode);
        return 1;
    }
    else {
        return 0;
    }
}
