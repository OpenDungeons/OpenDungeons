#include "entities/GameEntityType.h"

#include "network/ODPacket.h"
#include "utils/LogManager.h"

#include <istream>
#include <ostream>

ODPacket& operator<<(ODPacket& os, const GameEntityType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

ODPacket& operator>>(ODPacket& is, GameEntityType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<GameEntityType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const GameEntityType& type)
{
    os << static_cast<int32_t>(type);
    return os;
}

std::istream& operator>>(std::istream& is, GameEntityType& type)
{
    int32_t tmp;
    OD_ASSERT_TRUE(is >> tmp);
    type = static_cast<GameEntityType>(tmp);
    return is;
}
