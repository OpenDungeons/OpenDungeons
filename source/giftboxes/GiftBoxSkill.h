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

#ifndef GIFTBOXRESEARCH_H
#define GIFTBOXRESEARCH_H

#include "entities/GiftBoxEntity.h"

#include <string>
#include <iosfwd>

enum class SkillType;

class GiftBoxSkill: public GiftBoxEntity
{
public:
    GiftBoxSkill(GameMap* gameMap, const std::string& baseName, SkillType skillType);
    GiftBoxSkill(GameMap* gameMap);

    virtual void applyEffect();

protected:
    virtual void exportToStream(std::ostream& os) const override;
    virtual bool importFromStream(std::istream& is) override;

private:
    SkillType mSkillType;
};

#endif // GIFTBOXRESEARCH_H
