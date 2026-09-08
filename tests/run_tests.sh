#!/bin/sh
set -e

# Tests
ctest --test-dir build --output-on-failure
# Tests coverage
lcov --directory . --capture --output-file /coverage/coverage.info --rc geninfo_auto_base=1 --ignore-errors mismatch,inconsistent
lcov --remove /coverage/coverage.info '/usr/*' '*/tests/*' '*/gtest/*' '*/gmock/*' --output-file /coverage/coverage.filtered.info
genhtml /coverage/coverage.filtered.info --output-directory /coverage/coverage-report

# Sanitizers
./build/runTsan
./build/runAsan
