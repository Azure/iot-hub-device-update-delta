#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
CONFIGURATION=$1
VERBOSE=$2

if [ -z "$CONFIGURATION" ]; then
    CONFIGURATION="Debug"
fi

if [ -z "$VERBOSE" ]; then
    VERBOSE="false"
fi

if [ "$VERBOSE" == "true" ]; then
    VERBOSITY_PARAMETER="-v diag"
fi

declare -i failed_tests=0
declare -i passed_tests=0

if  dotnet test ./DiffGen/tests/UnitTests/UnitTests.csproj --logger "console;verbosity=detailed" --configuration $CONFIGURATION --no-build $VERBOSITY_PARAMETER ; then
    echo "Test succeeded!"
    passed_tests=passed_tests+1
else
    echo "Test failed!"
    failed_tests=failed_tests+1
fi

if dotnet test ./DiffGen/tests/ArchiveUtilityTest/ArchiveUtilityTest.csproj --logger "console;verbosity=detailed" --configuration $CONFIGURATION --no-build $VERBOSITY_PARAMETER ; then
    echo "Test succeeded!"
    passed_tests=passed_tests+1
else
    echo "Test failed!"
    failed_tests=failed_tests+1
fi

if dotnet test ./DiffGen/tests/EndToEndTests/EndToEndTests.csproj --logger "console;verbosity=detailed" --configuration $CONFIGURATION --no-build $VERBOSITY_PARAMETER ; then
    echo "Test succeeded!"
    passed_tests=passed_tests+1
else
    echo "Test failed!"
    failed_tests=failed_tests+1
fi

if [ "$failed_tests" == "0" ] ; then
   echo "All $passed_tests passed!"
   echo "Exiting with code 0."
   exit 0
else
   echo "$passed_tests passed!"
   echo "$failed_tests failed!"
   echo "Exiting with code 1."
   exit 1
fi