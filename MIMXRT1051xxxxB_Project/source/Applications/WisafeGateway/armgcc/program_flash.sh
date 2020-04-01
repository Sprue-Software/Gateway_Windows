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

cmd="wine $PATHTOELFTOSB -d -f kinetis -V -c $PATHTOBDSPIFILE -o  $FLASHOUTPUTSDIR/boot_image.sb  $FLASHOUTPUTSDIR/myfile_nopadding.imx"
#echo $cmd
eval $cmd
popd

#Now that flashloader has run, board will come back as an alternative USB device
echo -e "${BLUE}Waiting for USB device.....\r\n${NC}"

while :
do
  if [ "$(lsusb | grep -cim1 "1fc9:0130")" -ge 1 ]; then break; fi

done

#Program board with flashloader.. assumes USB is ready to go..
eval $PATHTOMFGTOOL -v $FLASHLOADERDIR/ivt_flashloader.bin 

#Now that flashloader has run, board will come back as an alternative USB device
echo -e "${BLUE}Waiting for USB device.....\r\n${NC}"

while :
do
  if [ "$(lsusb | grep -cim1 "15a2:0073")" -ge 1 ]; then break; fi
done

# uuu needs /dev/hidraw - this appear a short time after the device appears so hokey delay 
sleep 1
# Check that flashloader has started correctly
echo -e ".....Found USB Device\r\n"

cmd=$PATHTOBLHOST" -u 0x15a2,0x0073 -- get-property 1"
#echo $cmd
out=$(eval $cmd | grep -cim1 Success)
#echo $out

if [ "$out" -ge 1 ]; then echo -e "\r\nFlashloader started OK\r\n"; else exit 1; fi
 
# Should give us a success string check for this

out=$(eval $PATHTOBLHOST -u 0x15a2,0x0073 receive-sb-file $FLASHOUTPUTSDIR/boot_image.sb)
echo -e $out
