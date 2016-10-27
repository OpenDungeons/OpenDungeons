/*
 *  Copyright (C) 2011-2016  OpenDungeons Team
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

#ifndef RESEARCH_H
#define RESEARCH_H

#include <cstdint>
#include <vector>

class ODPacket;
class Player;

enum class SkillType;

class Skill
{
public:
    Skill(SkillType type, int32_t neededSkillPoints, const std::vector<const Skill*>& skillDepends);
    virtual ~Skill()
    {}

    inline int32_t getNeededSkillPoints() const
    { return mNeededSkillPoints; }

    inline SkillType getType() const
    { return mType; }

    bool canBeSkilled(const std::vector<SkillType>& skillsDone) const;

    //! \brief Builds the dependency tree in dependencies with the skills required for this skill
    //! excluding skills already done in skillsDone including the skill itself. Note that the returned vector
    //! will contain needed skills ordered by feasible skills (the first one will have no needed dependencies,
    //! the second none other than the first, ...)
    //! Note 1: dependencies will not be added twice in dependencies even if there are 2 dependencies that depends
    //! on the same skill
    //! Note 2: if dependencies is not empty when calling this function, its content will be used like skills already
    //! done and not checked if available or correctly ordered. This function will only append additional required dependencies.
    void buildDependencies(const std::vector<SkillType>& skillsDone, std::vector<SkillType>& dependencies) const;

    //! \brief Calls dependsOn for each SkillType
    bool dependsOn(const std::vector<SkillType>& skills) const;

    //! \brief Returns true if the skill is or depends on the given type and false otherwise
    bool dependsOn(SkillType type) const;

private:
    SkillType mType;

    int32_t mNeededSkillPoints;

    //! \brief List of the skills to be available. If one of them is available,
    //! the Skill can be searched.
    std::vector<const Skill*> mSkillDepends;
};

#endif // RESEARCH_H
