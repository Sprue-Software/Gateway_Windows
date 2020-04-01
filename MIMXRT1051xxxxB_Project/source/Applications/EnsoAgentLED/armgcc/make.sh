#!/bin/bash

source build.project.env
source build.global.env

export TOOLCHAIN_FILE=$(realpath $TOOLCHAIN_FILE_REL)
export ARMGCC_DIR=$(realpath $ARMGCC_DIR_REL)

make -j5 &&

if [ "$BUILD_TYPE" = "ramdebug" ]
then
	$ARMGCC_DIR/bin/arm-none-eabi-gdb ramdebug/$BIN_NAME.elf -x $DEBUGGER_FILE
fi
