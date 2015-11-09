#!/bin/bash
# Shell script to run the unit tests for OpenDungeons

OD_BINARY="opendungeons"
TEST_BASENAME="boosttest-source_tests-"

if [ ! -x $(pwd)/${OD_BINARY} ]; then
    echo "Can't find the ${OD_BINARY} binary in the current directory, aborting."
    exit 1
fi

TESTS_PASSED=""
TESTS_FAILED=""

# The boost test filenames are supposed to be as follows: ${TEST_BASENAME}LL-*
# LL=name of the level server the game should launch (00 if none).
for testbin in boosttest-source_tests-*; do
    test=$(echo ${testbin} |sed 's/'${TEST_BASENAME}'//')
    echo -e "\n### Unit test: ${test}\n"

    pid=0
    level=$(echo ${test} |cut -d'-' -f1)
    if [ "${level}" != "00" ]; then
        echo "--- Starting a server with map ${level}.level ---"
        ./${OD_BINARY} --server "${level}.level" --port 32222 --log srvLog.txt &
        pid=$!
    fi

    ./${testbin}

    if [ $? == 0 ]; then
        TESTS_PASSED+=" ${test}"
    else
        TESTS_FAILED+=" ${test}"
    fi

    if [ ${pid} -ne 0 ]; then
        kill $pid
        wait $pid  # Wait for the process to terminate
    fi
done

echo -e "\n#####################################"
echo "The following tests were PASSED: ${TESTS_PASSED}"
echo "The following tests were FAILED: ${TESTS_FAILED}"
echo -e "#####################################\n"

if [ ! -z "${TESTS_FAILED}" ]; then
    exit 2
fi
