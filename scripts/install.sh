################################################################################
# File: install.sh
# Description:
#   Installs development, design, and analysis software required by the project.
#   Automates system dependency installation (via dnf), CppUTest setup, and
#   optional Flatpak-based tooling for GUI and system modelling work.
#
# Features:
#   - Development environment setup (compilers, tools, libraries)
#   - Automatic installation of CppUTest framework
#   - Installation of design tools (Cambalache)
#   - Installation of analysis tools (Gaphor)
#
# Usage:
#   ./install.sh <install-type>
#
#   Supported install types:
#       develop – Install build/test/debug dependencies
#       design  – Install GUI design tools
#       analysis – Install modelling/analysis tools
#
################################################################################
#!/bin/bash


readonly INSTALLATION_OPTION=("develop" "design" "analysis")


function run_install()
{
        local install=$1
        if [[ $install == ${INSTALLATION_OPTION[0]} ]];
        then
                $ECHO "${INFO_COLOR}INFO: Installing required development packages${END_COLOR}"
                $ECHO "${SUCCESS_COLOR}"
                install_develop
                $ECHO "${END_COLOR}"
        elif [[ $install == ${INSTALLATION_OPTION[1]} ]];
        then
                $ECHO "${INFO_COLOR}INFO: Installing 'Design' software${END_COLOR}"
                $ECHO "${SUCCESS_COLOR}"
                install_design
                $ECHO "${END_COLOR}"
        elif [[ $install == ${INSTALLATION_OPTION[2]} ]];
        then
                $ECHO "${INFO_COLOR}INFO: Installing 'Analysis' software${END_COLOR}"
                $ECHO "${SUCCESS_COLOR}"
                install_analysis
                $ECHO "${END_COLOR}"
        else
                $ECHO "${ERROR_COLOR}ERROR: Currently, I do not support that kind of statistics${END_COLOR}"
        fi
}

function install_develop()
{
        DNF=$(which dnf)
        dnf_packages=(python3 lcov cloc cppcheck git texinfo help2man make cmake dh-autoreconf autoconf
                      automake binutils bison valgrind check gcovr gcc gcc-c++ glibc-devel curl gtk4
                      gtk4-devel gtkmm4.0-devel cairomm cairomm-devel libcurl libcurl-devel poppler-cpp-devel
                      debuginfo-install gtkmm30 glibmm24 xorg-x11-server-Xvfb)

        if [[ ! -z $DNF ]];
        then
                sudo dnf update
                sudo dnf -y install ${dnf_packages[@]}
                sudo dnf swap libcurl-minimal libcurl
                install_cpputest
                flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
        else
                $ECHO "${ERROR_COLOR}ERROR: This system does not use the dnf based package manage${END_COLOR}"
        fi
}

function install_cpputest()
{
        if [[ -z "$CPPUTEST_HOME" ]];
        then
                local tool_dir=$(pwd)/../
                cd $tool_dir
                git clone https://github.com/cpputest/cpputest.git
                cd cpputest
                autoreconf --install
                ./configure
                make tdd
                echo "export CPPUTEST_HOME=$tool_dir/cpputest/" >> /home/$(whoami)/.bashrc
                source /home/$(whoami)/.bashrc
        else
                $ECHO "${INFO_COLOR}INFO: CppUTest is already installed @ $CPPUTEST_HOME${END_COLOR}"
        fi
}

function install_design()
{
        flatpak install flathub ar.xjuan.Cambalache
}

function install_analysis()
{
        flatpak install flathub org.gaphor.Gaphor
}
