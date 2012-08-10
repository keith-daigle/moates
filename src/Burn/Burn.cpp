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

/* Class for Moates burn1/2
 * Will do erasing of chips and verification
 * Against buffer, automatic calcuation of offsets
 * based upon size of buffer
 *
 *
 * This is the implementation file for encapsulation of a Burn1/2 EEPROM burner
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
 *
 *
 * Otherwise, the implementation is pretty full, everything returns a bool
 * for future error checking or expansion, typedefs were attemtped to be used
 * in sane way to make things easier
 *
 *
 * Only real dependence is on the sleep function, most other platform specific details
 * are left to the serial class if you #define WIN32, you'll get the windows version
 *
 * The Serial class used is highly dependent on the underlying OS/Hardware
 * I believe the abstraction is done well enough to hide most of that harshness, though
 *
 * You can see the moatesburn.cpp file for an example program, or the BurnDriver.cpp
 * for a test suite that can be run after making changes
 *
 *
 *
 *
 *TODO:
 *Change buildcommand to take unsigned int and split it with divides,instead of char pointer
 *use ifdef's for Sleep header inclusion
 *always check for port being opened before any serial transactions
 *add seprate eec functions based upon h/w guide
 *update constructor to match model in ostrich
 *check for the getLastBlockSize
 *check for bools where calls can be put into if() s
 *change getblock to write into the bin array directly and reset the index if data OK isn't good
 *change sendblock to write directly from bin and send checksum afterwards - zero copy
 *
 */

#include "Burn.h"
#include <iostream>

//This assumes that the serial port is already setup for 921.6k
//if no device is found
//Tries @ 115.2 and asks for the device to be bumped up

bool Burn::checkForDevice(void)
{
   char tmp = 0 ;

   if(!serial.isOpen())
      return false;

   command[0] = versionCommand;
   command[1] = versionCommand;
   command[2] = EOF;

   serial.purgeRX();

   if(!sendCommands())
      return false;

   if(	serial.getByte(&hardwareVersion) &&
         serial.getByte(&firmwareVersion) &&
         serial.getByte(&hardwareVersionCH) )
   {
      if( hardwareVersion == burnHardwareByte )
         return foundDevice = true;
   }
   else
      hardwareVersion = firmwareVersion = hardwareVersionCH = 0x00;

   //If it didn't work, drop to 115.2, ask to have the baud brought to 921.6
   //and try again
   command[0] = 'S';
   command[1] = 0;
   command[2] = 'S';
   command[3] = EOF;

   serial.setSpeedAndDataBits(115200,8,'n',1);
   if(!serial.applySettings())
      return false;

   serial.purgeRX();

   if(!sendCommands())
      return false;

   //if we got a return byte and the device honored our request
   //move back to high speed and get the version again
   if(!serial.getByte(&tmp) || tmp != dataOK)
      return false;

   serial.setSpeedAndDataBits(921600,8,'n',1);

   if(!serial.applySettings())
      return false;

   command[0] = versionCommand;
   command[1] = versionCommand;
   command[2] = EOF;

   serial.purgeRX();

   if(!sendCommands())
      return false;

   if(	serial.getByte(&hardwareVersion) &&
         serial.getByte(&firmwareVersion) &&
         serial.getByte(&hardwareVersionCH) )
   {
      if( hardwareVersion == burnHardwareByte )
         return foundDevice = true;
   }
   else
      return false;
}

char Burn::getHardwareVersion(void)
{
   return hardwareVersion ;
}

char Burn::getFirmwareVersion(void)
{
   return firmwareVersion ;
}

char Burn::getHardwareVersionCH(void)
{
   return hardwareVersionCH ;
}

//Calcuates checksum across a number of bytes
bool Burn::updateChecksum( char c)
{
   if(checksumFirstByte)
   {
      checksum = c;
      checksumFirstByte = false;
   }
   else
      checksum += c;

   return true;
}

//reset checksum across a number of bytes
bool Burn::resetChecksum( void )
{
   return checksumFirstByte = true;
}

//reset checksum across a number of bytes
char Burn::getChecksum( void )
{
   return checksum;
}

//reset checksum across a number of bytes
bool Burn::resetBinIdx( void )
{
   binIdx = 0;
   return true;
}

//function to write file to chip and verify it
//This does the erase, verify of erase, burn and verify in one shot
bool Burn::writeFileToChip(void)
{
   return ( checkForDevice() &&
            eraseChip() &&
            verifyChipIsBlank() &&
            calculateChipOffset() &&
            readFileToMemory() &&
            writeMemoryToChip() &&
            verifyChipToFile() ) ;
}


