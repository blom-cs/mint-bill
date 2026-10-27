################################################################################
# File: build.sh
# Description:
#   Provides build automation for the project, supporting host development
#   builds and offline Fedora RPM production builds.
#
# Features:
#   - Host build mode (source-tree paths, Debug)
#   - RPM / deploy build mode (FHS paths, Release, rpmbuild)
#
# Usage:
#   ./scripts/mint-bill.sh --build host
#   ./scripts/mint-bill.sh --build rpm          # tenant defaults to tme
#   TENANT=tce ./scripts/mint-bill.sh --build rpm
#
#   Supported build types:
#       host    – Build for local development machine
#       rpm     – Build an offline RPM (and dependency set when possible)
#       deploy  – Alias for rpm
#
################################################################################
#!/bin/bash


# Release version for the RPM, the CMake project, and the bundle directory.
# The bundle name is mint-bill${PROJECT_VERSION_PREFIX}${PROJECT_VERSION_NUM}.
PROJECT_VERSION_NUM=1.0.0
PROJECT_VERSION_PREFIX=-v
BIN_SUFFIX=
CMAKE=cmake
BUILD_TYPE="Debug"
BUILD_DIR=build
APPLICATION_NAME=$(basename `pwd`)
APPLICATION_ID="org.app.$APPLICATION_NAME-tme"
BIN_DIR=$BUILD_DIR/$APPLICATION_NAME

function run_build()
{
        local build_type=$1
        case "${build_type}" in
                host)
                        $ECHO "${INFO_COLOR}Building for the host platform${END_COLOR}"
                        remove_directory $BUILD_DIR
                        host_build
                        ;;
                rpm|deploy)
                        $ECHO "${INFO_COLOR}Building an offline RPM package${END_COLOR}"
                        rpm_build
                        ;;
                *)
                        $ECHO "${ERROR_COLOR}ERROR: Build type not supported${END_COLOR}"
                        ;;
        esac
}

function host_build()
{
        mkdir -p $BIN_DIR
        $CMAKE -S . -B $BIN_DIR --warn-uninitialized -DCMAKE_BUILD_TYPE=$BUILD_TYPE  \
                -DAPP_NAME=$APPLICATION_NAME  \
                -DAPP_ID=$APPLICATION_ID  \
                -DCMAKE_EXECUTABLE_SUFFIX=$BIN_SUFFIX  \
                -DBUILD_PROJECT=ON
        $CMAKE --build $BIN_DIR
}

function rpm_build()
{
        local tenant=${TENANT:-all}
        bash "$(dirname "${BASH_SOURCE[0]}")/package-rpm.sh" "${tenant}"
}
