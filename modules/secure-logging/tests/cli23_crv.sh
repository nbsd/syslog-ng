#!/usr/bin/env bash
#############################################################################
# Copyright (c) 2025 Airbus Commercial Aircraft
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################

# Author: Airbus Commercial Aircraft <secure-logging@airbus.com>
# File:   cli23_crv.sh
# Date:   2026-05-29
#
# Wrapper for calling cli tool cr_verifier to provide an unencrypted log
# file based on the encrypted output of cr_logger.
#
# This script can be used to test whether an encrypted log file has been
# tampered or if some bit have been flipped.
# A small amount of bit flips can even be compensated.
# For more, see
#
#	@misc{cryptoeprint:2019/506,
#	 	  author = {Erik-Oliver Blass and Guevara Noubir},
#		  title = {Forward Security with Crash Recovery for Secure Logs},
#		  howpublished = {Cryptology {ePrint} Archive, Paper 2019/506},
#         year = {2019},
#		  url = {https://eprint.iacr.org/2019/506}
#   }
#
# Usage:
#   ./cli23_crv.sh <input_file_path> <working_directory> <count_lines>
#
# Example:
#   ./cli23_crv.sh /home/marcus/test_slog/cr/msg1000.txt.enc /home/marcus/test_slog/cr 1000
#
# Note: In case a file with the same name as an output file is available
# in the working directoy, it might be overwritten without asking.
#-----------------------------------------------------------------------

# set -x
set -o pipefail

VERSION="Version 1.0.3"

# remove path and extension from $0
s=$0
SCRIPTNAME="$(
    b="${s##*/}"
    echo "${b%.*}"
)"
echo "SCRIPTNAME: ${SCRIPTNAME}"

NOW=$(date +%Y-%m-%d_%H%M%S)
echo " "
echo " "
echo "***********************************************************"
echo "*** ${SCRIPTNAME}, ${VERSION}, ${NOW}"
echo "***********************************************************"
echo " "

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd -P)"
echo "SCRIPT_DIR: ${SCRIPT_DIR}"

# PREFIX: This path must fit the one given to the build system where binaries are provided.
PATH_PREFIX_VALUE=$("${SCRIPT_DIR}"/get_prefix.sh)
PREFIX=${PATH_PREFIX_VALUE}
echo "PREFIX: ${PREFIX}"
if [[ ! -e ${PREFIX} ]]; then
    echo "ERROR: Required path PREFIX for installed binaries not found."
    echo "Maybe the project has not yet been built or the build directory was moved or deleted."
    echo "The path to cr_verifier is PREFIX/bin"
    echo "You can try to set PREFIX manually here in the script instead of using get_prefix.sh"
    echo "Example: PREFIX=${HOME}/Software/install"
    echo "FAIL"
    exit 1
fi

HOMESLOGTEST=${SCRIPT_DIR}
BIN=${PREFIX}/bin

# error counter, success when this script returns 0
cnt_error=0

#-----------------------------------------------------------------------
# Function to display help/usage
usage() {
    echo " "
    echo "Usage: $(basename "$0") [OPTIONS] <input_file_path> <working_directory> <count_lines>"
    echo
    echo "Arguments:"
    echo "  input_file_path      Path to a plain log file."
    echo "  working_directory    Path to a working folder."
    echo "  line count           Count of lines in plain log file."
    echo
    echo "Options:"
    echo "  -h, --help           Show this help message and exit."
    echo " "
    echo "Example:"
    echo "./$(basename "$0") $HOME/test_slog/cr/msg1000.txt.enc $HOME/test_slog/cr 1000"
    echo " "
    exit 1
}

#-----------------------------------------------------------------------
# list current configuration and exit with error
check_script_config() {
    echo "ERROR! Precondition to start failed. Check configuration of $0"
    echo " "
    # Prefix is provided by a script. When this is not working
    # User can try to set it manually its the place where binaries are provided.
    echo "PREFIX: ${PREFIX}"
    echo "BIN: ${BIN}"
    # stop working. exit now.
    exit 1
}

