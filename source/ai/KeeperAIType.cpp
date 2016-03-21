/*!
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

#include "ai/KeeperAIType.h"

#include "utils/Helper.h"
#include "utils/LogManager.h"

namespace KeeperAITypes
{
KeeperAIType fromString(const std::string& type)
{
    for(uint32_t i = 0; i < static_cast<uint32_t>(KeeperAIType::nbAI); ++i)
    {
        KeeperAIType t = static_cast<KeeperAIType>(i);
        if(type.compare(toString(t)) == 0)
            return t;
    }
    OD_LOG_ERR("Asked wrong value for AI type=" + type);
    return KeeperAIType::nbAI;
}

const std::string& toString(KeeperAIType type)
{
    switch(type)
    {
        case KeeperAIType::easy:
        {
            static const std::string str = "easy";
            return str;
        }
        case KeeperAIType::normal:
        {
            static const std::string str = "normal";
            return str;
        }
        default:
            break;
    }
    OD_LOG_ERR("Wrong enum value=" + Helper::toString(static_cast<uint32_t>(type)));
    static const std::string str = "Wrong value";
    return str;
}

const std::string& toDisplayableString(KeeperAIType type)
{
    switch(type)
    {
        case KeeperAIType::easy:
        {
            static const std::string str = "AI Easy";
            return str;
        }
        case KeeperAIType::normal:
        {
            static const std::string str = "AI Normal";
            return str;
        }
        default:
            break;
    }
    OD_LOG_ERR("Wrong enum value=" + KeeperAITypes::toString(type));
    static const std::string str = "AI Wrong value";
    return str;
}
} //namespace KeeperAITypes