//Verify bin on chip against the file specified by binFile
//Must take offset into account, depends on file staying open from
//the offset calcualtion function
bool Burn::verifyChipToFile(void)
{
   char * fileOnDisk;
   int fsize;

   //Setup offset
   if(!calculateChipOffset())
      return false;

   //Read the chip into the bin array
   if(!readChipToMemory())
      return false;

   //Go to end of file to get length of file
   file.seekg(0, std::ios::end);

   //Get length of file
   fsize = file.tellg();

   //allocate some memory to hold file
   fileOnDisk = new char[fsize];

   //Go to beginning of file
   file.seekg(0, std::ios::beg);

   //Suck the file into memory
   file.read(fileOnDisk, fsize);

   //Walk the array based upon offsets and make sure that
   //the bytes match up
   for(int i = offsetOnChip; i < romSize ; i++)
      if(fileOnDisk[i-offsetOnChip] != bin[i] )
      {
         delete[] fileOnDisk;
         return false;
      }
   delete[] fileOnDisk;
   return true;
}

//Verify bin on chip against the file specified by binFile
bool Burn::verifyChipIsBlank(void)
{
   if(!readChipToMemory())
      return false;

   for(int i = 0; i < romSize; i++)
      if( bin[i] != (char) 0xFF)
         return false;

   return true;
}
//Function to load file's contents to memory buffer in bin
bool Burn::readFileToMemory(void)
{
   int fsize;
   if(file.is_open())
      file.close();

   file.open(binFile.c_str() , std::ios::in | std::ios::ate | std::ios::binary);
   //if there was an error opening it or the size is larger than the rom, bail
   if( (!file.is_open() ) || file.tellg() > romSize )
      return false;
   fsize = file.tellg();

   //go back to beginning
   file.seekg(0);

   return file.read(bin, fsize);
}

//This will dump the current memory buffer to disk based upon
//selected chip's size
bool Burn::writeMemoryToFile(void)
{
   if(file.is_open())
      file.close();

   file.open(binFile.c_str(), std::ios::out | std::ios::binary);
   //if there was an error opening the file, bail
   if( !file.is_open() )
      return false;

   return file.write(bin, romSize);
}

//Writes a bin to the selected chip from filename specified by binFile
//This assumes the chip is blank and has been verified as such
bool Burn::writeMemoryToChip(void)
{
   unsigned int i;
   //reset current chunk of bin to start and setup the offset

   if( !resetBinIdx())
   {
      //std::cerr << "resetBinIdx failed" << std::endl;
      return false;
   }
   if( !calculateChipOffset())
   {
      //std::cerr << "calcualteChipOffset failed" << std::endl;
      return false;
   }
   //Walk through bin sending chunks of the specified
   //block size
   for( i = offsetOnChip; i < romSize; i+=blockSize)
   {
      if(! buildCommand( 'W', (unsigned char * ) &i, i/(maxBinSize/banks) ))
      {
         //std::cerr << "buildcommand failed" << std::endl;
         return false;
      }
      if(!sendCommands() )
      {
         //std::cerr << "sendCommands failed" << std::endl;
         return false;
      }
      if(!sendDataBlock())
      {
         //std::cerr << "sendDataBlock failed" << std::endl;
         return false;
      }
   }
   //Check to see if we wrote the whole thing out
   if( i >= romSize)
      return true;
   else
      return false;

}

//reads a bin from the chip to the filename specified by binFile
bool Burn::readChipToMemory(void)
{
   char tmp[blockSize+1];
   unsigned int i;

   //reset the index so reads will start at 0
   resetBinIdx();
   //loop counter is used as address counter too
   //it's read in parts by the cp pointer
   //Bank will always be 0 for small chips and ignored by build command
   for( i = 0; i < romSize ; i+=blockSize)
   {
      if(! buildCommand( 'R', (unsigned char *) &i, i/(maxBinSize/banks)) )
      {
         //std::cerr << "resetChecksum failed" << std::endl;
         return false;
      }
      if(! serial.purgeRX() )
      {
         //std::cerr << "purgeRX failed" << std::endl;
         return false;
      }
      if(! sendCommands() )
      {
         //std::cerr << "sendCommands failed" << std::endl;
         return false;
      }

      if(! getDataBlock() )
      {
         //std::cerr << "getdatablock failed with '?', retrying" << std::endl;
         return false;
      }
   }

   //Check to see if we read the whole bin
   if( i >= romSize)
      return true;
   else
      return false;
}

