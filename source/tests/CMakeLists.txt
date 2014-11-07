include(BoostTestTargets)

add_boost_test(Random
	SOURCES
	test_Random.cpp
        "${SRC}/utils/Random.h"
        "${SRC}/utils/Random.cpp")
