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

class Skill;

enum class SkillType;
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

//! \brief Each skill is in one of these families
enum class SkillFamily
{
    rooms,
    traps,
    spells,
    nb
};

//! \brief Local class that will be used to store Skill data and link it to the gui
class SkillDef;

class SkillManager
{
public:
    SkillManager();
    virtual ~SkillManager();

    //! \brief Checks if the given room is available for the given seat. This check
    //! should be done on server side to avoid cheating
    static bool isRoomAvailable(RoomType type, const Seat* seat);

    //! \brief Checks if the given spell is available for the given seat. This check
    //! should be done on server side to avoid cheating
    static bool isSpellAvailable(SpellType type, const Seat* seat);

    //! \brief Checks if the given trap is available for the given trap. This check
    //! should be done on server side to avoid cheating
    static bool isTrapAvailable(TrapType type, const Seat* seat);

    //! \brief Checks if the given seat has already researched all the allowed skills
    static bool isAllSkillsDoneForSeat(const Seat* seat);

    //! \brief Builds randomly a pending skill list for the given seat. Note that the
    //! given pending vector may not be empty. In this case, it will be filled by the
    //! not already selected skills
    //! Note that this function will only use the seat for knowing already done or not allowed
    //! skills, not the currently pending ones. If they are to be used, skills should
    //! be initialized with them
    static void buildRandomPendingSkillsForSeat(std::vector<SkillType>& skills,
        const Seat* seat);

    static const Skill* getSkill(SkillType resType);

    //! \brief Lists all the skills and calls the given function for each one with parameters:
    //! - Gui skill button name
    //! - Gui use button name
    //! - Gui skill button progressbar name
    //! - The corresponding skill type
    static void listAllSkills(const std::function<void(const std::string&, const std::string&,
        const std::string&, SkillType)>& func);

    //! \brief Lists all the skills and calls the given function for each one with parameters:
    //! - SpellType
    //! - Gui use button progressbar name
    static void listAllSpellsProgressBars(const std::function<void(SpellType spellType, const std::string&)>& func);

    static void connectSkills(GameMode* mode, CEGUI::Window* rootWindow);

    static void connectGuiButtons(GameEditorModeBase* mode, CEGUI::Window* rootWindow, PlayerSelection& playerSelection);

private:
    //! \brief Allowed skills
    std::vector<const SkillDef*> mSkills;

    //! \brief Allows to map each family room/trap/spell to the corresponding SkillType
    std::vector<std::vector<SkillType>> mSkillsFamily;
};

#endif // RESEARCHMANAGER_H
