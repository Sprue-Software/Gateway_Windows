#!/bin/bash

GITROOT=`git rev-parse --show-toplevel`

FLASHOUTPUTSDIR="$GITROOT/src/Port/Source/Applications/WisafeGateway/armgcc/flash"

FLASHLOADERDIR="$GITROOT/src/Port/Tools/boardfiles"

PATHTOMFGTOOL="$GITROOT/src/Port/Tools/mfgtools/mfgtools/uuu/uuu"

PATHTOELFTOSB="$GITROOT/src/Port/Tools/Flashloader_i.MXRT1050_GA/Flashloader_RT1050_1.1/Tools/elftosb/win/elftosb.exe"

PATHTOSREC="$GITROOT/src/Port/Source/Applications/WisafeGateway/armgcc/copydown/ea3_wsgw.srec"

PATHTOBDFILE="$GITROOT/src/Port/Tools/boardfiles/imx-sdram-unsigned-dcd.bd"

PATHTOBDSPIFILE="$GITROOT/src/Port/Build/RT1050FlashTools/Flashloader/bd_file/imx10xx/program_flexspinor_image_qspinor.bd"

PATHTOBLHOST="$GITROOT/src/Port/Tools/Flashloader_i.MXRT1050_GA/Flashloader_RT1050_1.1/Tools/blhost/linux/amd64/blhost"

#Convert srec to imx file using elftosb
BLUE="\033[0;34m"
NC="\033[0m"
echo -e "${BLUE}Make sure the flash is erased\r\n${NC}"

pushd $FLASHLOADERDIR
cmd="wine $PATHTOELFTOSB -d -f imx -V -c $PATHTOBDFILE -o $FLASHOUTPUTSDIR/myfile.imx $PATHTOSREC"
#echo $cmd
eval $cmd
popd

#Program board.. assumes USB is ready to go..
eval $PATHTOMFGTOOL -v $FLASHOUTPUTSDIR/myfile_nopadding.imx

