include(BoostTestTargets)

add_boost_test(Random
        SOURCES
        test_Random.cpp
        ${SRC}/utils/Random.h
        ${SRC}/utils/Random.cpp)

add_boost_test(ODPacket
        SOURCES
        test_ODPacket.cpp
        ${SRC}/network/ODPacket.h
        ${SRC}/network/ODPacket.cpp
        LIBRARIES
        ${SFML_LIBRARIES})

add_boost_test(ConsoleInterface
        SOURCES
        test_ConsoleInterface.cpp
        ${SRC}/modes/AbstractModeManager.h
        ${SRC}/modes/ConsoleInterface.h
        ${SRC}/modes/ConsoleInterface.cpp
        ${SRC}/modes/Command.h
        ${SRC}/modes/Command.cpp)

add_boost_test(Goal
        SOURCES
        test_Goal.cpp
        ${SRC}/goals/Goal.cpp
        ${SRC}/utils/LogManager.cpp
        ${SRC}/utils/LogSinkConsole.cpp
        LIBRARIES
        ${SFML_LIBRARIES})

add_boost_test(Pathfinding
        SOURCES
        test_Pathfinding.cpp)

add_boost_test(LaunchGame
        SOURCES
        ${SRC}/tests/mocks/ODClientTest.cpp
        ${SRC}/network/ClientNotification.cpp
        ${SRC}/network/ODPacket.cpp
        ${SRC}/network/ODSocketClient.cpp
        ${SRC}/network/ODSocketServer.cpp
        ${SRC}/network/ServerMode.cpp
        ${SRC}/network/ServerNotification.cpp
        ${SRC}/utils/Helper.cpp
        ${SRC}/utils/LogManager.cpp
        ${SRC}/utils/LogSinkConsole.cpp
        test_LaunchGame.cpp
        LIBRARIES
        ${SFML_LIBRARIES})