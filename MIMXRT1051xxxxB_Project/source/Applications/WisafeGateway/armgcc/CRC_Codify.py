import os
import sys
import csv
import io
import binascii
import cPickle as pickle
#calculate CRC
#date
#version
#length
finalcrc=0
def CalcCRC (data,length ):
    crc = 0xFFFF;
    for x in range(length):
        crc  = (crc >> 8) | (crc << 8)
        #print(data[x])
        crc ^= data[x]
        crc ^= (crc & 0xFF) >> 4
        crc ^= (crc << 12)
        crc ^= ((crc & 0xFF) << 5)
        finalcrc=crc

def main(argv):
	# Acquire parameters
    binFilepath="copydown/ea3_wsgw.imx"
    headerFilename="copydown/CRC_ea3_wsgw.imx"
    header_info="Application_CRC_file.txt"
    # Open output binary file
   # headerFilename = projPath + "/" + HEADER_FILENAME + projName + ".bin"
    with open(headerFilename, "wb") as out:
        with open(header_info, "r") as out1:
            data=out1.read(32)
            y = str(data)
            print(y)
            data_wr=bytearray.fromhex(y)
            print(data_wr[4])
            #pickle.dump(data,out)
            #binascii.a2b_hex(binascii.hexlify(data))
            out.write(data_wr)
            with open(binFilepath, "rb") as f:
		f.seek(16)
                byte = f.read(os.path.getsize(binFilepath))
                print(os.path.getsize(binFilepath))
                #CalcCRC(bytearray(byte),os.path.getsize(binFilepath))
                out.write(byte);
		# Done
    print("Image file codified successfully",finalcrc)



if __name__ == "__main__":
    main(sys.argv)

