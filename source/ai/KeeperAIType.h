/*!
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

#ifndef KEEPERAITYPE_H
#define KEEPERAITYPE_H

#include <string>

enum class KeeperAIType
{
   easy,
   normal,
   nbAI              // Must be the last in this enum
};

namespace KeeperAITypes
{
    KeeperAIType fromString(const std::string& type);
    const std::string& toString(KeeperAIType type);
    const std::string& toDisplayableString(KeeperAIType type);
}


#endif // KEEPERAITYPE_H
