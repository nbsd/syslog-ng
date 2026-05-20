#!/bin/bash
#############################################################################
# Copyright (c) 2026 Airbus Commercial Aircraft
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
# File:   cli27_ldd.sh
# Date:   2026-05-20
#
# Helper script to call ldd for build binaries
#
# Preconditions:
#   Build done successfully and PREFIX points to binary folder
#
# Usage:
#   ./cli27_ldd.sh
#
#-----------------------------------------------------------------------

# set -x
set -o pipefail

VERSION="Version 1.0.0"

#-- for branch withiout crash recovery modules this must be false
IS_CRASH_RECOVERY="true"
#IS_CRASH_RECOVERY="false"

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

ARTIFACTS=(
    "${PREFIX}/sbin/syslog-ng"
    "${PREFIX}/sbin/syslog-ng-ctl"
    "${PREFIX}/bin/loggen"
    "${PREFIX}/bin/slogkey"
    "${PREFIX}/bin/slogencrypt"
    "${PREFIX}/bin/slogverify"
    "${PREFIX}/bin/loggen"
    "${PREFIX}/sbin/syslog-ng-ctl"
    "${PREFIX}/sbin/syslog-ng"
)

if [[ ${IS_CRASH_RECOVERY} == "true" ]]; then
    ARTIFACTS+=("${PREFIX}/bin/cr_logger" "${PREFIX}/bin/cr_verifier")
fi

HOMESLOGTEST=${SCRIPT_DIR}

BIN=${PREFIX}/bin
SBIN=${PREFIX}/sbin

SUBFOLDER_TEST="test_slog"
HOME_BACKUP=${HOME}/${SUBFOLDER_TEST}/${SCRIPTNAME}

# current Git branch for build log name
GIT_BRANCH=""
echo " "
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
else
    echo "Not a Git repository."
fi
if [[ -z ${GIT_BRANCH} ]]; then
    GIT_BRANCH="unkown-git-branch"
else
    echo "GIT_BRANCH: ${GIT_BRANCH}"
fi

LOGFILE="${HOME_BACKUP}/${NOW}_${GIT_BRANCH}_ldd.txt"
echo "File: ${LOGFILE}" >"${LOGFILE}"
# xxx  2>&1 | tee -a "${LOGFILE}"

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
    echo "SBIN: ${SBIN}"
    echo "HOME_BACKUP: ${HOME_BACKUP}"

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

# ----------------------------------------------------------------------
# -- function to call ldd for artifacts in given argument
# Usage: ldd_artifacts "path1" "path2" "path3" ...
ldd_artifacts() {
    local cnt_missing=0
    local path
    for path in "$@"; do
        if [[ ! -e ${path} ]]; then
            echo "ERROR: Required file '${path}' not found." 2>&1 | tee -a "${LOGFILE}"
            ((cnt_missing++))
        else
            {
                echo "sha256sum ${path}"
                sha256sum "${path}"
                echo
                echo "ldd ${path}"
                ldd "${path}"
                echo
                echo
            } 2>&1 | tee -a "${LOGFILE}"
        fi
    done
    # Return 0 if all exist, or 1 if any are missing
    [[ ${cnt_missing} -eq 0 ]]
}

echo " "
"${SCRIPT_DIR}"/get_git_info.sh

# -- do initial checks -----
check_missing "${PREFIX}" "${BIN}" "${SBIN}"

mkdir -p "${HOMESLOGTEST}"
check_missing "${HOMESLOGTEST}"

{
    echo " "
    echo "ls -alt ${PREFIX}/lib/"
    ls -alt "${PREFIX}/lib/"
} 2>&1 | tee -a "${LOGFILE}"

{
    echo " "
    echo "ls -alt ${PREFIX}/sbin/"
    ls -alt "${PREFIX}/sbin/"
} 2>&1 | tee -a "${LOGFILE}"

{
    echo " "
    echo "ls -alt ${PREFIX}/bin/"
    ls -alt "${PREFIX}/bin/"
} 2>&1 | tee -a "${LOGFILE}"

echo " "

if ldd_artifacts "${ARTIFACTS[@]}"; then
    echo -e "\nSuccess: All aritfacts found." 2>&1 | tee -a "${LOGFILE}"
else
    echo -e "\nFailure: Not all artifacts founds." 2>&1 | tee -a "${LOGFILE}"
    cnt_error=$((cnt_error + 1))
fi

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
