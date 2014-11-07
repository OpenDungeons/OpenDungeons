#include "utils/Random.h"

#define BOOST_TEST_MODULE Random
#include "BoostTestTargetConfig.h"

BOOST_AUTO_TEST_CASE(test_Random)
{
    Random::initialize();
    BOOST_CHECK (Random::Int(1, 2 ) <= 2);
}
