#!/bin/bash
# Shell script to run the unit tests for OpenDungeons

OD_BINARY="opendungeons"
UNIT_TESTS="00-ConsoleInterface 00-Random Goal 00-ODPacket 00-Pathfinding aa-LaunchGame"

if [ ! -x $(pwd)/${OD_BINARY} ]; then
    echo "Can't find the ${OD_BINARY} binary in the current directory, aborting."
    exit 1
fi

TESTS_PASSED=""
TESTS_FAILED=""

for test in ${UNIT_TESTS}; do
    echo -e "\n### Unit test: ${test}\n"

    if [ "${test}" == "aa-LaunchGame" ]; then
        ./${OD_BINARY} --server UnitTest.level --port 32222 --log srvLog.txt &
    fi
    ./boosttest-source_tests-${test}

    if [ $? == 0 ]; then
        TESTS_PASSED+=" ${test}"
    else
        TESTS_FAILED+=" ${test}"
    fi
done

echo -e "\n#####################################"
echo "The following tests were PASSED: ${TESTS_PASSED}"
echo "The following tests were FAILED: ${TESTS_FAILED}"
echo -e "#####################################\n"

if [ ! -z ${TESTS_FAILED} ]; then
    exit 2
fi
