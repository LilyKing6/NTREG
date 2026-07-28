#!/bin/bash
# Unit test runner - regenerates hive files before each test suite
# to ensure clean state and avoid cross-suite contamination.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../../build"
cd "$BUILD_DIR"
mkdir -p Config

TEST_DIR="$BUILD_DIR/tests/unit"
ln -sf ../../Config "$TEST_DIR/Config" 2>/dev/null || true

FAILED=0
for t in cm_api_tests cm_check_tests cm_core_tests cm_hive_tests \
          cm_index_tests cm_path_tests cm_value_adv_tests cm_value_tests; do
    TEST_PATH="$TEST_DIR/$t"
    if [ ! -x "$TEST_PATH" ]; then
        echo "=== $t === NOT FOUND"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Regenerate clean hive before each suite
    ./mkhive -h:SYSTEM -u -d:Config ../reginit/hivesys.inf > /dev/null 2>&1
    ./mkhive -h:SOFTWARE -u -d:Config ../reginit/hivesft.inf > /dev/null 2>&1
    ./mkhive -h:DEFAULT -u -d:Config ../reginit/hivedef.inf > /dev/null 2>&1

    echo "=== $t ==="
    if ! "$TEST_PATH"; then
        FAILED=$((FAILED + 1))
    fi
    echo
done

echo "=============================="
if [ $FAILED -eq 0 ]; then
    echo "All unit tests passed."
else
    echo "$FAILED test suite(s) failed."
    exit 1
fi
