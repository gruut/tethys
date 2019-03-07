#!/bin/bash

function usage()
{
    printf "\\tUsage: %s \\n\\t[Build Option -o <Debug|Release>] \\n"
    exit 1
}

if [[ "$(id -u)" -ne 0 ]]; then
        printf "\n\tThis requires sudo. Please run with sudo.\n\n"
        exit -1
fi

SOURCE_DIR=$(pwd)
BUILD_DIR="${SOURCE_DIR}/build"
CMAKE_BUILD_TYPE=Debug
INSTALL_PREFIX="/usr/local/gruut"
CMAKE=$(command -v cmake)

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

printf ">>>>>>>> Build Info\\n"
printf "\\tCurrent branch: %s\\n" "$( git rev-parse --abbrev-ref HEAD )"
printf "\\n\\tARCHITECTURE: %s\\n" "$( uname )"
printf "\\n\\tCMAKE_BUILD_TYPE=%s\\n" "${CMAKE_BUILD_TYPE}"
printf ">>>>>>>>\\n"

if [[ ! -d "${BUILD_DIR}" ]]; then
    mkdir -p ${BUILD_DIR}
fi

cd ${BUILD_DIR}
${CMAKE} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="/usr/local/gruut" ${SOURCE_DIR}
${CMAKE} --build ${BUILD_DIR}

sudo make install
