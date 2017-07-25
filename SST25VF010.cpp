/*
 *
 * Drivers for SST25VF010A
 * SPI Serial FLASH device
 *
 * JWH 24/11/14
 */
 
#include "mbed.h"
#include "cc3000.h"
#include "main.h"
#include "SST25VF010.h"

using namespace mbed_cc3000;

extern SPI ExtFLASH; 
//extern DigitalOut ExtFLASHnHOLD;
extern DigitalOut ExtFLASHnCS;
//extern DigitalOut ExtFLASHnWP;

//
// Low-level SPI FLASH drivers
//
//************************************************************************************************
                        
/*
* Write command and address
*/
void ExtFLASHCmdADDR (int address, int cmd)
{
    ExtFLASH.write(cmd);
    address &= SST_ADDRESS_MASK;    // Ensure address is in device range 
    ExtFLASH.write(address>>16);    // Address MSB
    ExtFLASH.write(address>>8);
    ExtFLASH.write(address);        // Address LSB    
}

/*
* Write enable device
*/
void ExtFLASHWREN (void)
{
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write(SST_WRITE_ENABLE);
    ExtFLASHnCS=1;                  // FLASH nCS inactive
}

/*
* Write disable device
*/
void ExtFLASHWRDI (void)
{
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write(SST_WRITE_DISABLE);
    ExtFLASHnCS=1;                  // FLASH nCS inactive
}

/*
* Read status register
*/
int ExtFLASHRDSR (void)
{
int sr;

    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write(SST_READ_SR);
    sr=ExtFLASH.write(0x00);     
    ExtFLASHnCS=1;                  // FLASH nCS inactive
    return sr;
}    
    
/*
* Block or Sector erase
*/
void ExtFLASHBlkSecErase(int address, int cmd)
{
    ExtFLASHWREN();                 // Write enable
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, cmd);
    ExtFLASHnCS=1;                  // FLASH nCS inactive 
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
        {
        // Wait for BUSY bit to clear
        }
}

/*
* Enable Write Status Register
*/
void ExtFLASHEWSR (void)
{
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write(SST_ENABLE_WRITE_SR);
    ExtFLASHnCS=1;                  // FLASH nCS inactive
}

/*
* Write Status Register
* Automatically executes EWSR before actual write to SR
*/
void ExtFLASHWRSR (int data)
{
    ExtFLASHEWSR();
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write(SST_WRITE_SR);
    ExtFLASH.write(data);
    ExtFLASHnCS=1;                  // FLASH nCS inactive
}

//
// Mid-level SPI FLASH drivers
//
//************************************************************************************************

/*
 * Read bytes
 */
void ExtFLASHRead(int address, uint8_t* databuffer, int numbytes)
{
int i;

    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, SST_READ);
    for (i=0; i<numbytes; i++)      // Get the data (dummy writes)
        {
        *(databuffer+i)= ExtFLASH.write(0x00);  
        }
    ExtFLASHnCS=1;                  // FLASH nCS inactive 
}

/*
 * Write bytes
 * Target memory MUST be in erased state (0xff) before writing 
 */


void ExtFLASHWrite(int address, uint8_t* databuffer, int numbytes)
{
int i;

    ExtFLASHWREN();                 // Write enable
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, SST_AAI_PROGRAM);
    ExtFLASH.write(*databuffer);    // Write first data byte
    ExtFLASHnCS=1;                  // FLASH nCS inactive
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
        {
        // Wait for BUSY bit to clear
        }    
    for (i=1; i<numbytes; i++)    // Put the rest of the data
        {
        ExtFLASHnCS=0;                   // FLASH nCS active
        ExtFLASH.write(SST_AAI_PROGRAM); // Write command
        ExtFLASH.write(*(databuffer+i)); // Write data
        ExtFLASHnCS=1;                   // FLASH nCS inactive
        while (ExtFLASHRDSR() & SR_BUSY_MASK)
            {
            // Wait for BUSY bit to clear
            }    
        }
    ExtFLASHWRDI();                 // Write disable
    while (ExtFLASHRDSR() & SR_AAI_MASK)
        {
        // Wait for AAI bit to clear
        }
}

/*
* Byte read
*/
int ExtFLASHByteRead(int address)
{
int data;

    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, SST_READ);
    data = ExtFLASH.write(0x00);  
    ExtFLASHnCS=1;                  // FLASH nCS inactive 
    return data;
}

/*
 * Byte program
 * Target address MUST be in erased state (0xff) before writing
 *
 */
void ExtFLASHByteWrite(int address, uint8_t databyte)
{
    ExtFLASHWREN();                 // Write enable
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, SST_BYTE_PROGRAM);
    ExtFLASH.write(databyte);  
    ExtFLASHnCS=1;                  // FLASH nCS inactive 
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
        {
        // Wait for BUSY bit to clear
        }
}

/*
* Sector erase
*/
void ExtFLASHSectorErase(int address)
{
    ExtFLASHBlkSecErase (address, SST_SECTOR_ERASE);
}

/*
* Block erase
*/
void ExtFLASHBlockErase(int address)
{
    ExtFLASHBlkSecErase (address, SST_BLOCK_ERASE);
}

/*
* Chip erase
*/
void ExtFLASHChipErase(void)
{
    ExtFLASHWREN();                 // Write enable
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASH.write (SST_CHIP_ERASE);
    ExtFLASHnCS=1;                  // FLASH nCS inactive 
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
        {
        // Wait for BUSY bit to clear
        }
}
// **************************
// Support for Firmware Update

void ExtFLASHOpen(int address, uint8_t flashData)
{
    ExtFLASHWREN();                 // Write enable
    ExtFLASHnCS=0;                  // FLASH nCS active
    ExtFLASHCmdADDR (address, SST_AAI_PROGRAM);
    ExtFLASH.write(flashData);      // Write first data byte
    ExtFLASHnCS=1;                  // FLASH nCS inactive
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
    {
        // Wait for BUSY bit to clear
    }  
}          
void ExtFLASHIncWrite (uint8_t flashData)
{
/*  ExtFLASHnCS=0;                   // FLASH nCS active
    ExtFLASH.write(SST_AAI_PROGRAM); // Write command
    ExtFLASH.write(flashData);       // Write data
    ExtFLASHnCS=1;                   // FLASH nCS inactive
    while (ExtFLASHRDSR() & SR_BUSY_MASK)
    {
        // Wait for BUSY bit to clear
    }   
*/     
}
void ExtFLASHClose(void)
{
    ExtFLASHWRDI();                 // Write disable
    while (ExtFLASHRDSR() & SR_AAI_MASK)
    {
        // Wait for AAI bit to clear
    }
}
