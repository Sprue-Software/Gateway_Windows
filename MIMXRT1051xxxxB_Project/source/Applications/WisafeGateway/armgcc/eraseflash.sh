#!/bin/bash

GITROOT=`git rev-parse --show-toplevel`

FLASHLOADERDIR="$GITROOT/src/Port/Tools/boardfiles"

PATHTOMFGTOOL="$GITROOT/src/Port/Tools/mfgtools/mfgtools/uuu/uuu"

PATHTOELFTOSB="$GITROOT/src/Port/Tools/Flashloader_i.MXRT1050_GA/Flashloader_RT1050_1.1/Tools/elftosb/win/elftosb.exe"

PATHTOSREC="$GITROOT/src/Port/Source/Applications/WisafeGateway/armgcc/copydown/ea3_wsgw.srec"

PATHTOBDFILE="$GITROOT/src/Port/Tools/boardfiles/imx-sdram-unsigned-dcd.bd"

PATHTOBDSPIFILE="$GITROOT/src/Port/Build/RT1050FlashTools/Flashloader/bd_file/imx10xx/program_flexspinor_image_qspinor.bd"

PATHTOBLHOST="$GITROOT/src/Port/Tools/Flashloader_i.MXRT1050_GA/Flashloader_RT1050_1.1/Tools/blhost/win/blhost.exe"
#Convert srec to imx file using elftosb

echo "Make sure the flash erased or the board is booted with the jumper closed\r\n"
echo "Waiting for USB device.....\r\n"

while :
do
  if [ "$(lsusb | grep -cim1 "1fc9:0130")" -ge 1 ]; then break; fi

done

#Erase flash on board...
eval $PATHTOMFGTOOL -v $FLASHLOADERDIR/erase_flash.bin

read -n 1 -s -p "Erasing flash. Check serial output to see progress and reset board once finished.\r\nPress any key when ready to proceed...\r\n"  
