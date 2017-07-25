
/*
* Chip & Mfr. IDs
*/
#define SST_MFG_ID_ADDR     0x00    // Address of manufacturer's ID
#define SST_MFR_ID          0xbf    // Manufacturer ID
#define SST_DEVICE_ID_ADDR  0x01    // Address of device ID
#define SST_DEVICE_ID       0x49    // Device ID

/*
* Drivers
*/
void ExtFLASHRead(int, uint8_t*, int);  // Read n bytes from address into buffer
void ExtFLASHWrite(int, uint8_t*, int); // Write n bytes from buffer to address

void ExtFLASHByteWrite(int, uint8_t);   // Write a byte to address
int ExtFLASHByteRead(int);              // Read a byte from address

void ExtFLASHSectorErase(int);          // Erase a sector
void ExtFLASHBlockErase(int);           // Erase a block
void ExtFLASHChipErase(void);           // Erase the whole chip

int  ExtFLASHRDSR (void);               // Read status register
void ExtFLASHWRSR (int);                // Write status register

// Support for Firmware Update
void ExtFLASHOpen(int address, uint8_t flashData);
void ExtFLASHIncWrite (uint8_t flashData);
void ExtFLASHClose(void);
#endif
