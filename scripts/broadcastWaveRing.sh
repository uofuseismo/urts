#!/usr/bin/env bash
# Purpose: Starts/stops/checks the status of broadcastWaveRing.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=$(pwd)/waveRing.ini
# Name of executable
EXECUTABLE=broadcastWaveRing
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="Earthworm Wave Ring Broadcast"

# Define the executable
ARGS="--ini=${CONFIG_FILE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
