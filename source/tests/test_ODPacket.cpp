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

#define BOOST_TEST_MODULE ODPacket
#include "BoostTestTargetConfig.h"

#include "network/ODPacket.h"

BOOST_AUTO_TEST_CASE(test_ODPacket)
{
    //Test input/output
    {
        ODPacket packet;
        const int32_t inInt = 22;
        packet << inInt;
        int32_t outInt = 0;
        packet >> outInt;
        BOOST_CHECK(outInt == inInt);
        const std::string inString("TEST");
        packet << inString;
        std::string outString;
        packet >> outString;
        BOOST_CHECK(inString.compare(outString) == 0);
    }
    //Test template input function
    {
        ODPacket packet;
        const std::string inString1("test1");
        const char* inString2 = "test2";
        const int32_t inInt = -25;
        ODPacket::putInPacket(packet, inString1, inString2,
                              inInt);
        std::string outString1;
        std::string outString2;
        int32_t outInt;
        packet >> outString1 >> outString2 >> outInt;
        BOOST_CHECK(inString1.compare(outString1) == 0);
        BOOST_CHECK(outString2.compare(inString2) == 0);
        BOOST_CHECK(inInt == outInt);

    }
}