bool Burn::buildCommand( char c, unsigned char * address, int bank)
{
   if(romSize)
   {
      command[0] = romType;
      command[1] = c;
      switch(c)
      {
      case 'R':
      case 'W':
         //make sure block size is sane
         if(blockSize > maxHWBlockSize)
         {
            command[0] = EOF;
            return false;
         }

         //This attempts to hit the last byte written to the rom
         //If we call with a block that would end up being past it's last
         //address, allows read/write functions to be sloppy
         //It seems like it works, but it's frigging ugly as sin
         //Should be rewritten in a more clear fashion, would proabaly cause a bus error on big endian box
         if( romSize -  ( * ((int * ) address)  ) < blockSize  )
         {
            setLastBlockSize((unsigned char ) (romSize - ( * ((int * ) address) )));
            command[2] = getLastBlockSize();
            //std::cerr << "setting requested number of bytes to: " << std::dec << command[2] << std::endl;
         }
         else if(blockSize == maxHWBlockSize)
         {
            command[2] = 0x00;
            setLastBlockSize(0x00);
         }
         else
         {
            command[2] = blockSize;
            setLastBlockSize(blockSize);
         }
         //Make sure 3rd address byte is taken into account for
         //large chips
         if(romType == EECIV || romType == AM29F040 )
         {
            command[3] = bank;
            command[4] = address[1];
            command[5] = address[0];
            command[6] = EOF;
         }
         else
         {
            command[3] = address[1];
            command[4] = address[0];
            command[5] = EOF;
         }
         break;

         //Only allow erases on the EEC/AMD/SST chips
      case 'E':
         if(romType== EECIV || romType == AM29F040)
         {
            command[2] = bank;
            command[3] = EOF;
         }
         else if(romType == SST27SF512)
            command[2] = EOF;
         else
            return false;

         break;
      }
      return true;
   }
   return false;
}
//Erase the chip, docs say it takes 1/2 sec
//So, we sleep a whole second before checking the
//return clode
bool Burn::eraseChip(void)
{
   bool status = true;
   char tmp = 0;

   if( ! serial.purgeRX() )
      return false;

   if(romType == EECIV || romType == AM29F040 )
   {
      for(int i = 0; i < banks && status; i++)
      {
         if(!eraseBank(i))
         {
            return false;
         }
#ifdef WIN32
         Sleep(1000);
#else
         sleep(1);
#endif
         if(!serial.getByte(&tmp))
         {
            return false;
         }
         if(tmp != dataOK)
         {
            return false;
         }
      }
      return true;
   }
   else if(romType == SST27SF512)
   {
      if(! buildCommand('E', NULL, 0) )
      {
         return false;
      }
      if(! sendCommands() )
      {
         return false;
      }
#ifdef WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
      if(!serial.getByte(&tmp))
      {
         return false;
      }
      if(tmp == dataOK)
      {
         return true;
      }
   }
   return false;

}

//Erase bank, only good for 29f040 chips, will return error if other chips selected
bool Burn::eraseBank( int i )
{
   if((romType == EECIV || romType == AM29F040)  &&  i >= 0 && i < banks)
      return ( buildCommand('E', NULL, i) && sendCommands() );

   else
      return false;
}

//Calcuate offset of binary on chip, based upon file size
bool Burn::calculateChipOffset(void)
{
   if(file.is_open())
      file.close();

   file.open(binFile.c_str() , std::ios::in | std::ios::ate | std::ios::binary);
   if(!file.is_open())
      return false;

   offsetOnChip = (int) romSize - (int) file.tellg();

   if(offsetOnChip >= romSize || offsetOnChip < 0 )
   {
      offsetOnChip = 0;
      return  false;
   }
   else
      return true;
}

int Burn::getOffset(void)
{
   return offsetOnChip;
}

//sets the current chip type
//automatically adjusts size
bool Burn::setChipType(ChipType ct)
{
   romType = ct;
   if(romType == NONE) romSize = NONE_SIZE;
   else if(romType == AT29C256) romSize = AT29C256_SIZE;
   else if(romType == M2732A) romSize = M2732A_SIZE;
   else if(romType == AM29F040) romSize = AM29F040_SIZE;
   else if(romType == SST27SF512) romSize = SST27SF512_SIZE;
   else if(romType == EECIV) romSize = EECIV_SIZE;
   //if we cant set the size for this type, bomb
   else
   {
      romType = NONE;
      return false;
   }

   return true;
}

//gets current chip type
ChipType Burn::getChipType(void)
{
   return romType;
}

//sets the current bin file name
bool Burn::setBinFile(std::string s)
{
   binFile = s;
   return true;
}

//gets current file name for bin file
std::string Burn::getBinFile(void)
{
   return binFile;
}

