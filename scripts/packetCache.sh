#!/usr/bin/env bash
# Purpose: Starts/stops/checks status of the packetCache.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=$(pwd)/packetCache.ini
# Name of executable
EXECUTABLE=packetCache
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="PacketCache Service"

# Define the executable
ARGS="--ini=${CONFIG_FILE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
