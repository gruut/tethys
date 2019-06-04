#!/bin/bash

SOURCE_DIR=$(pwd)
BUILD_DIR="${SOURCE_DIR}/build"
CMAKE_BUILD_TYPE=Debug
INSTALL_PREFIX="/usr/local/tethys"
CMAKE=$(command -v cmake)
ARCH=$(uname)

txtbld=$(tput bold)
bldred=${txtbld}$(tput setaf 1)
bldblue=${txtbld}$(tput setaf 4)
txtrst=$(tput sgr0)

function usage()
{
    printf "\\tUsage: %s \\n\\t[Build Option -o <Debug|Release>] \\n"
    exit 1
}

function check_and_require_sudo()
{
    if [[ "$(id -u)" -ne 0 ]]; then
        printf "${bldblue}It requires sudo. Please insert the password. \n\n"
        return 0
    fi
}

while getopts "o:" option; do
    case ${option} in
        o)
            options=( "Debug" "Release" )
            if [[ "${options[*]}" =~ "${OPTARG}" ]]; then
                CMAKE_BUILD_TYPE=${OPTARG}
            else
                printf "\\nInvalid argument: %s\\n"
                usage
                exit 1
            fi
        ;;
    esac
done

printf "${bldred}>>>>>>>> Build Info\\n"
printf "\\tCurrent branch: %s\\n" "$( git rev-parse --abbrev-ref HEAD )"
printf "\\n\\tARCHITECTURE: %s\\n" "${ARCH}"
printf "\\n\\tCMAKE_BUILD_TYPE=%s\\n" "${CMAKE_BUILD_TYPE}"
printf "\\n"
printf "\\n\\tBuild order\\n"
printf "\\n\\t1. Install dependencies\\n"
printf "\\n\\t2. CMake build\\n"
printf "\\n\\t3. Compile\\n"
printf ">>>>>>>>\\n${txtrst}"

# Install dependencies
if [[ ${ARCH} == "Linux" ]]; then
    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )

    case ${OS_NAME} in
        "Ubuntu")
            check_and_require_sudo
            if [[ ! -d "/usr/local/include/spdlog" ]]; then
                git clone https://github.com/gabime/spdlog.git && cd spdlog
                cmake -DSPDLOG_BUILD_EXAMPLES=OFF \
                      -DSPDLOG_BUILD_BENCH=OFF \
                      -DSPDLOG_BUILD_TESTS=OFF \
                      -DCMAKE_BUILD_TYPE=Release
                make
                make install
            fi
        ;;
    esac
elif [[ ${ARCH} == "Darwin" ]]; then
    brew install spdlog
fi

if [[ ! -d "/usr/local/include/soci" ]]; then
    check_and_require_sudo

    git clone git://github.com/SOCI/soci.git && cd soci
    mkdir -p build && cd build
    cmake -DSOCI_TESTS=OFF ..
    make
    make install
fi

# CMake build
if [[ ! -d "${BUILD_DIR}" ]]; then
    mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
${CMAKE} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" ${SOURCE_DIR}

# Compile & Install
if check_and_require_sudo; then
    sudo make install
else
    make install
fi
