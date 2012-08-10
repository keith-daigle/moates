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
 * Class for Moates burn1/2
 * Will do erasing of chips and verification
 * Against buffer, automatic calcuation of offsets
 * based upon size of buffer
 *
 *
 * This is the header file for encapsulation of a Burn1/2 EEPROM burner
 * based upon the HW interfacing guide available on the moates.net website
 *
 * the following improvements could be used in current implementation:
 *
 * 1) At this point the EECIV stuff isn't supported very well so that
 * part of the code can use some beefing up
 *
 * 2) most of the functions should be collapsed to use the bank functions
 * if a whole chip is being written/erased  for a AM29 chip
 *
 * 3) Better support for banking where available, with offsets into banks being calc'd
 *
 * 4) Additional error checking in all read/write functions to make sure port is open
 * and device has been found
 *
 * Otherwise, the implementation is pretty full, everything returns a bool
 * for future error checking or expansion, typedefs were attemtped to be used
 * in sane way to make things easier
 *
 *
 * Only real dependence is on the sleep function, most other platform specific details
 * are left to the serial class if you #define WIN32, you'll get the windows version
 *
 */
#include <config.h>
#include <string>
#include <iostream>
#include <fstream>
#include "Serial.h"

//This matches 1st command byte for burn1
//So it knows which type of chip to read/write to
enum ChipType
{
   NONE = 0,
   AT29C256 = '2',
   M2732A = '3',
   AM29F040 = '4',
   SST27SF512 = '5',
   EECIV = 'J'
};

enum ChipSize
{
   NONE_SIZE = 0,
   AT29C256_SIZE = 0x8000,
   M2732A_SIZE = 0x1000,
   AM29F040_SIZE = 0x80000,
   SST27SF512_SIZE = 0x10000,
   EECIV_SIZE = 0x80000
};

class Burn
{


public:
   //Calcuates a running checksum, returning it each time
   bool updateChecksum(char);

   //resets checksum so that it starts fresh upon next call
   bool resetChecksum(void);

   //returns current checksum
   char getChecksum(void);

   //resets the index into the bin array for reading/writing blocks of data
   bool resetBinIdx(void);

   //Erase the chip
   bool eraseChip(void);

   //Verify bin on chip is blank
   bool verifyChipIsBlank(void);

   //Verify bin on chip against the file specified by binFile
   bool verifyChipToFile(void);

   //Writes a bin to the selected chip from filename specified by binFile
   bool writeMemoryToChip(void);

   //reads a bin from the chip to the filename specified by binFile
   bool readChipToMemory(void);

   //function to write memory buffer to file
   bool writeMemoryToFile(void);

   //function to load the data from the memory buffer to disk
   bool readFileToMemory(void);

   //function to write file to chip and verify it
   //This does the erase, verify of erase, burn and verify in one shot
   bool writeFileToChip(void);

   //Erase bank, only good for 29f040 chips, will return error if other chips selected
   bool eraseBank(int);

   //Verify bank is blank on 29f040 or eeciv
   bool verifyBankIsBlank(void);

   //Write to a bank on 29f040 or eeciv
   bool writeMemoryToBank(void);

   //Verify bin on chip against the file specified by binFile
   //Need to calculate offset into bank
   bool verifyBankToFile(int);

   //sent bank for 29F040 or eeciv adapter
   bool setBank(unsigned int);

   //get bank if working with 29F040 or eeciv adapter
   unsigned int getBank(void);

   //Calcuate offset of binary on chip, based upon file size
   bool calculateChipOffset(void);

   //returns currently calculated offset
   int getOffset(void);

   //Calcuate offset of binary into bank, based upon file size
   bool calculateBankOffset(void);

   //sets the current chip type
   bool setChipType(ChipType);

   //gets current chip type
   ChipType getChipType(void);

   //sets the current chip type
   bool setBinFile(std::string);

   //gets current chip type
   std::string getBinFile(void);

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

   //reads a block of data from the serial port
   // size will match blockSize and data will be in bin[]
   // starting at previously set binIndex
   bool sendDataBlock(void);

   //builds the command
   bool buildCommand(char, unsigned char *, int);

   //This checks for a device on the currently configured com port
   bool checkForDevice(void);

   //These will return the bytes from the VV command used to
   //check for the device and it's version information
   //There's no set, they're only set in the checkForDevice function
   char getHardwareVersion(void);
   char getFirmwareVersion(void);
   char getHardwareVersionCH(void);



   //Constructor
   Burn(void);

private:
   //Largest bin we handle, this has an effect on bank
   //calculations used in reads/writes
   static const unsigned int maxBinSize = 524288;

   //Number of banks on a 29f040
   static const unsigned int banks = 8;

   //maximum possible length of a command string includes 1 byte for EOF/checksum
   static const unsigned int maxCommandLen = 8;

   //maximum possible size of block the hardware will accept
   static const unsigned int maxHWBlockSize = 256;

   //index for the 'R' or the 'W' command to be sent
   static const unsigned int readWriteIdx = 1;

   //read/write command
   static const int writeCommand = 'W';

   //character to read back version of attached device
   static const int versionCommand = 'V';

   //character to read back version of attached device
   static const int burnHardwareByte = 0x05;

   //character that device sends to OK data reception
   static const int dataOK = 'O';

   ChipType romType;
   ChipSize romSize;
   char hardwareVersion;
   char firmwareVersion;
   char hardwareVersionCH;
   unsigned char checksum;
   bool checksumFirstByte;
   bool foundDevice;
   std::fstream file;
   std::string binFile;
   std::string comPort;
   Serial serial;
   int sizeOfBin;
   int offsetOnChip;
   int blockSize;
   int lastBlockSize;
   int binIdx;
   char bin[maxBinSize];
   int command[maxCommandLen];
};
