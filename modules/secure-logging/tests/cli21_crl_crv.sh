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
# File:   cli21_crl_crv.sh
# Date:   2026-05-29
#
# Smoke Test of cli tools cr_logger and cr_verifier
#
# This scripts tests standalone tools cr_logger and cr_verifier.
# syslog-ng is not involved here.
# This script is used for a quick smoke test of cr_logger and cr_verifier
# using only a small plain log file which is generated.
# Furthermore a small number of bit flips are done in the loggers output file
# to test their detection by the verifier and its recovery skills.
#
# Preconditions:
#   cr_logger, cr_verifier are available in the install path given by PREFIX
#   This should be the case after run_cmake_rebuild.sh or run_autotools_rebuild.sh
#   has been called from syslog-ng root folder.
#   Helper scripts are available:
#     get_prefix.sh
#     generate_logs.sh
#     tamper_file.sh
#   Initial key used by cr_logger an cr_verifier is available:
#     master.key
#   Expected being called from inside
#   <syslog-ng-root-folder>/modules/secure-logging/tests
#
# Usage:
#   ./cli21_crl_crv.sh
#
#-----------------------------------------------------------------------

# set -x
set -o pipefail

VERSION="Version 1.0.9"

# The COUNT_OF_LOG_LINES is used when this script is not provided
# with a path to plain log file and a log file must be generated
# therefore instead
COUNT_OF_LOG_LINES=500
SHOW_DIFF=false

# remove path and extension from $0
s=$0
SCRIPTNAME="$(
    b="${s##*/}"
    echo "${b%.*}"
)"
echo "SCRIPTNAME: ${SCRIPTNAME}"

PID=$$
echo "PID: ${PID}"

RANDOM_ID=$(
    /bin/dd if=/dev/urandom bs=1 count=4 2>/dev/null |
        od -An -N4 -tx
)
CLEAN_ID=$(echo "${RANDOM_ID}" | tr -d ' ')

NOW=$(date +%Y-%m-%d_%H%M%S)
echo " "
echo " "
echo "***********************************************************"
echo "*** ${SCRIPTNAME}, ${VERSION}, ${NOW}"
echo "***********************************************************"
echo " "

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd -P)"
echo "SCRIPT_DIR: ${SCRIPT_DIR}"

PATH_PREFIX_VALUE=$("${SCRIPT_DIR}"/get_prefix.sh)
# This path must fit the one given to the build system where binaries are provided.
PREFIX=${PATH_PREFIX_VALUE}
echo "PREFIX: ${PREFIX}"
if [[ ! -e ${PREFIX} ]]; then
    echo "ERROR: Required path PREFIX for installed binaries not found."
    echo "Maybe the project has not yet been built or the build directory was moved or deleted."
    echo "The path to cr_logger and cr_verifier is PREFIX/bin"
    echo "You can try to set PREFIX manually here in the script instead of using get_prefix.sh"
    echo "Example: PREFIX=${HOME}/Software/install"
    echo "FAIL"
    exit 1
fi
HOMESLOGTEST=${SCRIPT_DIR}

BIN=${PREFIX}/bin
SBIN=${PREFIX}/sbin

SUBFOLDER_TEST="test_slog"
PATH_SUFFIX="${SCRIPTNAME}_${NOW}_${PID}_${CLEAN_ID}"
SUBFOLDER="data"
TEST=/tmp/${SUBFOLDER_TEST}/${SUBFOLDER}_${PATH_SUFFIX}
echo "TEST: ${TEST}"

HOME_BACKUP=${HOME}/${SUBFOLDER_TEST}/${SCRIPTNAME}
# COPY_TO_HOME_BACKUP="false"
COPY_TO_HOME_BACKUP="true"

# prepared key is used
# MACADDRESS="01:23:45:67:89:AB"
# SERIALNUMBER="12345678"

# error counter, success when this script returns 0
cnt_error=0

