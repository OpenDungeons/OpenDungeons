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

#ifndef GAMEEDITORMODEBASE_H
#define GAMEEDITORMODEBASE_H

#include "modes/AbstractApplicationMode.h"

#include "gamemap/MiniMap.h"

namespace CEGUI
{
    class Window;
}

enum class RoomType;
enum class TrapType;

//! \brief Class containing common functionality between editor and game modes.
class GameEditorModeBase : public AbstractApplicationMode
{
public:
    GameEditorModeBase(ModeManager *modeManager, ModeManager::ModeType modeType, CEGUI::Window* rootWindow);
    virtual void activate() = 0;
protected:
    void connectGuiAction(const std::string& buttonName, AbstractApplicationMode::GuiAction action);

    //! \brief gets a game entity from the corresponding ogre name
    GameEntity* getEntityFromOgreName(const std::string& entityName);

    CEGUI::Window* mRootWindow;

    //! \brief A reference to the game map used by the game mode
    //! For now, handled by the frame listener, don't delete it from here.
    GameMap* mGameMap;

    //! \brief The minimap used in this mode
    MiniMap mMiniMap;
private:
    //! \brief Minimap click event handler
    bool onMinimapClick(const CEGUI::EventArgs& arg);

    //helper functions to connect buttons to entities
    void connectRoomSelect(const std::string& buttonName, RoomType roomType);
    void connectTrapSelect(const std::string& buttonName, TrapType trapType);
};

#endif // GAMEEDITORMODEBASE_H