#-----------------------------------------------------------------------
# -- helper function to check of all files in given array do exist
# Usage: check_missing "path1" "path2" "path3" ...
check_missing() {
    for path in "$@"; do
        # Check if the path does NOT exist (-e works for files and directories)
        if [[ ! -e ${path} ]]; then
            echo "Error: Required path '${path}' not found."
            check_script_config
            # ERROR, check_script_config will exit 1
        fi
    done
    return 0 # SUCESS, all files found
}

#-----------------------------------------------------------------------

# Handle explicit help flags first
case "$1" in
-h | --help)
    usage
    ;;
esac

# Check for the correct number of arguments
if [[ $# -ne 3 ]]; then
    echo "Error: Invalid number of arguments."
    usage
fi

# Assign arguments to descriptive variables
INPUT_FILE="$1"
WORKING_DIR="$2"
LOG_LINES="$3"

# -- Validate First Argument (Regular File) ---
# -e checks if it exists; -f checks if it is a regular file
if [[ ! -f ${INPUT_FILE} ]]; then
    echo "Error: Input file '${INPUT_FILE}' does not exist or is not a regular file." >&2
    exit 1
fi

# -- Process and Validate Second Argument (Working Directory) ---
# Attempt to create the directory (including parents)
mkdir -p "${WORKING_DIR}" 2>/dev/null

# Check if the directory exists and is writable
if [[ ! -d ${WORKING_DIR} ]]; then
    echo "Error: Failed to create or access directory '${WORKING_DIR}'." >&2
    exit 1
fi

if [[ ! -w ${WORKING_DIR} ]]; then
    echo "Error: Directory '${WORKING_DIR}' exists but is not writable." >&2
    exit 1
fi

# Check if the input is a valid integer
if [[ ! ${LOG_LINES} =~ ^-?[0-9]+$ ]]; then
    echo "Error: 3rd argument is not a valid integer."
    exit 1
fi

# Check if it is greater than zero
if [[ ${LOG_LINES} -gt 0 ]]; then
    echo "${LOG_LINES} is a positive number."
else
    echo "Error: 3d argument is not positive: ${LOG_LINES}."
    exit 1
fi

echo " "
"${SCRIPT_DIR}"/get_git_info.sh

# -- do initial checks -----
check_missing "${PREFIX}" "${BIN}"
check_missing "${HOMESLOGTEST}" "${BIN}/cr_logger" "${BIN}/cr_verifier"

echo "Validation successful!"
echo "Processing file: ${INPUT_FILE}"
echo "Working in:      ${WORKING_DIR}"
echo "Count lines:     ${LOG_LINES}"

#-----------------------------------------------------------------------

cp -f "${HOMESLOGTEST}/master.key" "${WORKING_DIR}/master.key"
check_missing "${WORKING_DIR}/master.key"

SFN=$(basename "${INPUT_FILE}")
echo "SFN: ${SFN}"

echo "----------------------------------------"
echo "-- Verifiy log file"
echo "----------------------------------------"
echo " "
if ! "${BIN}/cr_verifier" \
    --key "${WORKING_DIR}/master.key" \
    --in "${INPUT_FILE}" \
    --out "${WORKING_DIR}/${SFN}.verifier.txt" \
    --maxlogs "${LOG_LINES}"; then
    cnt_error=$((cnt_error + 1))
fi

if [[ ! -e "${WORKING_DIR}/${SFN}.verifier.txt" ]]; then
    echo "Error: Required path '${WORKING_DIR}/${SFN}.verifier.txt' not found."
    cnt_error=$((cnt_error + 1))
fi

echo "You might want to call this script like:"
echo "$0 2>&1 | tee ${WORKING_DIR}/protocol_${SCRIPTNAME}_${NOW}.log"
echo " "

echo "The verifiers output file: '${WORKING_DIR}/${SFN}.verifier.txt'"
echo "In case the the loggers output file ${INPUT_FILE} has not been tampered"
echo "it has the same content as the plain log file."
echo " "

echo "return cnt_error: ${cnt_error}"
#if ((cnt_error == 0)); then
if [[ ${cnt_error} -eq 0 ]]; then
    echo "PASS"
else
    echo "Found ERROR"
    echo "FAIL"
fi

echo " "
echo Done
echo " "
# exit "${cnt_error:-0}"
exit "${cnt_error}"
