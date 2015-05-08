#include "rooms/RoomType.h"

#include "network/ODPacket.h"
#include "utils/Helper.h"

#include <istream>
#include <ostream>

std::istream& operator>>(std::istream& is, RoomType& rt)
{
  uint32_t tmp;
  is >> tmp;
  rt = static_cast<RoomType>(tmp);
  return is;
}

std::ostream& operator<<(std::ostream& os, const RoomType& rt)
{
  uint32_t tmp = static_cast<uint32_t>(rt);
  os << tmp;
  return os;
}

ODPacket& operator>>(ODPacket& is, RoomType& rt)
{
  uint32_t tmp;
  is >> tmp;
  rt = static_cast<RoomType>(tmp);
  return is;
}

ODPacket& operator<<(ODPacket& os, const RoomType& rt)
{
  uint32_t tmp = static_cast<uint32_t>(rt);
  os << tmp;
  return os;
}
