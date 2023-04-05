#!/usr/bin/env bash
# Purpose: Starts/stops/checks status of the firstMotionClassifierService.
# Author: Ben Baker 
source urts.sh

# Workspace directory
WORKDIR=`dirname $0`
# Config file
CONFIG_FILE=$(pwd)/firstMotionClassifier.ini
# Name of executable
EXECUTABLE=firstMotionClassifierService
# Instance of service
INSTANCE=1
# Seconds to wait to stop/start a program
DURATION=10
# Name displayed by the init script
NAME="P First Motion Classifier Service"

# Define the executable
ARGS="--ini=${CONFIG_FILE} --instance=${INSTANCE}"

ACTION=$1
EXIT_STATUS=$(urts "${ACTION}" "${EXECUTABLE}" "${ARGS}" "${NAME}" "${DURATION}" ${WORKDIR})
if [ ${EXIT_SUCCESS} ]; then
   echo "Errors detected: ${EXIT_STATUS}"
fi
exit ${EXIT_STATUS}