#-----------------------------------------------------------------------
# list current configuration and exit with error
check_script_config() {
    echo "ERROR! Precondition to start failed. Check configuration of $0"
    echo " "
    # Prefix is provided by a script. When this is not working
    # User can try to set it manually its the place where binaries are provided.
    echo "PREFIX: ${PREFIX}"
    echo "BIN: ${BIN}"
    echo "TEST: ${TEST}"
    echo "COUNT_OF_LOG_LINES: ${COUNT_OF_LOG_LINES}"
    echo "COPY_TO_HOME_BACKUP: ${COPY_TO_HOME_BACKUP}"
    if [[ ${COPY_TO_HOME_BACKUP} == "true" ]]; then
        echo "HOME_BACKUP: ${HOME_BACKUP}"
    fi

    # stop working. exit now.
    exit 1
}

#-----------------------------------------------------------------------
# Function to display help/usage
usage() {
    echo "Usage: $0 [file_path]"
    echo "  - No arguments: Runs in default mode and a log file is generated."
    echo "  - file_path: log file that shall be processed."
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

echo " "
"${SCRIPT_DIR}"/get_git_info.sh

mkdir -p "${TEST}"
if [[ ${COPY_TO_HOME_BACKUP} == "true" ]]; then
    mkdir -p "${HOME_BACKUP}"
    check_missing "${HOME_BACKUP}"
fi

# -- do initial checks -----
check_missing "${TEST}" "${PREFIX}" "${BIN}" "${SBIN}"
check_missing "${HOMESLOGTEST}" "${BIN}/cr_logger" "${BIN}/cr_verifier"

# cleanup files from previous tests
rm -f "${TEST}"/*.key "${TEST}"/*.txt "${TEST}"/*.enc 2>/dev/null
rm -f "${TEST}"/*.out "${TEST}"/*.log 2>/dev/null

if [[ ${COPY_TO_HOME_BACKUP} == "true" ]]; then
    rm -f "${HOME_BACKUP}"/*.key "${HOME_BACKUP}"/*.txt "${HOME_BACKUP}"/*.enc 2>/dev/null
    rm -f "${HOME_BACKUP}"/*.out 2>/dev/null
fi

cp -f "${HOMESLOGTEST}/generate_logs.sh" "${TEST}/generate_logs.sh"
cp -f "${HOMESLOGTEST}/master.key" "${TEST}/master.key"
cp -f "${HOMESLOGTEST}/tamper_file.sh" "${TEST}/tamper_file.sh"

check_missing "${TEST}/generate_logs.sh" "${TEST}/master.key" "${TEST}/tamper_file.sh"

# Check the number of arguments
if [[ $# -eq 0 ]]; then
    echo "----------------------------------------"
    echo "-- Create log file (because no arguemnt was given)"
    echo "----------------------------------------"
    echo " "
    "${TEST}/generate_logs.sh" "${COUNT_OF_LOG_LINES}" >"${TEST}/plainlog.txt"
    return_value=$?
    echo "return_value: ${return_value}"
    if [[ ! ${return_value} -eq 0 ]]; then
        echo "ERROR: Generation of log file failed!"
        cnt_error=$((cnt_error + 1))
        exit 1
    fi
    if [[ ! -e "${TEST}/plainlog.txt" ]]; then
        echo "ERROR: Required file '${TEST}/plainlog.txt' not found."
        cnt_error=$((cnt_error + 1))
        exit 1
    fi
    echo "Logfile ${TEST}/plainlog.txt has been generated successfully"

elif [[ $# -eq 1 ]]; then
    FILE_PATH=$1
    # Check if the file actually exists
    if [[ -f ${FILE_PATH} ]]; then
        echo "Processing file: ${FILE_PATH}"
        if [[ -f ${FILE_PATH} ]]; then
            # Capturing the count into a variable
            COUNT_OF_LOG_LINES=$(wc -l <"${FILE_PATH}")
            echo "The file has ${COUNT_OF_LOG_LINES} lines."
            cp "${FILE_PATH}" "${TEST}/plainlog.txt"
            ls -alt "${TEST}"
            check_missing "${TEST}/plainlog.txt"
        else
            echo "Error: File '${FILE_PATH}' not found."
            exit 1
        fi
    fi

else
    # Too many arguments provided
    echo "Error: Too many arguments."
    usage
fi

#
# dirty hack to test a binary input instead a text file
# to test whether an application crashes
# cp -f "${HOMESLOGTEST}/bin" "${TEST}/plainlog.txt"
#

echo "----------------------------------------"
echo "-- Encrypt log file"
echo "----------------------------------------"
echo " "
if ! "${BIN}/cr_logger" \
    --key "${TEST}/master.key" \
    --in "${TEST}/plainlog.txt" \
    --out "${TEST}/plainlog.txt.enc" \
    --maxlogs "${COUNT_OF_LOG_LINES}"; then
    cnt_error=$((cnt_error + 1))
fi

if [[ ! -e "${TEST}/plainlog.txt.enc" ]]; then
    echo "Error: Required path '${TEST}/plainlog.txt.enc' not found."
    cnt_error=$((cnt_error + 1))
else
    echo "----------------------------------------"
    echo "-- Verifiy log file"
    echo "----------------------------------------"
    echo " "
    if ! "${BIN}/cr_verifier" \
        --key "${TEST}/master.key" \
        --in "${TEST}/plainlog.txt.enc" \
        --out "${TEST}/plainlog.txt.verifier.txt" \
        --maxlogs "${COUNT_OF_LOG_LINES}"; then
        cnt_error=$((cnt_error + 1))
    fi
    if [[ ! -e "${TEST}/plainlog.txt.verifier.txt" ]]; then
        echo "Error: Required path '${TEST}/plainlog.txt.verifier.txt' not found."
        cnt_error=$((cnt_error + 1))
    else
        echo "----------------------------------------"
        echo "-- Check verified log file"
        echo "----------------------------------------"
        echo " "
        HASH1=$(sha256sum "${TEST}/plainlog.txt" | awk '{ print $1 }')
        if echo "${HASH1}  ${TEST}/plainlog.txt.verifier.txt" | sha256sum -c - >/dev/null 2>&1; then
            # 'sha256sum -c' exits with 0 (success) if they match
            echo "Original log file is identical to verifier output as expected."
            echo " "
        else
            echo "Error: Decrypted file is different!"
            cnt_error=$((cnt_error + 1))
            # 'sha256sum -c' exits with non-zero (failure) if they don't match
            echo "-- INFO ---"
            echo "Original log file is different from verifier output!"
            # NOTE: input file might get truncated and fixed to utf-8 only
            # so the hash sum test only is useful when the input file is
            # a valid utf-8 encoded file.
            # check file length of input file and output file to detect
            # truncation
            # check whether original input file is utf-8 file
            if iconv -f UTF-8 -t UTF-8 "${TEST}/plainlog.txt" >/dev/null 2>&1; then
                echo "'${TEST}/plainlog.txt' is valid UTF-8."
                echo "check length.."
                SIZE1=$(wc -c <"${TEST}/plainlog.txt")
                SIZE2=$(wc -c <"${TEST}/plainlog.txt.verifier.txt")
                SIZE1=$(echo "${SIZE1}" | tr -d ' ')
                SIZE2=$(echo "${SIZE2}" | tr -d ' ')
                echo "File 1: ${TEST}/plainlog.txt (${SIZE1} bytes)"
                echo "File 2: ${TEST}/plainlog.txt.verifier.txt (${SIZE2} bytes)"
                if [[ ${SIZE1} -eq ${SIZE2} ]]; then
                    echo "Files are the same length."
                    # only test that tools produce output files cnt_error=$((cnt_error + 1))
                else
                    echo "Files are not the same length. Lines of input file might have been truncated."
                fi
            else
                echo "Reason: '${TEST}/plainlog.txt' contains invalid UTF-8 sequences."
                echo "The Crash Recovery Logger fixes invalid characters and"
                echo "also truncates length of a input line to 2048 bytes."
            fi # -- iconv

            if [[ ${SHOW_DIFF} == "true" ]]; then
                differences=$(diff -u "${TEST}/plainlog.txt" "${TEST}/plainlog.txt.verifier.txt")
                status=$?
                if [[ ${status} -eq 0 ]]; then
                    echo "Files are identical. No differences."
                elif [[ ${status} -eq 1 ]]; then
                    echo "Differences found:"
                    echo "${differences}" # This is where we "echo the differences"
                else
                    # Exit code >1 means an error occurred.
                    echo "Error running diff (status: ${status}):"
                    echo "${differences}" # This will contain diff's error message
                fi
            fi # -- diff
        fi     # -- hash
    fi         # -- verifier
fi             # -- logger

echo "----------------------------------------"
echo "-- Now manipulate (tamper) the encrypted file"
echo "----------------------------------------"

#  "${TEST}/plainlog.txt.enc"
# PATH_PREFIX_VALUE=$("${SCRIPT_DIR}"/get_prefix.sh)
cp "${TEST}/plainlog.txt.enc" "${TEST}/tampered.enc"

echo "Now, 6 bits in the encrypted file will be flipped."
if "${TEST}"/tamper_file.sh "${TEST}"/tampered.enc 6; then
    echo "tamper_file.sh executed successfully (exit status 0)."
else
    # The exit status is now held in $?
    echo "Error. tamper_file.sh failed with exit status $?."
    cnt_error=$((cnt_error + 1))
fi

# Verify that file has really been tampered
if cmp -s "${TEST}/plainlog.txt.enc" "${TEST}/tampered.enc"; then
    echo "Check tamper_file.sh output: File has NOT been tampered. Test ERROR."
    cnt_error=$((cnt_error + 1))
else
    echo "Check tamper_file.sh output: File has been tampered."
fi

echo "----------------------------------------"
echo "-- Verify the tampered encrypted log file"
echo "----------------------------------------"

# Call log Crash Recovery log verifier with tampared file
if ! "${BIN}/cr_verifier" \
    --key "${TEST}/master.key" \
    --in "${TEST}/tampered.enc" \
    --out "${TEST}/tampered.txt.verifier.txt" \
    --maxlogs "${COUNT_OF_LOG_LINES}"; then
    echo "WARNING: file was tampered! This is expected in this test."
fi
if [[ ! -e "${TEST}/tampered.txt.verifier.txt" ]]; then
    echo "Error: Required path '${TEST}/tampered.txt.verifier.txt' not found."
    cnt_error=$((cnt_error + 1))
else
    echo "----------------------------------------"
    echo "-- Check verified log file from tampered encrypted file"
    echo "----------------------------------------"
    echo " "
    HASH1=$(sha256sum "${TEST}/plainlog.txt" | awk '{ print $1 }')
    if echo "${HASH1}  ${TEST}/tampered.txt.verifier.txt" | sha256sum -c - >/dev/null 2>&1; then
        # 'sha256sum -c' exits with 0 (success) if they match
        echo "Original log file is identical to verifier output as expected."
        echo " "
    else
        echo "Even if tampered, the plain log file should be the same!"
        cnt_error=$((cnt_error + 1))
        # 'sha256sum -c' exits with non-zero (failure) if they don't match
        echo "-- INFO ---"
        echo "Original log file is different from verifier output!"
    fi
fi

if [[ ${COPY_TO_HOME_BACKUP} == "true" ]]; then
    cp -R "${TEST}/." "${HOME_BACKUP}/"
fi

# cleanup /tmp/${SUBFOLDER_TEST}/
rm -rf /tmp/"${SUBFOLDER_TEST}"/"${SUBFOLDER}_${PATH_SUFFIX}"/

echo " "
echo "----------------------------------------"
echo "-- Used binaries"
echo "----------------------------------------"
echo " "
echo "ls -alt ${BIN}/"
ls -alt "${BIN}/"
echo " "
echo "ls -alt ${SBIN}/"
ls -alt "${SBIN}/"
echo " "

sha256sum "${BIN}/cr_logger"
sha256sum "${BIN}/cr_verifier"
echo " "

echo "You might want to call this script like:"
echo "$0 2>&1 | tee ./protocol_${SCRIPTNAME}_${NOW}.log"
if [[ ${COPY_TO_HOME_BACKUP} == "true" ]]; then
    echo "$0 2>&1 | tee ${HOME_BACKUP}/protocol_${SCRIPTNAME}_${NOW}.log"
    echo " "
fi
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
