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

#ifndef RESEARCH_H
#define RESEARCH_H

#include <cstdint>
#include <vector>
#include <string>

class ODPacket;
class Player;

enum class ResearchType
{
    nullResearchType,

    // Rooms
    roomTreasury,
    roomDormitory,
    roomHatchery,
    roomTrainingHall,
    roomLibrary,
    roomWorkshop,
    roomCrypt,
    roomPrison,
    roomBridgeWooden,
    roomBridgeStone,

    // Traps
    trapBoulder,
    trapCannon,
    trapSpike,
    trapDoorWooden,

    // Spells
    spellSummonWorker,
    spellCallToWar,
    spellCreatureDefense,
    spellCreatureExplosion,
    spellCreatureHaste,
    spellCreatureHeal,
    spellCreatureSlow,
    spellCreatureStrength,
    spellCreatureWeak,

    // This should be the last
    countResearch
};

ODPacket& operator<<(ODPacket& os, const ResearchType& type);
ODPacket& operator>>(ODPacket& is, ResearchType& type);
std::ostream& operator<<(std::ostream& os, const ResearchType& type);
std::istream& operator>>(std::istream& is, ResearchType& type);

class Research
{
public:
    Research(ResearchType type, int32_t neededResearchPoints, const std::vector<const Research*>& researchDepends);
    virtual ~Research()
    {}

    inline int32_t getNeededResearchPoints() const
    { return mNeededResearchPoints; }

    inline ResearchType getType() const
    { return mType; }

    bool canBeResearched(const std::vector<ResearchType>& researchesDone) const;

    //! \brief Builds the dependency tree in dependencies with the researches required for this research
    //! excluding researches already done in researchesDone including the research itself. Note that the returned vector
    //! will contain needed researches ordered by feasible researches (the first one will have no needed dependencies,
    //! the second none other than the first, ...)
    //! Note 1: dependencies will not be added twice in dependencies even if there are 2 dependencies that depends
    //! on the same research
    //! Note 2: if dependencies is not empty when calling this function, its content will be used like researches already
    //! done and not checked if available or correctly ordered. This function will only append additional required dependencies.
    void buildDependencies(const std::vector<ResearchType>& researchesDone, std::vector<ResearchType>& dependencies) const;

    //! \brief Calls dependsOn for each ResearchType
    bool dependsOn(const std::vector<ResearchType>& researches) const;

    //! \brief Returns true if the research is or depends on the given type and false otherwise
    bool dependsOn(ResearchType type) const;

    //! \brief The research name as used in level files.
    static std::string researchTypeToString(ResearchType type);

    //! \brief The research name as seen in game events.
    static std::string researchTypeToPlayerVisibleString(ResearchType type);
private:
    ResearchType mType;

    int32_t mNeededResearchPoints;

    //! \brief List of the researches to be available. If one of them is available,
    //! the Research can be searched.
    std::vector<const Research*> mResearchDepends;
};

#endif // RESEARCH_H
