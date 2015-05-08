#include "SpellType.h"

#include "network/ODPacket.h"

#include <istream>
#include <ostream>

std::istream& operator>>(std::istream& is, SpellType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<SpellType>(tmp);
    return is;
}

std::ostream& operator<<(std::ostream& os, const SpellType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}

ODPacket& operator>>(ODPacket& is, SpellType& tt)
{
    uint32_t tmp;
    is >> tmp;
    tt = static_cast<SpellType>(tmp);
    return is;
}

ODPacket& operator<<(ODPacket& os, const SpellType& tt)
{
    uint32_t tmp = static_cast<uint32_t>(tt);
    os << tmp;
    return os;
}
