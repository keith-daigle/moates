/*
 * Copyright (c) 2012, Keith Daigle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Class for Moates Ostrich
 *
 * Will encapsulate a ostrich and allow easy access to hw
 * For writing, verifying, and changing banks of emulation
 * will handle triggered tracing, windowed tracing, and full on tracing
 * Will also allow traced data to be dumped to file
 * Streaming traces are handled by passing FD of com port to application
 * after kicking off trace, or just tracing to file
 * Other tracing is handled by getdatablock
 *
 *
 *
 *
 *
 *
 *
 */
#include <string>
#include <iostream>
#include <fstream>
#include "Serial.h"

class Ostrich
{


public:
   //Bank number for full device presentation
   static const int wholeEnchilada = 8;

   //number of bytes in serial number
   static const int serialNumberLen = 8;

   //Calcuates a running checksum, returning it each time
   bool updateChecksum(char);

   //resets checksum so that it starts fresh upon next call
   bool resetChecksum(void);

   //returns current checksum
   char getChecksum(void);

   //resets the index into the bin array for reading/writing blocks of data
   bool resetBinIdx(void);

   //Verify bin on chip against the file specified by binFile
   bool verifyBankToFile(void);

   //reads a bin from the chip to the filename specified by binFile
   bool readBankToMemory(void);

   //function to write memory buffer to file
   bool writeMemoryToFile(void);

   //function to load the data from the memory buffer to disk
   bool readFileToMemory(void);

   //function to write file to chip and verify it
   //This does the erase, verify of erase, burn and verify in one shot
   bool writeFileToBank(void);

   //Write to a bank
   bool writeMemoryToBank(void);

   //This will force the device to set all it's banks to the same setting
   //used primarily when going from whole device to banked mode or from banked to whole device
   bool forceAllBanks(void);

   //This sets the bank on the device, uses the char argument to decide
   //'E' for emulation bank, 'P' for persistent bank, 'U' for read/write bank
   bool setBank(int, char);

   //This returns the bank on the device, verifies against hardware each time
   //this also uses the char argument to decide which bank to operate on
   //'E' for emulation bank, 'P' for persistent bank, 'U' for read/write bank
   int getBank(char);

   //Calcuate offset of binary into bank, or whole address space if bank == wholeEnchilada
   bool calculateOffset(void);

   //returns currently calculated offset, will handle both banks and
   //full device depending on current configuration
   int getOffset(void);

   //sets the current chip type
   bool setBinFile(std::string);

   //gets current chip type
   std::string getBinFile(void);

   //sets filename for trace output if enabled
   bool setTraceFile(std::string);

   //gets filename that trace will be written to
   std::string getTraceFile(void);

   //gets whether or not the tracing to file is enabled
   bool getWriteTraceToFile(void);

   //sets whether or not the tracing to file is enabled
   bool setWriteTraceToFile(bool);

   //sets the current com port
   bool setComPort(std::string);

   //gets current com port
   std::string getComPort(void);

   //Sends commands to device
   bool sendCommands(void);

   //return block size for reads/writes to chip
   int getBlockSize(void);

   //set size for reads/writes to chip, must clamp to maxHWBlockSize
   bool setBlockSize(int);

   //if default block size is used these functions are almost useless
   //used internally to get the size of the block requested from hardware
   //used to handle case where non power of 2 number of bytes requested
   int getLastBlockSize(void);

   //used internally to tell functions that last block of data read/written
   //was of a size other than blockSize, for non ^2 applictions
   bool setLastBlockSize(int);

   //reads a block of data from the serial port, size will match blockSize
   // size will match blockSize and data will be from bin[]
   // starting at previously set binIndex
   bool getDataBlock(void);

   //reads a block of trace data into the internal buffer
   //size is based upon the currently configured address length
   //addresses per packet and packets per trace
   bool getTraceBlock(void);

   //sends a block of data out the serial port
   // size will match blockSize and data will from bin[]
   // starting at previously set binIndex
   bool sendDataBlock(void);

   //builds the command
   bool buildCommand(int, int, int);

   //This checks for a device on the currently configured com port
   //Also tries to suck back the serial number and vendor ID
   bool checkForDevice(void);

   //These will return the bytes from the VV command used to
   //check for the device and it's version information
   char getHardwareVersion(void);
   char getFirmwareVersion(void);
   char getHardwareVersionCH(void);

