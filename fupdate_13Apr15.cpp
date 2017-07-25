//****************************************************************************************************
// HTTP Get for iAqua firmware update
// Giles Hutchison
// April 2015
//
// The firmware update file format that this process expects to be download is defined in the readme.txt file
// in F:\Development\iAqua\Software\bootloader\python_mbed_builder

// Enable the following define to turn on USB CDC Comms Debu
#define FU_DEBUG 1

#define FU_START_ADDRESS 0x10000
#define FU_BUFFER_SIZE  384
#define FU_TCP_TIMEOUT  6000 // mili-seconds
 

#include "mbed.h"
#include "cc3000.h"
#include "wifi.h"
#include "UDPSocket.h"
#include "main.h"
#include "SerialDebug.h"
#include "cc3000_WiFi.h"
#include "wdog.h"
#include "Websocket.h"
#include "WSmessage.h"
#include "ACTiFi.h"
#include "SST25VF010.h"     
#include "OTAFU.h"          // Firmware update & image handling
#include "EEPROM.h"

#ifdef FU_DEBUG
    #include "USBSerial.h"
    USBSerial pc;               // USB CDC serial port
#endif

extern char WSmessageBuf[];

extern Watchdog wd;

int fupdate ( char IPaddress[], char path_file[]  )
{ 
int        n=0;            // return value from socket.receive
//uint8_t    local = 1;      // Chose Pi or actifi.co.uk to download 
uint8_t    connected =1;   // Socket open?
uint32_t   checksum =0;    // 32 bit checksum
uint32_t   csword=0;       // construct a 32 bit int from 4 bytes. 
uint32_t   byteCount =0;   // number of bytes written to serial EEPROM
uint32_t   size=0;         // Amount of data received
uint32_t   i=0;            // For loop counter
int        headerLen = 0;  // Length of HTTP Header terminated by 2x CR,lF
int        HTTPCode = 0;
//char*      messageBuf;
//messageBuf = (char*)malloc(FU_BUFFER_SIZE+1);
TCPSocketConnection socket;


//    if (messageBuf ==NULL) { return (-3); }

#ifdef FU_DEBUG
    pc.printf("HTTP GET Test\r\n");
#endif    
    wd.Service();  
    if (socket.connect(IPaddress, 80) < 0) 
    {
#ifdef FU_DEBUG
        pc.printf("\r\nUnable to connect to %s server\r\n", IPaddress);
#endif
        connected = 0;
    }   
  
    if (connected == 1)
    {
#ifdef FU_DEBUG
        pc.printf("\r\nConnected\r\n");
#endif        
        // *****************************
        // Erase Flash before writing
// ** ExtFLASHBlockErase(0x10000);
// ** ExtFLASHBlockErase(0x18000);
        wd.Service();  
        socket.set_blocking( false , FU_TCP_TIMEOUT );        // Block for 30 seconds
        headerLen =0;             // Need to find start of file - at end of HTTP response

        size=0;      // Totoal amount of data received
        // File to download
        sprintf (WSmessageBuf, "GET %s HTTP/1.0\r\n", path_file); // sprintf will write a trailing zero
        n = socket.send_all(WSmessageBuf, strlen(WSmessageBuf));  
#ifdef FU_DEBUG
        pc.printf("\r\n%s",WSmessageBuf);
#endif        
        sprintf (WSmessageBuf, "Host:%s\r\n", IPaddress);         // sprintf will write a trailing zero
        n = socket.send_all(WSmessageBuf, strlen(WSmessageBuf)); 
#ifdef FU_DEBUG
        pc.printf("%s\r\n",WSmessageBuf);
#endif 
        n = socket.send_all("\r\n",2);   
               
        if ( n <= 0 ) 
        {  
#ifdef FU_DEBUG
             pc.printf("Failed to send\r\n");
#endif
        }     
        else 
        { 
            do 
            {
                wd.Service(); 
                     
                n = socket.receive_all(WSmessageBuf, FU_BUFFER_SIZE);  

                wd.Service();  
                if (n >=0) 
                {
                    WSmessageBuf[n] =0;         // Terminate the string to be safe
                    i=0;
                    if (headerLen == 0)         // Searching for end of HTTP response
                    {
#ifdef FU_DEBUG
    pc.printf("\r\n**%s**\r\n",WSmessageBuf);
    HTTPCode =  ((WSmessageBuf[9] -48)*100) + 
                ((WSmessageBuf[10] -48)*10) + 
                ((WSmessageBuf[11] -48));
    if (size < FU_BUFFER_SIZE){pc.printf("HTTP code is %d\r\n",HTTPCode);  }          
#endif 
                        for (i=0; i<=n; i++)    // Find the start of the binary file
                        {                        

                            if ((WSmessageBuf[i] == '\r') && (WSmessageBuf[i+1] == '\n')&&(WSmessageBuf[i+2] == '\r') && (WSmessageBuf[i+3] == '\n'))
                            {
#ifdef FU_DEBUG
                                pc.printf("End of HTTP reponse seen\r\n"); 
#endif
                                i=i+4;          // i is now pointing at start of received file data
                                byteCount =0;
                                headerLen = i + size; 
                                break;          // from for loop
                            }
                        }
                        //here after break 
                    } 
                    if (headerLen >0)  
                    {   // File data being received
                        do
                        {
                            if ((byteCount%4) == 0)
                            { 
                                csword = ((WSmessageBuf[i]<<0  )&0x000000FF); // don't do csword + on lowest byte as starting
                            } 
                            if ((byteCount%4) == 1)
                            { 
                                csword = csword + ((WSmessageBuf[i]<<8  )&0x0000FF00); 
                            } 
                            if ((byteCount%4) == 2)
                            { 
                                csword = csword + ((WSmessageBuf[i]<<16 )&0x00FF0000); 
                            }
                            if ((byteCount%4) == 3)
                            { 
                                csword = csword + ((WSmessageBuf[i]<<24 )&0xFF000000);
                                checksum = checksum + csword;
 #ifdef FU_DEBUG
                                if (checksum == 0) { pc.printf("Checksum seen equal to zero at filesize %d\r\n",byteCount);}   
 #endif                
                            } 
                            
                            
                            //**********************************************************************
                            // WRITE TO Serial FLASH devices
// **                            if (byteCount==0) ExtFLASHOpen ( FU_START_ADDRESS, WSmessageBuf[i]);
// **                            else              ExtFLASHIncWrite(WSmessageBuf[i]);
 #ifdef FU_DEBUG
     pc.printf("%c",WSmessageBuf[i]);
 #endif   

                            //**********************************************************************
                            i++;            // Pointer into tmp Buffer
                            byteCount++;    // Pointer into Serial EPROM 
                
                        } while (i<n);
                    }    
                    size=size+n;  
                }
            }    while (n >0 ) ;      // *** >= if using socket.receive instead of socket.receive_all   ***     
            // Leave if n is -ve
            WSmessageBuf[0] = '\0';
#ifdef FU_DEBUG
    pc.printf("TCP:Finished GET: Size =%d byteCount=%d HeaderLen=%d, checksum=%x\r\n", size-headerLen, byteCount, headerLen,checksum);  
#endif            
            if (checksum != 0) {n=-(size-headerLen); }
            else {n= byteCount;}
            socket.close();
            ExtFLASHClose();
#ifdef FU_DEBUG
    pc.printf("Closed Socket and Write protect Serial Flash\r\n");
#endif    
        }
    } else
    {
        n = -1; // Failed to connect; 
                
    }  
#ifdef FU_DEBUG
    pc.printf("return: (%d)\r\n",n);
#endif  
//    free (WSmessageBuf);
    return (n);// -1 Failed to connect; -2 checksum !=0; else return Filesize in bytes 
}    

