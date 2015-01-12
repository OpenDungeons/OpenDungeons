#include "network/ODPacket.h"
#include <boost/mpl/map.hpp>

#define BOOST_TEST_MODULE ODPacket
#include "BoostTestTargetConfig.h"

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
