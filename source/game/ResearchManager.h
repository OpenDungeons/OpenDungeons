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

#ifndef RESEARCHMANAGER_H
#define RESEARCHMANAGER_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class Research;

enum class ResearchType;
enum class RoomType;
enum class SpellType;
enum class TrapType;

class GameEditorModeBase;
class GameMode;
class PlayerSelection;
class Seat;

namespace CEGUI
{
class Window;
};

//! \brief Each research is in one of these families
enum class ResearchFamily
{
    rooms,
    traps,
    spells,
    nb
};

//! \brief Local class that will be used to store Research data and link it to the gui
class ResearchDef;

class ResearchManager
{
public:
    ResearchManager();
    virtual ~ResearchManager();

    //! \brief Checks if the given room is available for the given seat. This check
    //! should be done on server side to avoid cheating
    static bool isRoomAvailable(RoomType type, const Seat* seat);

    //! \brief Checks if the given spell is available for the given seat. This check
    //! should be done on server side to avoid cheating
    static bool isSpellAvailable(SpellType type, const Seat* seat);

    //! \brief Checks if the given trap is available for the given trap. This check
    //! should be done on server side to avoid cheating
    static bool isTrapAvailable(TrapType type, const Seat* seat);

    static const Research* getResearch(ResearchType resType);

    static void listAllResearches(const std::function<void(const std::string&, const std::string&,
        const std::string&, ResearchType)>& func);

    static void connectResearches(GameMode* mode, CEGUI::Window* rootWindow);

    static void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection);

private:
    //! \brief Allowed researches
    std::vector<const ResearchDef*> mResearches;

    //! \brief Allows to map each family room/trap/spell to the corresponding ResearchType
    std::vector<std::vector<ResearchType>> mResearchesFamily;
};

#endif // RESEARCHMANAGER_H
