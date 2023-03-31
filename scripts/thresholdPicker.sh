#!/usr/bin/env bash
# Purpose: Starts/stops/checks the status of thresholdPicker.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=$(pwd)/thresholdPicker.ini
# Name of executable
EXECUTABLE=thresholdPicker
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="Threshold Picker Module"

# Define the configuration file 
ARGS="--ini=${CONFIG_FILE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
