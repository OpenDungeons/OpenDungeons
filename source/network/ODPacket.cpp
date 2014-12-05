/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
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

#include "network/ODPacket.h"

#define OD_INT64TOINT32H(valInt64)              (static_cast<int32_t>(valInt64 >> 32))
#define OD_INT64TOINT32L(valInt64)              (static_cast<int32_t>(valInt64))
#define OD_INT32TOINT64(valInt32h,valInt32l)    ((((static_cast<int64_t>(valInt32h)) << 32) & static_cast<int64_t>(0xFFFFFFFF00000000)) + ((static_cast<int64_t>(valInt32l)) & static_cast<int64_t>(0x00000000FFFFFFFF)))

ODPacket& ODPacket::operator >>(bool& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(int8_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(uint8_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(int16_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(uint16_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(int32_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(uint32_t& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(int64_t& data)
{
    // TODO : currently, SFML 2.1 do not handle int64. We do it ourselves
    int32_t dataH;
    int32_t dataL;
    mPacket>>dataH;
    mPacket>>dataL;
    data = OD_INT32TOINT64(dataH,dataL);
    return *this;
}

ODPacket& ODPacket::operator >>(uint64_t& data)
{
    // TODO : currently, SFML 2.1 do not handle int64. We do it ourselves
    uint32_t dataH;
    uint32_t dataL;
    mPacket>>dataH;
    mPacket>>dataL;
    data = OD_INT32TOINT64(dataH,dataL);
    return *this;
}

ODPacket& ODPacket::operator >>(float& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(double& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(char* data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(std::string& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(wchar_t* data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(std::wstring& data)
{
    mPacket>>data;
    return *this;
}

ODPacket& ODPacket::operator >>(Ogre::Vector3& data)
{
    mPacket >> data.x >> data.y >> data.z;
    return *this;
}
ODPacket& ODPacket::operator <<(bool data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(int8_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(uint8_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(int16_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(uint16_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(int32_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(uint32_t data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(int64_t data)
{
    // TODO : currently, SFML 2.1 do not handle int64. We do it ourselves
    int32_t dataH = OD_INT64TOINT32H(data);
    int32_t dataL = OD_INT64TOINT32L(data);
    mPacket<<dataH;
    mPacket<<dataL;
    return *this;
}

ODPacket& ODPacket::operator <<(uint64_t data)
{
    // TODO : currently, SFML 2.1 do not handle int64. We do it ourselves
    int32_t dataH = OD_INT64TOINT32H(data);
    int32_t dataL = OD_INT64TOINT32L(data);
    mPacket<<dataH;
    mPacket<<dataL;
    return *this;
}

ODPacket& ODPacket::operator <<(float data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(double data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(const char* data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(const std::string& data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(const wchar_t* data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(const std::wstring& data)
{
    mPacket<<data;
    return *this;
}

ODPacket& ODPacket::operator <<(const Ogre::Vector3&   data)
{
    mPacket << data.x << data.y << data.z;
    return *this;
}

ODPacket::operator bool() const
{
    return mPacket;
}

void ODPacket::clear()
{
    mPacket.clear();
}

void ODPacket::writePacket(int32_t timestamp, std::ofstream& os)
{
    int32_t bufferSize = mPacket.getDataSize();
    const char* buffer = static_cast<const char*>(mPacket.getData());
    os.write(reinterpret_cast<const char*>(&timestamp), sizeof(int32_t));
    os.write(reinterpret_cast<const char*>(&bufferSize), sizeof(int32_t));
    os.write(buffer, bufferSize);
}

const int32_t BUFFER_SIZE = 1024;

int32_t ODPacket::readPacket(std::ifstream& is)
{
    int32_t timestamp;
    int32_t packetSize;

    is.read(reinterpret_cast<char*>(&timestamp), sizeof(int32_t));
    if(is.eof())
        return -1;

    is.read(reinterpret_cast<char*>(&packetSize), sizeof(int32_t));
    if(is.eof())
        return -1;

    mPacket.clear();
    char buffer[BUFFER_SIZE];
    while(packetSize > 0)
    {
        int32_t sizeToRead = std::min(packetSize, BUFFER_SIZE);
        is.read(buffer, sizeToRead);
        mPacket.append(buffer, sizeToRead);
        packetSize -= sizeToRead;
    }

    return timestamp;
}
