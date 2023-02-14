#!/usr/bin/env bash
# Purpose: Starts/stops/checks the status of incrementer.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=$(pwd)/incrementer.ini
# Name of executable
EXECUTABLE=incrementerService
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="Incrementer Service"

# Define the configuration file 
ARGS="--ini=${CONFIG_FILE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
