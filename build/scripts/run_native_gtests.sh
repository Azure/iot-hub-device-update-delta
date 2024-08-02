#!/bin/bash
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
cmake_build_dir=$1
test_data_root=$2
test_prefix=$3

echo "Running tests under $cmake_build_dir with test_data_root: $test_data_root"

if [[ "$test_prefix" != "" ]]; then
	echo "test_prefix: $test_prefix"
fi

ALL_GTESTS=$(find $cmake_build_dir -name "*gtest" -type f)

declare -A FAILED_GTESTS=()


for f in $ALL_GTESTS
do
	GTEST_CMDLINE="$test_prefix $f --test_data_root $test_data_root"
	echo "Running: $GTEST_CMDLINE"
	$GTEST_CMDLINE

	if [[ $? != 0 ]]; then
		FAILED_GTESTS[$f]=$?
	fi
done

EXIT_RESULT=0

for failed_test in "${!FAILED_GTESTS[@]}";
do
	echo "Failure: $failed_test failed with result: ${FAILED_GTESTS[$failed_test]}"
	EXIT_RESULT=1
done

if [[ $EXIT_RESULT == 0 ]]; then
	echo "All tests passed!"
fi

exit $EXIT_RESULT