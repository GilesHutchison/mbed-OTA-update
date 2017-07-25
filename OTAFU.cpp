// ************************************
// iAqua:  Over The Air Firmware Update
// Author: Giles Hutchison
// Date:   23rd April 2015
// ************************************

#include "mbed.h"
#include "wdog.h"
#include "Messages.h"
#include "WSmessage.h"
#include "SST25VF010.h"
#include "OTAFU.h"

#include "cc3000.h"
#include "wifi.h"
#include "UDPSocket.h"
#include "main.h"
#include "cc3000_WiFi.h"
#include "Websocket.h"
#include "ACTiFi.h"
#include "EEPROM.h"
#include "SerialDebug.h"

//#define FU_DEBUG 
#define DBG_REPORT   if (FU_DEBUG) sendSoftSerial(DebugTextBuffer)

extern bool SendWSmessage;
extern char WSmessageBuf[WEBSOCKET_MSG_BUFFER_SIZE];  
extern char DebugTextBuffer[DEBUGBUFFERSIZE];
extern Watchdog wd;

// *******************************
// Debug - dump SMEM
// *******************************
#ifdef serialdebug    
void dump_SMEM(void)
{
int i,d;
    sprintf(DebugTextBuffer,"SMEM: "); REPORT;       
    for (i=0; i<16; i++)
        {
        d=ExtFLASHByteRead(i);
        sprintf(DebugTextBuffer,"a:%x d:%x, ",i,d); REPORT;       
        }
    sprintf(DebugTextBuffer,"\r\n"); REPORT;           
}
#endif

// *********************************
//  Erase an image in SMEM  
// *********************************
void OTAFU_erase_image (int address)
{
int i;

    for (i=0; i<FU_IMAGE_BLOCKS; i++)
        {
        #ifdef serialdebug
        sprintf(DebugTextBuffer,"Erasing SMEM %x \r\n",address); REPORT;   
        #endif
        ExtFLASHBlockErase(address);
        address=address+SST_BLOCK_SIZE;   
        }
}

//*********************************
//  Check firmare image in SMEM
//*********************************
int FU_check_image (int image)
{
uint32_t sum;   
uint32_t* data;
uint8_t* databuffer;
int i,address,chk;

#define VECTORSIZE 32                            // Size of image vector + checksum table

    databuffer = (uint8_t*)malloc(VECTORSIZE);   // Allocate buffer for image vectors
    if (databuffer==NULL)
        {
        return -1;
        }

    address=image*FU_IMAGE_SIZE;                 // Compute base address of image to check  
    ExtFLASHRead(address,databuffer,VECTORSIZE);
    
    sum=0;
    chk=0;
    data=(uint32_t*)databuffer;
    for (i=0; i<8; i++)                           // use fixed values to save space
        {
        sum = sum + *data;
        if (*data)                                // Check for all 0s
            {
            chk=1;    
            }
        data++;    
        } 
    free (databuffer);
    if ((!sum)&&chk )
        {
        return(0);                                // Good ARM image vector checksum
        }
    else
        {
        return(1);                                // Bad ARM image vector checksum
        }
}

// ***************************************************
// USB No longer used as takes up too much code
// #ifdef FU_DEBUG
//    #include "USBSerial.h"
//    USBSerial pc;         USB CDC serial port
// #endif
// ***************************************************
// Main Firmware Update routine
// ***************************************************