   //no set s/n or vendor commands since I don't have hardware to test them out on
   //and don't want to thrash anyone's hardware
   //same with Serial number and vendor ID, must call checkForDevice or
   //getSerialNumAndVendorFromHW to populate these
   bool getSerialNumber(char *);
   int getSerialNumberLen(void);
   char getVendorID(void);

   //Function to try and retireve the serial number and hardware vendor
   bool getSerialNumAndVendorFromHW(void);

   //This returns true if the trace is set to be windowed
   //Will disable triggered tracing
   bool getTraceWindowed(void);

   //This sets the trace to be windowed
   bool setTraceWindowed(bool);

   //This returns true if the trace is set to be triggered
   //uses start address  to begin tracing end address to stop
   bool getTraceTriggered(void);

   //This turns on triggered tracing, will disable windowed tracing
   //uses start address to begin tracing end address when hit to stop
   //This also has an effect on the windowed and redundant settings
   //will be set to the proper settings when this is set to true, but are
   //not touched if triggered is transitioned from true -> false
   bool setTraceTriggered(bool);

   //This returns true if the trace will include consecutive hits of the same address
   bool getTraceNonRedundant(void);

   //If tre multiple, redundant hits of the same address are *NOT* included in the trace output
   bool setTraceNonRedundant(bool);

   //This returns true if the trace is set to return relative addresses
   bool getTraceRelativeAddress(void);

   //This will enable tracing to return relative addresses, be careful of number of bytes in
   //Each address, and size of window if windowed
   bool setTraceRelativeAddress(bool);

   //This will return the current start address for triggered or windowed tracing
   int getTraceStartAddress(void);

   //This sets the start address either for triggered or windowed tracing and relative addresses
   //Should also consider using setTraceStartAddress(getOffset()) when tracing in relative
   //mode so traces will more easily index into bin
   bool setTraceStartAddress(int);

   //This will return the current start address for triggered or windowed tracing
   int getTraceEndAddress(void);

   //This sets the start address either for triggered or windowed tracing
   //If set larger than maxBinSize this will disable the end address for triggered
   //tracing, in any other mode it's an error
   bool setTraceEndAddress(int);

   //This will return the number of bytes in addresses provided by trace
   char getTraceAddressBytes(void);

   //This sets the number of bytes in addresses provided by trace
   bool setTraceAddressBytes(char);

   //This sets the number of addresses returned in a single packet of the trace
   bool setTraceAddrPerPacket(int);

   //This will return the current number of addresses returned in a single packet of the trace
   int getTraceAddrPerPacket(void);

   //This sets the number of packets in trace until another trace must be requested, non streaming only
   bool setPacketsPerTrace(char);

   //This will return the current number of packets in trace until another trace must be requested, non streaming only
   char getPacketsPerTrace(void);

   //This sets the buffer for the trace output
   //To be handled by application, if unset only hit map is updated
   bool setTraceBuf(int *, int);

   //This will return traced data to the buffer set by setTraceBuf
   //if file tracing is on and the file has been opened
   //this will also dump the addresses to the file
   bool getTraceToBuf(void);

   //This will attempt to get the trace data back to the application and
   //update the map , if file tracing is on and the file has been opened
   //this will also dump the addresses to the file
   bool getTraceToBufMap(void);

   //This will read the trace data to an internal buffer and update the map
   //If file tracing is on and the file has been opened, this will also dump the
   //addresses to the file
   bool getTraceToMap(void);

   //This will read the trace data to a file only, will attempt to open file on its
   //own, unlike other 3 functions
   bool getTraceToFile(void);

   //This will open the trace file so it can be used for tracing
   //Will set the trace flag to true
   bool openTraceFile(void);

   //This will close the trace file  and unset the trace flag
   bool closeTraceFile(void);

   //This will copy the whole trace map back to a buffer provided by the application
   //Could be costly if taking whole 512K back and forth multiple times
   bool getHitMap(char *);

   //This will copy a section of the trace map back, starting at 1st arg, running to 2nd arg
   bool getHitMap(char *, int, int);

   //This will copy a section of the trace map back, starting at 0, and including a number of addresses
   //Good for relative address tracing
   bool getHitMap(char *, int);

   //This will tell if a address was hit by looking it up in the map
   bool wasHit(int);

   //This will reset the trace map so that all addreses are un-hit
   bool resetHitMap(void);

