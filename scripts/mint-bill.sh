################################################################################
# File: mint-bill.sh
# Description:
#   Main entrypoint script for building, testing, installing, and gathering
#   statistics for the Mint-Bill project. Delegates operations to build.sh,
#   test.sh, install.sh, and statistics.sh. Provides complete project lifecycle
#   tooling through a unified command interface.
#
# Features:
#   - Command-line argument parsing
#   - Environment validation
#   - Dispatch for build, test, install, and metrics
#   - ANSI-colored messaging
#
# Usage:
#   ./mint-bill.sh [--build] [--test] [--install] [--stats]
#   ./mint-bill.sh -h
#
################################################################################
#!/bin/bash
source $(dirname "$0")/build.sh
source $(dirname "$0")/test.sh
source $(dirname "$0")/install.sh
source $(dirname "$0")/statistics.sh

readonly YES=0
readonly NO=1
readonly ECHO="echo -e"
readonly INFO_COLOR="\e[1;36m"
readonly ERROR_COLOR="\e[1;31m"
readonly SUCCESS_COLOR="\e[1;32m"
readonly END_COLOR="\e[0m"


function main()
{
        environment_verification $#
        parse_command_line_arguments $@
}

function environment_verification()
{
        local all_arguments=$1
        if [[ $(basename `pwd`) != $PROJECT_NAME ]];
        then
                $ECHO "${ERROR_COLOR}ERROR: $(basename "$0") must be called from:\n${END_COLOR}"
                $ECHO "${ERROR_COLOR}\t$(dirname `pwd`)\n\r${END_COLOR}"
                $ECHO "${INFO_COLOR}INFO: Currently, you are here:\n${END_COLOR}"
                $ECHO "${INFO_COLOR}\t$(pwd)${END_COLOR}"
                exit 1;
        fi

        if [[ $all_arguments -eq 0 ]];
        then
                help
                exit 1;
        fi
}

function parse_command_line_arguments()
{
        local test_type=""
        local stats_type=""
        local build_type=""
        local install_type=""
        while [[ -n "$1" ]];
        do
                case $1 in
                        -b | --build | -b=* | --build=*)
                                is_argument_value_provided $@
                                if return_value_not_ok;
                                then
                                        $ECHO "${ERROR_COLOR}ERROR: Build option not specified${END_COLOR}"
                                        help
                                        exit 1
                                else
                                        if [[ ($1 == "-b") || ($1 == "--build") ]];
                                        then
                                                build_type=$(extract_argument $2)
                                                shift
                                        else
                                                build_type=$(extract_argument $1)
                                        fi
                                        run_build $(to_lower $build_type)
                                fi
                                ;;
                        -t | --test | -t=* | --test=*)
                                is_argument_value_provided $@
                                if return_value_not_ok;
                                then
                                        $ECHO "${ERROR_COLOR}ERROR: Test option not specified${END_COLOR}"
                                        help
                                        exit 1
                                else
                                        if [[ ($1 == "-t") || ($1 == "--test") ]];
                                        then
                                                test_type=$(extract_argument $2)
                                                shift
                                        else
                                                test_type=$(extract_argument $1)
                                        fi
                                        run_test $(to_lower $test_type)
                                fi
                                ;;
                        -s | --stats | -s=* | --stats=*)
                                is_argument_value_provided $@
                                if return_value_not_ok;
                                then
                                        $ECHO "${ERROR_COLOR}ERROR: Statistic option not specified${END_COLOR}"
                                        help
                                        exit 1
                                else
                                        if [[ ($1 == "-s") || ($1 == "--stats") ]];
                                        then
                                                stats_type=$(extract_argument $2)
                                                shift
                                        else
                                                stats_type=$(extract_argument $1)
                                        fi
                                        run_statistics $(to_lower $stats_type)
                                fi
                                ;;
                        -i | --install | -i=* | --install=*)
                                is_argument_value_provided $@
                                if return_value_not_ok;
                                then
                                        $ECHO "${ERROR_COLOR}ERROR: Installation option not specified${END_COLOR}"
                                        help
                                        exit 1
                                else
                                        if [[ ($1 == "-i") || ($1 == "--install") ]];
                                        then
                                                install_type=$(extract_argument $2)
                                                shift
                                        else
                                                install_type=$(extract_argument $1)
                                        fi
                                        run_install $(to_lower $install_type)
                                fi
                                ;;
                        -h | --help)
                                help
                                ;;
                        *)
                                help
                                exit 1
                                ;;
                esac
                shift
        done
}

function help()
{
        $ECHO "${INFO_COLOR}\n\rUsage: ./scripts/mintbill.sh [OPTION]${END_COLOR}"
        $ECHO "${INFO_COLOR}Building, testing, and analyzing a projects code${END_COLOR}"
        $ECHO
        $ECHO "${INFO_COLOR}OPTIONS:${END_COLOR}"
        $ECHO "${INFO_COLOR}\t-b, -b=, --build, --build=SUPPORTED_BUILD${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\tSUPPORTED_BUILD:${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[HOST], compiles for the host computer you are developing on (e.g. Linux PC or Laptop)${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[RPM], [DEPLOY], builds an offline Fedora RPM (TENANT=tme|tce).\n\r${END_COLOR}"
        $ECHO "${INFO_COLOR}\t-t, -t=, --test, --test=SUPPORTED_TEST${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\tSUPPORTED_TEST:${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[SCA], performs static code analysis using Cppcheck${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[COVERAGE], performs code coverage with gcovr${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[UNIT], runs all unit tests in your project using CppUTest${END_COLOE}"
        $ECHO "${INFO_COLOR}\t\t\t[SHOW], graphically displays the code coverage in firefox\n\r${END_COLOE}"
        $ECHO "${INFO_COLOR}\t-s, -s=, --stats, --stats=SUPPORTED_STATISTICS${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\tSUPPORTED_STATISTICS:${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[LC], takes a source code line and file count\n\r${END_COLOR}"
        $ECHO "${INFO_COLOR}\t-i, -i=, --install, --install=SUPPORTED_INSTALLS${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\tSUPPORTED_INSTALLS:${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[DEVELOP], installs all required packages for the project${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[DESIGN], installs the Cambalache application used for GUI design${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[ANALYSIS], installs the Gaphor application for block diagram analysis${END_COLOR}"
        $ECHO "${INFO_COLOR}\t-h, --help${END_COLOR}"
        $ECHO "${INFO_COLOR}\t\t\t[HELP], displays this information\n\r${END_COLOR}"
}

function is_argument_value_provided()
{
        local valid=$NO
        if [[ ("$1" == *=* && -n ${1#*=}) || ( -n "$2" && "$2" != -*) || ( -n "$3" && "$3" != -*) ]];
        then
                valid=$YES
        fi

        return $valid
}

function remove_directory()
{
        local dirname=$1
        if directory_exists $dirname;
        then
                rm -rf $dirname
        fi
}

function directory_exists()
{
        [[ -d "${1}" ]];
}

function extract_argument()
{
        echo "${2:-${1#*=}}"
}

function return_value_not_ok()
{
        [[ $? -ne 0 ]];
}

function to_lower()
{
        echo "${1}" | tr '[:upper:]' '[:lower:]'
}

main $@