int fupdate ( char IPaddress[], char path_file[]  )
{ 
int        n=0;            // return value from socket.receive
uint8_t    connected =1;   // Socket open?
uint32_t   checksum =0;    // 32 bit checksum
uint32_t   csword=0;       // construct a 32 bit int from 4 bytes. 
uint32_t   byteCount =0;   // number of bytes written to serial EEPROM
uint32_t   size=0;         // Amount of data received
uint32_t   i=0;            // For loop counter
int        headerLen = 0;  // Length of HTTP Header terminated by 2x CR,lF
uint32_t   HTTPLength;     // Length of file, extracted from HTTP response
bool       openFlash = 1;  // Set up flash and do first write
#ifdef FU_DEBUG
int        HTTPCode = 0;   // Decode of HTTP response code - debug only
#endif
TCPSocketConnection socket;

// ***************************************************
// HTTP GET based file loader
//****************************************************

    sprintf(DebugTextBuffer,"HTTP GET Firmware update\r\n"); REPORT;
    byteCount =0;   
    HTTPLength = 0; //64K max
    checksum =0;
    csword=0;
    wd.Service();  
    // ************************************************
    // Erase Flash before writing
    ExtFLASHBlockErase(0x10000);
    ExtFLASHBlockErase(0x18000);
    
    do
    {
    
        if (socket.connect(IPaddress, 80) < 0) 
        {
            #ifdef FU_DEBUG
            sprintf(DebugTextBuffer,"\r\nUnable to connect to %s server\r\n", IPaddress); REPORT;
            #endif
            #ifdef serialdebug
            sprintf(DebugTextBuffer,"Unable to connect to %s server\r\n", IPaddress); REPORT;   
            #endif
            connected = 0;
        }   
      
        if (connected == 1)
        {
            #ifdef FU_DEBUG
            sprintf(DebugTextBuffer,"Socket Connected"); REPORT;
            #endif  
            wd.Service();  
            socket.set_blocking( false , FU_TCP_TIMEOUT );           // Block for 30 seconds
            headerLen =0;                                            // Need to find start of file - at end of HTTP response
            size=0;                                                  // keeps tack of data received
    
            // *************************************
            // Set up the HTTP GET strings
            // 
            sprintf (WSmessageBuf, "GET %s HTTP/1.0\r\n", path_file); // sprintf will write a trailing zero
            n = socket.send_all(WSmessageBuf, strlen(WSmessageBuf));  
            #ifdef FU_DEBUG
            sprintf(DebugTextBuffer,"\r\n%s",WSmessageBuf); REPORT;
            #endif        
            sprintf (WSmessageBuf, "Host:%s\r\n", IPaddress);         // sprintf will write a trailing zero
            n = socket.send_all(WSmessageBuf, strlen(WSmessageBuf)); 
            #ifdef FU_DEBUG
            sprintf(DebugTextBuffer,"%s",WSmessageBuf); REPORT;
            #endif 
            if ((HTTPLength > (byteCount+HTTP_RANGE )) || (HTTPLength ==0))
                sprintf (WSmessageBuf, "Range: bytes=%d-%d\r\n",byteCount, (byteCount+HTTP_RANGE));
            else
                sprintf (WSmessageBuf, "Range: bytes=%d-%d\r\n",byteCount, (HTTPLength -1) );    
            HTTPLength = 0;                                           // Need to clear the Length to build it correctly     
            n = socket.send_all(WSmessageBuf, strlen(WSmessageBuf));
            #ifdef FU_DEBUG
            sprintf(DebugTextBuffer,"%s",WSmessageBuf); REPORT;
            #endif 
            n = socket.send_all("\r\n",2);                            // End with CR LF
                   
            if ( n <= 0 ) 
            {  
                #ifdef FU_DEBUG
                sprintf(DebugTextBuffer,"Failed to send\r\n"); REPORT;
                #endif
            }     
            else 
            { 
                do 
                {
                    wd.Service();  
                    //******* FETCH THE DATA FROM THE SOCKET *******
                    n = socket.receive(WSmessageBuf, FU_BUFFER_SIZE);    
                    wd.Service();  
                    if (n >=0) 
                    {
                        WSmessageBuf[n] =0;                            // Terminate the string to be safe
                        i=0;
                        if (headerLen == 0)                            // Searching for end of HTTP response
                        {
                            #ifdef FU_DEBUG
                            // This printe the Header Data received from the server if uncommented
                            // sprintf(DebugTextBuffer,"\r\n**%s**\r\n",WSmessageBuf); REPORT;
                            HTTPCode =  ((WSmessageBuf[9] -48)*100) + 
                                        ((WSmessageBuf[10] -48)*10) + 
                                         ((WSmessageBuf[11] -48));
                            if (size <FU_BUFFER_SIZE){sprintf(DebugTextBuffer,"HTTP code is %d\r\n",HTTPCode);  REPORT; }          
                            #endif 
                            for (i=0; i<=n; i++)    // Get the langth of the file (appended to the Content-Range: response)
                            {                        
                                if ((WSmessageBuf[i]   == '-') && (WSmessageBuf[i+1] == 'R')&&
                                    (WSmessageBuf[i+2] == 'a') && (WSmessageBuf[i+3] == 'n')&&
                                    (WSmessageBuf[i+4] == 'g') && (WSmessageBuf[i+5] == 'e')&&
                                    (WSmessageBuf[i+6] == ':')                             
                                   )
                                {
                                    i=i+7;                             
                                    do 
                                    {
                                        i++;
                                        if (WSmessageBuf[i] == '/')  // length value is after the /
                                        {
                                           i++;
                                           while (WSmessageBuf[i] !='\r')
                                           {
                                                HTTPLength = (HTTPLength *10)+ (WSmessageBuf[i]-48);
                                                i++;
                                           } 
                                        }   
                                    } while (WSmessageBuf[i] !='\r'); 
                                    #ifdef FU_DEBUG
                                    sprintf(DebugTextBuffer,"HTTP Length is %d\r\n",HTTPLength);  REPORT;
                                    #endif  
                                }
    
                                if ((WSmessageBuf[i] == '\r') && (WSmessageBuf[i+1] == '\n')&&
                                    (WSmessageBuf[i+2] == '\r') && (WSmessageBuf[i+3] == '\n'))
                                {   // End of HTTP reponse seen
                                    i=i+4;          // i is now pointing at start of received file data
                                    //byteCount =0; removed due to multi pass Range GET
                                    headerLen = i + size; 
                                    openFlash = 1;
                                    break;          // from for loop
                                }
                            }
                            //here after break 
                        } 
                        // ********************************************************
                        // Processed the Header , now process the binary file data
                        //
                        if (headerLen >0)  
                        {   // File data being received    
                            while (i<n)
                            {   // calculate the running 32 bit checksum, dealing with a byte at a time
                                if ((byteCount%4) == 0) csword = ((WSmessageBuf[i]<<0  )&0x000000FF); 
                                if ((byteCount%4) == 1) csword = csword + ((WSmessageBuf[i]<<8  )&0x0000FF00); 
                                if ((byteCount%4) == 2) csword = csword + ((WSmessageBuf[i]<<16 )&0x00FF0000); 
                                if ((byteCount%4) == 3)
                                { 
                                    csword = csword + ((WSmessageBuf[i]<<24 )&0xFF000000);
                                    checksum = checksum + csword;
                                    // expect to see checksum ==0  twice, once at byte 31 where Vectors are checked in the code
                                    // and secondly at the EOF. See binmod.py python firmware update generator
                                    #ifdef FU_DEBUG
                                    if (checksum == 0) 
                                    { sprintf(DebugTextBuffer,"Checksum seen equal to zero at filesize %d\r\n",byteCount); REPORT;
                                    }   
                                    #endif                
                                } 
                                //**********************************************************************
                                // WRITE TO Serial FLASH devices
                                if (openFlash == 1) ExtFLASHOpen ( (FU_START_ADDRESS + byteCount), WSmessageBuf[i]);
                                else                ExtFLASHIncWrite(WSmessageBuf[i]);
                                //**********************************************************************
                                i++;            // Pointer into tmp Buffer
                                byteCount++;    // Pointer into Serial EPROM 
                    
                            } //end of while (i<n)
                        }  //end of if (headerlen == 1)     
                        size=size+n;  
                    }
                }    while (n >=0 ) ;              
                // Leave if n is -ve
                WSmessageBuf[0] = '\0';
                #ifdef FU_DEBUG
                sprintf(DebugTextBuffer,"GET processed: Size =%d byteCount=%d HeaderLen=%d, checksum=%x\r\n", 
                size-headerLen, byteCount, headerLen,checksum); REPORT;  
                #endif            
                if (checksum != 0) {n=-2; }
                else {n= byteCount;}
                socket.close();
                openFlash = 0;
                ExtFLASHClose();
                #ifdef FU_DEBUG
                sprintf(DebugTextBuffer,"Closed Socket and Write protect Serial Flash\r\n"); REPORT;
                #endif    
            }
            
        } else
        {
            n = -1; // Failed to connect;             
        }
        sprintf(DebugTextBuffer,"HTTP GET Range to %d\r\n",byteCount); REPORT;
    } while (byteCount < HTTPLength) ;     
    #ifdef FU_DEBUG
    sprintf(DebugTextBuffer,"Finished Multi-Pass GET return(%d)\r\n",n); REPORT;
    #endif  
    return (n);// -1 Failed to connect; -2 checksum !=0; else return Filesize in bytes 
}    

