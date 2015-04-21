include(BoostTestTargets)

add_boost_test(Random
        SOURCES
        test_Random.cpp
        "${SRC}/utils/Random.h"
        "${SRC}/utils/Random.cpp")

add_boost_test(ODPacket
        SOURCES
        test_ODPacket.cpp
        "${SRC}/network/ODPacket.h"
        "${SRC}/network/ODPacket.cpp"
        LIBRARIES
        ${SFML_LIBRARIES})

add_boost_test(ConsoleInterface
        SOURCES
        test_ConsoleInterface.cpp
        "${SRC}/modes/AbstractModeManager.h"
        "${SRC}/modes/ConsoleInterface.h"
        "${SRC}/modes/ConsoleInterface.cpp"
        "${SRC}/modes/Command.h"
        "${SRC}/modes/Command.cpp")

add_boost_test(CreatureDefinition
        SOURCES
        test_CreatureDefinition.cpp
        "${SRC}/entities/CreatureDefinition.cpp"
        "${SRC}/network/ODPacket.cpp"
        "${SRC}/rooms/RoomType.cpp"
        "${SRC}/utils/LogManager.cpp"
        "${SRC}/utils/Helper.cpp"
        LIBRARIES
        ${SFML_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OGRE_LIBRARIES})

add_boost_test(Goal
        SOURCES
        test_Goal.cpp
        "${SRC}/goals/Goal.cpp")

add_boost_test(Tile
        SOURCES
        test_Tile.cpp
        "${SRC}/entities/Tile.cpp"
        "${SRC}/entities/EntityBase.cpp"
        "${SRC}/game/Player.cpp"
        "${SRC}/game/Seat.cpp"
        "${SRC}/goals/Goal.cpp"
        "${SRC}/network/ODPacket.cpp"
        "${SRC}/utils/Helper.cpp"
        "${SRC}/utils/LogManager.cpp"
        "${SRC}/tests/mocks/SeatMockImpl.cpp"
        "${SRC}/tests/mocks/TileMockImpl.cpp"
        LIBRARIES
        ${Boost_LIBRARIES}
        ${OGRE_LIBRARIES}
        ${SFML_LIBRARIES})

add_boost_test(Pathfinding
        SOURCES
        test_Pathfinding.cpp)