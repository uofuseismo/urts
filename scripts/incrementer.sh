#!/usr/bin/env bash
# Purpose: Starts/stops/checks the broadcastWaveRing status.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=/home/bbaker/Codes/urts/clang_build/incrementer.ini
# Name of executable
EXECUTABLE=incrementer
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="Incrementer Service"

# Define the executable
#COMMAND="${EXECUTABLE} --ini=${CONFIG_FILE}"
ARGS="--ini=${CONFIG_FILE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