   //This will reset the index into the hit buffer for making ints out of
   //returned addresses
   bool resetHitIdx(void);

   //This will return an integer representing the next address in the packet
   //that was hit
   int getNextAddressHit(void);


   //Constructor
   Ostrich();
   //Destructor
   ~Ostrich();

private:
   //Largest bin we handle matches full address space of Ostrich
   static const int maxBinSize = 524288;

   //Number of banks in the Ostrich
   static const int banks = 8;

   //how many types of bank setting there are
   static const int bankTypes = 3;

   //maximum possible length of a command string includes 1 byte for EOF/checksum
   static const int maxCommandLen = 14;

   //maximum possible size of block the hardware will accept
   static const int maxHWBlockSize = 256;

   //Size of block for bulk reads/writes
   static const int bulkBlockSize = 256;

   //maximum bulk block size
   static const int maxBulkBlockSize = bulkBlockSize * 256;

   //how many types of bank setting there are
   static const int maxAddressBytes = 3;

   //how many addresses can be stored in a trace packet
   static const int maxAddressesPerPacket = 255;

   //how many trace packets can be received in a single command
   static const int maxPacketsPerTrace = 255;

   //the size of the hit buffer in bytes
   //must be delcared below consts it uses
   static const int hitBufferMaxSize = ((maxAddressBytes * maxAddressesPerPacket) + 2) * maxPacketsPerTrace;

   //character to read back version of attached device
   static const int ostrichHardwareByte = 0x0A;
   static const int ostrichTwoHardwareByte = 0x14;
   static const int ostrichHardwareCH = 'O';

   //character that device sends to OK data reception
   static const int dataOK = 'O';

   //index in command string for writes
   static const int writeIdx = 0;

   //index in command string for bulk writes
   static const int writeIdxBulk = 1;

   //character to send to start writing data to device
   static const int bankCommand = 'B';

   //character to send to start writing data to device
   static const int traceCommand = 'T';

   //character to send to start writing data to device
   static const int writeCommand = 'W';

   //character to send to start reading data from device
   static const int readCommand = 'R';

   //character to read back version of attached device
   static const int versionCommand = 'V';

   //character to change serial speed divisor on device
   static const int speedCommand = 'S';

   //character to read back serial number and vendor ID
   static const int serialNumCommand = 'N';

   //Trace bitmask definitions
   static const unsigned char streamingTrace = 0x80;
   static const unsigned char windowedTrace = 0x40;
   static const unsigned char nonRedundantTrace= 0x20;
   static const unsigned char triggerStartAddress = 0x10;
   static const unsigned char triggerEndAddress = 0x08;
   static const unsigned char relativeAddress = 0x04;
   static const unsigned char twoByteAddress = 0x02;
   static const unsigned char oneByteAddress = 0x01;

   //Trace related variables
   bool traceToFile;
   bool traceIsWindowed;
   bool traceIsTriggered;
   bool traceNonRedundant;
   bool traceRelativeAddress;
   int traceStartAddress;
   int traceEndAddress;
   char traceAddressBytes;
   char addressesPerPacket;
   char packetsPerTrace;

   //Banks for ostrich internals
   int emuBank;
   int updateBank;
   int persistentBank;
   int currentBankSize;

   //Initial device checking variables
   bool foundDevice;
   char hardwareVersion;
   char firmwareVersion;
   char hardwareVersionCH;
   char vendorID;
   char serialNumber[serialNumberLen];

   char checksum;
   bool checksumFirstByte;

   //Files and strings
   std::fstream file;
   std::fstream trace;
   std::string binFileName;
   std::string traceFile;
   std::string comPort;

   Serial serial;
   int offset;
   //Blocksize will be used to determine if bulk command should be used
   //or normal commands
   int blockSize;
   int lastBlockSize;
   int binIdx;
   char bin[maxBinSize];
   int command[maxCommandLen];

   //The hitMap could be implemented as bitmap if space starts to really be
   //a problem, currently 1 char per address
   char hitMap[maxBinSize];

   // size the buffer for the readback of traces so it handles largest possible read and 2 oks
   char hitBuffer[hitBufferMaxSize];
   //index for reading addresses back out of hit buffer
   int hitIdx;

   //Information on buffer if application wishes to have traces copied back to it
   //End of trace is marked by EOF in array if ended early, addresses are reconstructed
   //to integers
   int * extTraceBuffer;
   int  extTraceBufferSize;
};
