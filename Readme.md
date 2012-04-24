================================================================================
ReadMe for Stellaris ATDT Cloud<br>
================================================================================
ABOUT: Explains the contents in the stellaris_atdt_cloud project<br>
AUTHOR: Exosite<br>

License of all cloud-specific components is BSD, Copyright 2012, Exosite LLC 
(see LICENSE file)<br>


General Overview:
--------------------------------------------------------------------------------
This project demonstrates how to use a Stellaris Development Kit to send and 
receive data to/from the cloud via Exosite's Cloud Data Platform.  The 
communication with the cloud is accomplished over HTTP with the Janus-RC GSM864Q 
GPRS modem. The project sends AT commands over a UART to the Janus modem.  The 
project functionality writes a 'Ping' value to the cloud and reads a value 
'interval' from the cloud.  Cloud information is shown on the on-board LCD 
screen. For some development kits with enough screen size, an option to read a 
public temperature data source is also available to display on the screen.<br>

The source code in this folder is a project called "Stellaris ATDT Cloud" which 
has been verified with Code Composer Studio 4 and IAR for the following TI 
Stellaris Development Kits:<br>

* EK-LM3S6965 (with addition of RS232 Transceiver)<br>
* EK-LM3S1968 (with addition of RS232 Transceiver)<br>
* RDK-IDM-L35<br>

Note: This code can be used with any of the Stellaris Development kits that have 
a UART accessible.  Some modification may be required depending on the UART used and
the display/LCD available.  Also, some kits have RS232 signal levels on UARTs 
(like the RDK-IDM-L35) and others are only TTL.  For TTL UART signals, a RS232 
transceiver will need to be connected to properly interface with the cellular modem
which accepts a RS232 serial connection.<br>


Project Notes:
--------------------------------------------------------------------------------
* The project is based on the Stellaris libraries in the Stellarisware package for 
  the relevant Stellaris processor (see Development Guide)<br>

* PART_LM3S6965<br>
  --) uses UART0 for GRPS comms<br>
  --) uses the OLED display for messages<br>

* PART_LM3S1968<br>
  --) uses UART2 for GRPS comms<br>
  --) uses the OLED display for messages<br>

* RDK-IDM-L35<br>
  --) uses UART2 for GRPS comms<br>
  --) uses the QVGA LCD display for messages<br>
  --) can show public temp if #define PUBLICTEMP 1 is undefined in main.c. and 
     'pubtemp' data source is available.<br>
  --) The application starting location is set to 0x00000800 because these RDK 
      kits come with a serial boot loader loaded at 0x00000000.  If using the 
      .bin file and LM Flash utility make sure to program at this location.<br>


CCS4 Quick Start:
--------------------------------------------------------------------------------
1) Sign-up for an Exosite Portals account (portals.exosite.com).<br>
   -) Create a device (need CIK)<br>
   -) Add data source with Alias 'ping', format 'integer'<br>
   -) Add data source with Alias 'interval', format 'integer'<br>
   -) Write a value to the 'interval' data source (like '60').<br>
   -) Add a public data source using a public temperature data source with an RID,
      set Alias to 'pubtemp'.<br>

2) Install TI CCS4 & StellarisWare for a supported processor (see Development Guide)<br>
   * NOTE: Installed CCS4 to C:\CCS4 (ran installer as Admin)<br>
   * NOTE: Installed StellarisWare to C:\StellarisWare (ran installer as Admin)<br>
   
3) Open a workspace at C:\StellarisWare\boards\ek-lm3s6965 (or \em-lm3s1968\ or 
   \rdk-idm-l35) and copy the folder "stellaris_atdt_cloud" and all sub-folders & files 
   into that location<br>

4) Set the Active Build Configuration (Debug or Release version for LM3S6965, LM3S1968, 
   or RDK-IDM-L35) from Project -> Active Build Configuration<br>

4) Edit the CIK value and APN information in exosite_gprs.h.  Get your APN from your 
   cellular provider.<br>

5) Compile and download<br>



Development Guide:
--------------------------------------------------------------------------------
1) Read the "Future Improvments" items below - these are items that are need to be 
   fixed before the code base is production-ready<br>

2) When compiling for a different uP version or board, make modifications to 
   exosite_utils.c.  Note the usage of #ifdefs for PART_LM3S6965 for example.<br>

3) Board-specific development software tools:<br>

   * Development for EK-LM3S6965 (which is defined as PART_LM3S6965) is accomplished by 
     downloading and installing the software from 
     http://www.ti.com/tool/ekc-lm3s6965-cd  (Version 855, release date 2/01/2012)<br>

   * Development for EK-LM3S1968 (which is defined as PART_LM3S1968) (also supports 
     LM3S1958) is accomplished by downloading and installing the software from 
     http://www.ti.com/tool/sw-ek-lm3s1968 (Version 855, release date 2/01/2012)<br>

   * Development for RDK-IDM-L35 (which is defined by PART_RDK-IDM-L35 and PART_LM3S1958) 
     is accomplished by downloading and installing the software from 
     http://www.ti.com/tool/sw-rdk-idm-l35 (Version 855, release date 2/01/2012)<br>

4) NOTE that the environment variable "SW_ROOT" must be defined.  If it is not, for 
   some reason (you will get build errors about include option missing dir), add 
   a user environment variable "SW_ROOT" as "C:\Stellarisware" under Window -> 
   Preferences -> C/C++->Managed Build<br>

5) NOTE that you must first build a Release version of the 
   $\StellarisWare\driverlib\ccs-cm3 to generate library files for UART, etc... before
   release versions of the project can be build.  Stellarisware default install only
   includes precompiled Debug versions of the dependencies.<br>

6) NOTE that the Janus Terminus Modem _must_ have RTS and CTS tied together on the RS232 
   side. The Stellaris UARTs used are not using flow control.  Buffer over-flows will 
   happen, inconsistent modem responses, and not data socket communications will work if 
   this is not done.<br>


Future Improvements:
--------------------------------------------------------------------------------
* Add record function<br>
* Add flash write function to store CIK<br>
* Add Provisioning<br>
* AT# SGACT turns off automatically<br>
  * The OFF condition cause is unknown<br>
  * Add Error retry<br>
* Modem usually needs 30 to 40 seconds to be ready to use after power-on, look 
  to see if code can detect this better and not try to configure socket until 
  it's ready.<br>
* Add support for Serial and Ethernet bootloaders for RDK kits.<br>
* Not totally satisfied with the stability of CloseSocket... may need to make 
  more robust<br>
* Not totally satisfied with failure retries, although hard failures do 
  eventually catch themselves<br>
* Fix up GPRS state machine for GPRS_STATE_AT_SGACT_GET & GPRS_STATE_AT_SGACT_SET 
  (backwards retry logic)<br>


================================================================================
Release Info
================================================================================
Release 2012-04-23
--------------------------------------------------------------------------------
* Initial release<br>

