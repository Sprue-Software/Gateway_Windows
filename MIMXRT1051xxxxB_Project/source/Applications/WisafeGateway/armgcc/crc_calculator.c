// This Code is to calculate CRC Of Wisafe GateWay Application Code
//Auther:Nishikant diwathe
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>  
#include <inttypes.h>
#include "copydown/imx_ea3_wsgw.h"


#define POLY 0x8408


uint16_t CalcCRC_16 (const uint8_t *data,int32_t length );
unsigned short crc16(unsigned char *data_p, int length);
unsigned char header[16];

//#define Size 50 
int main(int argc, char *argv[])
{
	FILE * file;
	FILE * file2;
  unsigned short calculated_CRC;
  unsigned short calculated_CRC2;
  unsigned char *year; 
  int year_format;
  time_t t ; 
  struct tm *tmp ; 
  char MY_TIME[50]; 
  time( &t ); 
      
    //localtime() uses the time pointed by t , 
    // to fill a tm structure with the  
    // values that represent the  
    // corresponding local time. 
      
   tmp = localtime( &t ); 
      
    // using strftime to display time 
  strftime(MY_TIME, sizeof(MY_TIME), "%x - %I:%M%p", tmp); 
      
  printf("Formatted date & time : %s\n", MY_TIME ); 
  printf("Formatted date  : %d\n", (1900+tmp->tm_year) ); 
  //tmp->tm_year += (1900+tmp->tm_year);
  printf("Formatted date year : %d\n", tmp->tm_year ); 
  year_format=tmp->tm_year+1900;
  printf("Formatted date year_format : %d\n", year_format); 
  year=(unsigned char *)&year_format;
  printf("Formatted year  : %d\n",year[0]  ); 
  printf("Formatted year1  : %d\n",year[1]  ); 
  unsigned char * crc_image;
  unsigned char *image_lth;
  unsigned char buff[16];
  unsigned int size =sizeof (imx_ea3_wsgw);
  crc_image=(unsigned char *)&calculated_CRC;
  image_lth=(unsigned char *)&size;
  printf ("Size of Image %x\n",size);
  calculated_CRC=CalcCRC_16(imx_ea3_wsgw, size);
  memset(header,0,16);

  header[0]=*image_lth++;
  header[1]=*image_lth++;
  header[2]=*image_lth++;
  header[3]=*image_lth;

  header[4]=*crc_image++;
  header[5]=*crc_image;
  header[6]=0; // Can Support 32 Bit CRC
  header[7]=0; // Can Support 32 Bit CRC


// Optional Requirement 

  //Date
  header[8]=tmp->tm_mday; //date optional
  header[9]=tmp->tm_mon; //Month optional
  header[10]=20;//year  optional 
  header[11]=19;//year  optional
  header[12]=1; //major optional
  header[13]=0; //Minor  optional



  printf ("\ncalculated_CRC of Image Spr %x\n\r",calculated_CRC);
  file2 = fopen ("Application_CRC_file.txt", "w");

  //char *buffer;
  unsigned long fileLen;
  for (int k=0;k<16;k++)
  {
    fprintf(file2, "%02x",header[k]);

  }


	fclose(file2);


  return 0;
}


uint16_t CalcCRC_16 (const uint8_t *data,int32_t length )
{
 uint16_t crc = 0xFFFF;
 for (int32_t x=0; x<length; x++)
    { 
        crc  = (crc >> 8) | (crc << 8);
        crc ^= data[x];
        crc ^= (crc & 0xFF) >> 4;
        crc ^= (crc << 12) ;
        crc ^= ((crc & 0xFF) << 5) ;
       
    }
    return crc;
}



unsigned short crc16(unsigned char *data_p, int length)
{
      unsigned char i;
      unsigned int data;
      unsigned int crc = 0xffff;

      if (length == 0)
            return (~crc);

      do
      {
            for (i=0, data=(unsigned int)0xff & *data_p++;
                 i < 8; 
                 i++, data >>= 1)
            {
                  if ((crc & 0x0001) ^ (data & 0x0001))
                        crc = (crc >> 1) ^ POLY;
                  else  crc >>= 1;
            }
      } while (--length);

      crc = ~crc;
      data = crc;
      crc = (crc << 8) | (data >> 8 & 0xff);

      return (crc);
}
