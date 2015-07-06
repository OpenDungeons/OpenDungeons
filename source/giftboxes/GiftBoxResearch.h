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

#ifndef GIFTBOXRESEARCH_H
#define GIFTBOXRESEARCH_H

#include "entities/GiftBoxEntity.h"

#include <string>
#include <iosfwd>

enum class ResearchType;

class GiftBoxResearch: public GiftBoxEntity
{
public:
    GiftBoxResearch(GameMap* gameMap, bool isOnServerMap, const std::string& baseName, ResearchType researchType);
    GiftBoxResearch(GameMap* gameMap, bool isOnServerMap);

    virtual void exportToStream(std::ostream& os) const override;
    virtual void importFromStream(std::istream& is) override;

    virtual void applyEffect();

private:
    ResearchType mResearchType;
};

#endif // GIFTBOXRESEARCH_H
