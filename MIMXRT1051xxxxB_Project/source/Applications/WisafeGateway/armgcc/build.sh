#!/bin/bash

source build.project.env
source build.global.env

export TOOLCHAIN_FILE=$(realpath $TOOLCHAIN_FILE_REL)
export ARMGCC_DIR=$(realpath $ARMGCC_DIR_REL)

cmake -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G "Unix Makefiles" . &&
./make.sh
