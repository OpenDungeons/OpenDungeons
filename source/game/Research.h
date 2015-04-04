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

    // Traps
    trapBoulder,
    trapCannon,
    trapSpike,

    // Spells
    spellSummonWorker,
    spellCallToWar,

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
