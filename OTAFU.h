// Over The Air Firmware Update
// 
// 

#ifndef OTAFU_H
#define OTAFU_H

#include "SST25VF010.h"

#define FU_IMAGE_SIZE 0x10000           // size of firmware download images
#define FU_IMAGE_START_ADDRESS 0x10000  // Start address of FU image in SMEM
#define FU_IMAGE_BLOCK0_ADDRESS FU_IMAGE_START_ADDRESS // First block for erasing
#define FU_IMAGE_BLOCKS 2               // Numbef of FU image blocks

// For http GET based downloader
#define FU_START_ADDRESS 0x10000
#define FU_BUFFER_SIZE  395
#define FU_TCP_TIMEOUT  5000 // milliseconds
#define HTTP_RANGE 4095                 // Size of data block used in GET Range:
//#define HTTP_RANGE 2047               // Alternative sizeused in GET Range:

#define FU_DOWNLOAD_DIRECTORY "/firmware/fupdate.bin"

#define BC_MAX_DOWNLOAD_ATTEMPTS 3      // Maximum number of download attempts

/*
 * FU mode
 */  
enum FU_mode {
    IDLE=0,
    START=1,
    LOAD=2,
    FINISH=3
}; 

int FU_check_image (int);
void OTAFU_erase_image (int);

// ***********************************************************************************************
// Firmware Update Process
// Fetches the file at location
// HTTP://IPaddress.path_file e.g.
// fupdate ("www.actifi.co.uk", "/firmware/fupdate.bin" 
// will fetch the binary file located at
// http://www.actifi.co.uk/firmware/fupdate.bin
//
// This function returns a -ve value on failure, and the filesize on sucess
// Return = -1 : failed to connect
// Return = -2 : Checksum error. 
// Return = File size on success
// Note: The binary file has the 2's compliment of 32bit word checksum of 
// the entire file as the last word in the file, so that the rolling checksum == 0 at end of file
//
int fupdate ( char IPaddress[], char path_file[] );

#endif