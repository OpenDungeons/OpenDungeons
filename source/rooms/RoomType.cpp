#include "rooms/RoomType.h"

#include "utils/Helper.h"

#include <istream>
#include <ostream>

namespace Rooms
{
std::string getRoomNameFromRoomType(RoomType t)
{
    switch (t)
    {
    case RoomType::nullRoomType:
        return "NullRoomType";

    case RoomType::dungeonTemple:
        return "DungeonTemple";

    case RoomType::dormitory:
        return "Dormitory";

    case RoomType::treasury:
        return "Treasury";

    case RoomType::portal:
        return "Portal";

    case RoomType::workshop:
        return "Workshop";

    case RoomType::trainingHall:
        return "TrainingHall";

    case RoomType::library:
        return "Library";

    case RoomType::hatchery:
        return "Hatchery";

    case RoomType::crypt:
        return "Crypt";

    default:
        return "UnknownRoomType enum=" + Helper::toString(static_cast<int>(t));
    }
}

RoomType getRoomTypeFromRoomName(const std::string& name)
{
    if(name.compare("DungeonTemple") == 0)
        return RoomType::dungeonTemple;

    if(name.compare("Dormitory") == 0)
        return RoomType::dormitory;

    if(name.compare("Treasury") == 0)
        return RoomType::treasury;

    if(name.compare("Portal") == 0)
        return RoomType::portal;

    if(name.compare("Workshop") == 0)
        return RoomType::workshop;

    if(name.compare("TrainingHall") == 0)
        return RoomType::trainingHall;

    if(name.compare("Library") == 0)
        return RoomType::library;

    if(name.compare("Hatchery") == 0)
        return RoomType::hatchery;

    if(name.compare("Crypt") == 0)
        return RoomType::crypt;

    return RoomType::nullRoomType;
}
} //namespace Rooms

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
