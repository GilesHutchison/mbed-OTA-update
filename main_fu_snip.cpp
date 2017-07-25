#include "fupdate.h"

#define FIRMW

// AT LINE 371
            WiFiGo=1; // Re-enable WiFi loop 
      
            while (WiFiGo)   // Container for WS set up and main loop
                {      
                wd.Service();

#ifdef FIRMW
                sprintf (DebugTextBuffer, "Firmware Update start\r\n"); REPORT;
            //  fu_return = fupdate ( "192.168.3.250","/firmware/fupdate.bin");
                fu_return = fupdate ( "www.actifi.co.uk","/firmware/fupdate.bin");
               if (fu_return >= 0) 
                { 
                    sprintf (DebugTextBuffer, "Firmware Update OK: size = %d\r\n",fu_return); REPORT; 
                }
                else 
                {
                    if (fu_return == -1)
                    {
                        sprintf (DebugTextBuffer, "Firmware Update Failed to Connect\r\n"); REPORT; 
                    }
                    else
                    {
                        if (fu_return == -2){sprintf (DebugTextBuffer, "Firmware Update Checksum Failed\r\n"); REPORT; }
                        else {sprintf (DebugTextBuffer, "Firmware Update Failed: Unknown reason\r\n"); REPORT; }
                    }
                }    
                        
                do 
                {
                    wd.Service();      
                    wait (5);
                } while (1);                              
#else  