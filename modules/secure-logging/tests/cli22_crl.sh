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
# File:   cli22_crl.sh
# Date:   2026-05-29
#
# Wrapper for calling cli tool cr_logger to provide an encrypted log
# file that can be tampered to test cr_verifier afterwards.
#
# Crash Recovery is documented here:
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
#   ./cli22_crl.sh <input_file_path> <working_directory>
#
# Example:
#   ./cli22_crl.sh /home/marcus/test_slog/cr/pg2701.txt /home/marcus/test_slog/cr
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
    echo "The path to cr_logger is PREFIX/bin"
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
    echo "Usage: $(basename "$0") [OPTIONS] <input_file_path> <working_directory>"
    echo
    echo "Arguments:"
    echo "  input_file_path      Path to a plain log file."
    echo "  working_directory    Path to a working folder."
    echo
    echo "Options:"
    echo "  -h, --help           Show this help message and exit."
    echo " "
    echo "Example:"
    echo "./$(basename "$0") $HOME/test_slog/cr/msg1000.txt $HOME/test_slog/cr"
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
if [[ $# -ne 2 ]]; then
    echo "Error: Invalid number of arguments."
    usage
fi

# Assign arguments to descriptive variables
INPUT_FILE="$1"
WORKING_DIR="$2"

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

echo " "
"${SCRIPT_DIR}"/get_git_info.sh

# -- do initial checks -----
check_missing "${PREFIX}" "${BIN}"
check_missing "${HOMESLOGTEST}" "${BIN}/cr_logger" "${BIN}/cr_verifier"

echo "Validation successful!"
echo "Processing file: ${INPUT_FILE}"
echo "Working in:      ${WORKING_DIR}"

#-----------------------------------------------------------------------

cp -f "${HOMESLOGTEST}/master.key" "${WORKING_DIR}/master.key"
check_missing "${WORKING_DIR}/master.key"

# Check the number of arguments
# Capturing the count into a variable
LOG_LINES=$(wc -l <"${INPUT_FILE}")
echo "The file has ${LOG_LINES} lines."
SFN=$(basename "${INPUT_FILE}")
echo "SFN: ${SFN}"
PLAIN_FILE="${WORKING_DIR}/${SFN}"
if [[ -f ${PLAIN_FILE} ]]; then
    # File already exists, check if content is identical
    if cmp -s "${INPUT_FILE}" "${PLAIN_FILE}"; then
        echo "Notice: File '${SFN}' already exists in destination and is identical. Proceeding..."
    else
        echo "Error: File '${SFN}' exists in destination but has DIFFERENT content." >&2
        echo "Operation cancelled to prevent overwriting or data mismatch." >&2
        exit 1
    fi
else
    # File does not exist, safe to copy
    echo "Copying '${SFN}' to '${WORKING_DIR}'..."
    cp "${INPUT_FILE}" "${PLAIN_FILE}"
fi
check_missing "${PLAIN_FILE}"

echo "----------------------------------------"
echo "-- Encrypt log file"
echo "----------------------------------------"
echo " "
if ! "${BIN}/cr_logger" \
    --key "${WORKING_DIR}/master.key" \
    --in "${PLAIN_FILE}" \
    --out "${PLAIN_FILE}.enc" \
    --maxlogs "${LOG_LINES}"; then
    cnt_error=$((cnt_error + 1))
fi

if [[ ! -e "${PLAIN_FILE}.enc" ]]; then
    echo "Error: Required path '${PLAIN_FILE}.enc' not found."
    cnt_error=$((cnt_error + 1))
fi

echo "You might want to call this script like:"
echo "$0 2>&1 | tee ${WORKING_DIR}/protocol_${SCRIPTNAME}_${NOW}.log"
echo " "

echo " "
echo "The verifier needs to know to loggers output file path, the working directory path and"
echo "how many lines are encrypted in the output file."

echo "Output file of cr_logger:"
echo "  ${PLAIN_FILE}.enc"
echo "Working directoy:"
echo "  ${WORKING_DIR}"
echo "Count of lines in plain log file:"
echo "  ${LOG_LINES}"
echo " "
echo "The verifier can be called by wrapper script ./cli23_crv.sh, like:"
echo " "
echo "./cli23_crv.sh ${PLAIN_FILE}.enc ${WORKING_DIR} ${LOG_LINES}"
# ./cli23_crv.sh /home/marcus/test_slog/cr/pg2701.txt.enc /home/marcus/test_slog/cr 22314
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