//sets the current com port
//attempts to open it upon set
bool Burn::setComPort(std::string s)
{

   // Put code in here for new/delete
   if(!serial.isOpen())
      if(	serial.setPort(comPort = s)  &&
            serial.openCommPort() &&
            serial.setSpeedAndDataBits(921600,8,'n',1) &&
            serial.setTimeouts(10,0,0,0,0) &&
            serial.applySettings()
        )
         return true;

   return false;
}

//gets current com port
std::string Burn::getComPort(void)
{
   return comPort;
}

//return block size for reads/writes to chip
int Burn::getBlockSize(void)
{
   return blockSize;
}


//set size for reads/writes to chip, must clamp to maxHWBlockSize
bool Burn::setBlockSize(int i)
{
   if(i>maxHWBlockSize)
   {
      lastBlockSize = blockSize = maxHWBlockSize;
      return false;
   }
   else
   {
      lastBlockSize = blockSize = i;
      return true;
   }
}

//These 2 functions are really only needed where block sizes and data are non ^2
//since the reads end up with an odd size at last read/write
int Burn::getLastBlockSize(void)
{
   return lastBlockSize;
}

bool Burn::setLastBlockSize(int i)
{
   if(i >= maxHWBlockSize || i>= blockSize)
   {
      lastBlockSize = blockSize;
      return false;
   }

   else
      lastBlockSize = i;

   return true;
}

//This probably needs some more error checking
//The hardware doesn't take lightly to byte at a time either
//Wants to have all bytes in command presented ASAP, doesn't like 1 byte at a time
bool Burn::sendCommands(void)
{
   int i, len;
   char tmpCmd[maxCommandLen+1];

   if( !resetChecksum() )
      return false;

   for(i = len = 0; i < maxCommandLen && command[i] != EOF; i++)
   {
      updateChecksum(command[i]);
      tmpCmd[len++] = command[i];
   }

   //The write data needs to be included in the checksum
   //So if it's a write don't include the checksum just yet
   if(command[readWriteIdx] != writeCommand )
      tmpCmd[len++] = getChecksum();

   return serial.sendBytes( tmpCmd, len );
}

//This depends on the binIdx variable
//should be reset in any high-level write function and will increment itself
bool Burn::sendDataBlock(void)
{
   int i, sz;
   char tmp;
   char tmpDataBlock[maxHWBlockSize+1];
   bool isOK = true;

   //if lastblocsize is set smaller than the
   //normal blocksize, we need short read/write
   if( lastBlockSize < blockSize)
      sz = lastBlockSize;
   else
      sz = blockSize;

   if( sz == 0)
      sz = maxHWBlockSize;

   //purge the receive buffers so we don't get a false return on
   //the dataOK check byte
   if(! serial.purgeRX() )
      return false;

   for(i = 0; i < sz ; i++)
   {
      updateChecksum(bin[binIdx]);
      tmpDataBlock[i] = bin[binIdx++];
   }

   tmpDataBlock[sz] = getChecksum();
   if(! serial.sendBytes(tmpDataBlock, sz+1))
      return false;

   //read from port to get the return code from device
   isOK = serial.getByte(&tmp);

   if(isOK && tmp == dataOK)
      return true;

   return false;
}

bool Burn::getDataBlock(void)
{
   int i, sz;
   char tmp[maxHWBlockSize+1];

   //if lastblocsize is set smaller than the
   //normal blocksize, we need short read/write
   if(lastBlockSize < blockSize)
      sz = lastBlockSize;
   else
      sz = blockSize;

   if(sz == 0)
      sz = maxHWBlockSize;

   //This will be sensitive to serial timeouts if not set properly
   //Or if it's run in blocking i/o mode since getbyte won't return
   //Timeouts probably need to be set to something like 100ms/500ms
   if(!serial.getBytes( tmp, sz+1))
   {
      //std::cerr << "getBytes failed in getDataBlock size: "<< sz << std::endl;
      return false;
   }

   //This needs to walk the data bytes returned and
   //update the checksum, last byte returned should be the checksum
   //provided by the device
   if( !resetChecksum())
      return false;

   for(i=0; i<sz; i++)
      updateChecksum(tmp[i]);

   //If the data was recieved OK, copy the data into the array
   //using the bin index, checksum will be last byte in tmp array
   if(tmp[sz]==getChecksum())
   {
      for(i=0; i<sz && binIdx<maxBinSize; i++)
      {
         bin[binIdx++] = tmp[i];
      }
      return true;
   }

   //std::cerr << "Checksum verification on getDataBlock failed" << std::endl;
   return false;
}
Burn::Burn( void )
{
   foundDevice = false;
   romType = NONE;
   lastBlockSize = blockSize = maxHWBlockSize;
   checksumFirstByte = true;
   offsetOnChip = 0;
   hardwareVersion = firmwareVersion = hardwareVersionCH = 0x00;

}
