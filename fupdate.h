// HTTP Get for iAqua firmware update
// 
// 

#ifndef FUPDATE_H
#define FUPDATE_H
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
 
#endif// HTTP Get for iAqua firmware update
