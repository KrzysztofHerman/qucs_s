#!/bin/bash

# This is a basic test script for qucs-mean.
# It checks if the application launches without crashing.
# More comprehensive UI testing is difficult in this environment.

# Set the path to the qucs-mean executable
# Assuming it's built in a 'build' directory relative to the main project root
# and installed into 'bin' within that build directory, or directly in 'build/qucs-mean'
# Adjust the path if your build structure is different.

QUCS_MEAN_EXEC_PATH_1="../../build/bin/qucs-mean" # Path if installed
QUCS_MEAN_EXEC_PATH_2="build/qucs-mean" # Path if built in qucs-mean/build
QUCS_MEAN_EXEC_PATH_3="./qucs-mean" # Path if built directly in qucs-mean (after cd qucs-mean)

QUCS_MEAN_EXEC=""

if [ -f "$QUCS_MEAN_EXEC_PATH_1" ]; then
    QUCS_MEAN_EXEC="$QUCS_MEAN_EXEC_PATH_1"
elif [ -f "$QUCS_MEAN_EXEC_PATH_2" ]; then
    QUCS_MEAN_EXEC="$QUCS_MEAN_EXEC_PATH_2"
elif [ -f "$QUCS_MEAN_EXEC_PATH_3" ]; then
    QUCS_MEAN_EXEC="$QUCS_MEAN_EXEC_PATH_3"
else
    echo "Error: qucs-mean executable not found. Searched paths:"
    echo "1. $QUCS_MEAN_EXEC_PATH_1"
    echo "2. $QUCS_MEAN_EXEC_PATH_2"
    echo "3. $QUCS_MEAN_EXEC_PATH_3"
    exit 1
fi

echo "Found qucs-mean executable at: $QUCS_MEAN_EXEC"

# Attempt to launch the application and kill it after a short period
# The primary goal is to see if it starts without immediate errors.
# The timeout is important because GUI apps might hang without a display.
timeout 5s $QUCS_MEAN_EXEC &
PROC_ID=$!

wait $PROC_ID
EXIT_CODE=$?

if [ $EXIT_CODE -eq 124 ]; then
    echo "qucs-mean timed out as expected (no display server)."
    echo "Test considered PASSED (application launched)."
    exit 0
elif [ $EXIT_CODE -eq 0 ]; then
    echo "qucs-mean exited cleanly (unexpected in no-display env, but ok)."
    echo "Test considered PASSED (application launched)."
    exit 0
else
    echo "qucs-mean exited with error code: $EXIT_CODE"
    echo "Test considered FAILED."
    exit 1
fi
