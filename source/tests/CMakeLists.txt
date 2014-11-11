include(BoostTestTargets)

add_boost_test(Random
	SOURCES
	test_Random.cpp
        "${SRC}/utils/Random.h"
        "${SRC}/utils/Random.cpp")

add_boost_test(Console
        SOURCES
        test_ConsoleInterface.cpp
        "${SRC}/modes/ConsoleInterface.h"
        "${SRC}/modes/ConsoleInterface.cpp"
        "${SRC}/modes/Command.h"
        "${SRC}/modes/Command.cpp")
