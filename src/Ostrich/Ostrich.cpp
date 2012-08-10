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

/* This is a C++ abstraction for
 * the moates.net ostrich eeprom emulator
 * needs the Serial class that is included
 * in the Seraial.h/Serial.cpp file
 */
#include "Ostrich.h"

bool Ostrich::updateChecksum(char c)
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


//Starts checksum process over again, will not change last checksum
bool Ostrich::resetChecksum(void)
{
   return checksumFirstByte = true;
}

//Returns current checksum
char Ostrich::getChecksum(void)
{
   return checksum;
}

//Resets the index of the in memory array for reads/writes to ostrich
bool Ostrich::resetBinIdx(void)
{
   binIdx = 0;
   return true;
}

//This operates on currently set read/write bank
bool Ostrich::verifyBankToFile(void)
{
   char * fileOnDisk = NULL;
   int fsize = 0;

   //Setup offset
   if(!calculateOffset())
      return false;

   //Read the chip into the bin array
   if(!readBankToMemory())
      return false;

   if(file.is_open())
      file.close();

   file.open(binFileName.c_str() , std::ios::in | std::ios::ate | std::ios::binary);
   //if there was an error opening it or the size is larger than the rom, bail
   if( (!file.is_open() ) || file.tellg() > currentBankSize )
   {
      file.close();
      return false;
   }

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
   for(int i = offset; i < currentBankSize ; i++)
      if(fileOnDisk[i-offset] != bin[i] )
      {
         delete[] fileOnDisk;
         file.close();
         return false;
      }
   delete[] fileOnDisk;
   file.close();
   return true;
}

bool Ostrich::writeMemoryToBank(void)
{
   int i = 0;
   //reset current chunk of bin to start and setup the offset

   if( !resetBinIdx())
   {

#ifdef DEBUG
      std::cerr << "resetBinIdx failed" << std::endl;
#endif

      return false;
   }
   if( !calculateOffset())
   {
#ifdef DEBUG
      std::cerr << "calcualteOffset failed" << std::endl;
#endif

      return false;
   }
   //Walk through bin sending chunks of the specified
   //block size
   for( i = offset; i < currentBankSize; i+=lastBlockSize)
   {
      if(! buildCommand( 'W', i , 0))
      {
#ifdef DEBUG
         std::cerr << "buildcommand failed" << std::endl;
#endif

         return false;
      }
      if(!sendCommands() )
      {

#ifdef DEBUG
         std::cerr << "sendCommands failed" << std::endl;
#endif

         return false;
      }
      if(!sendDataBlock())
      {
#ifdef DEBUG
         std::cerr << "sendDataBlock failed" << std::endl;
#endif

         return false;
      }
   }
   //Check to see if we wrote the whole thing out
   if( i >= currentBankSize)
      return true;
   else
      return false;

}

bool Ostrich::readBankToMemory(void)
{
   int i = 0;

   //reset the index so reads will start at 0
   resetBinIdx();
   //loop counter is used as address counter too

   for( i = 0; i < currentBankSize ; i+=blockSize)
   {
#ifdef DEBUG
      std::cerr << "in getbank loop for i=" << i << std::endl;
#endif
      if(! buildCommand( 'R', i, 0) )
      {
#ifdef DEBUG
         std::cerr << "buildCommand failed" << std::endl;
#endif
         return false;
      }
      if(! serial.purgeRX() )
      {
#ifdef DEBUG
         std::cerr << "purgeRX failed" << std::endl;
#endif
         return false;
      }
      if(! sendCommands() )
      {
#ifdef DEBUG
         std::cerr << "sendCommands failed" << std::endl;
#endif
         return false;
      }

      if(! getDataBlock() )
      {
#ifdef DEBUG
         std::cerr << "getdatablock failed" << std::endl;
#endif
         return false;
      }
   }

   //Check to see if we read the whole bin
   if( i >= currentBankSize && currentBankSize > 0)
      return true;
   else
      return false;
}

bool Ostrich::writeMemoryToFile(void)
{
   bool b;
   if(file.is_open())
      file.close();

   file.open(binFileName.c_str(), std::ios::out | std::ios::binary);
   //if there was an error opening the file, bail
   if( !file.is_open() )
      return false;
   b =  file.write(bin, currentBankSize);
   file.close();
   return b;
}

bool Ostrich::readFileToMemory(void)
{
   int fsize = 0;
   bool b;
   if(file.is_open())
      file.close();

   file.open(binFileName.c_str() , std::ios::in | std::ios::ate | std::ios::binary);
   //if there was an error opening it or the size is larger than the rom, bail
   if( (!file.is_open() ) || file.tellg() > currentBankSize )
      return false;
   fsize = file.tellg();

   //go back to beginning
   file.seekg(0);
   b =  file.read(bin, fsize);
   file.close();
   return b;
}
// this will write a file to a bank
// using an automatic offset based upon
// bank size and the file size
// just call set functions for filename and bank
// and call this to get the file written out to the emulator
// returns false if failure occured somewhere
//
bool Ostrich::writeFileToBank(void)
{
   return ( checkForDevice() &&
            calculateOffset() &&
            readFileToMemory() &&
            writeMemoryToBank() &&
            verifyBankToFile() ) ;
}

//This will set the update bank on the hardware based upon passed bank number
//and character, 'U' for read/write, 'P' for persistent, 'E' for emulation
//Needs to handle switch between banked and full device mode gracefully
bool Ostrich::setBank(int i , char c)
{
   char tmp = 0;
   if( i < wholeEnchilada)
      currentBankSize = maxBinSize/banks;

   else if( i == wholeEnchilada)
      currentBankSize = maxBinSize;

   else
      return false;

#ifdef DEBUG
   std::cerr << "currentBankSize = " << currentBankSize << std::endl;
#endif


   //Catch case where we go from bank to whole deivce or whole device to bank
   //Force all banks to be the same if whole device change, could be disruptive
   //But doing it other ways leads to inconsistencies
   if( (i == wholeEnchilada && (updateBank != wholeEnchilada || emuBank != wholeEnchilada || persistentBank != wholeEnchilada)) ||
         (i != wholeEnchilada && (updateBank == wholeEnchilada || emuBank == wholeEnchilada || persistentBank == wholeEnchilada)) )
   {
#ifdef DEBUG
      std::cerr<< "Changing from whole device presentation to banked presentation or vice-versa" << std::endl;
#endif

      emuBank = updateBank = persistentBank = i;
      if(! forceAllBanks() )
         return false;
      else
         return true;
   }

   switch(c)
   {
   case 'U':
#ifdef DEBUG
      std::cerr<< "setting the update bank" << std::endl;
#endif
      updateBank = i;
      break;

   case 'E':
#ifdef DEBUG
      std::cerr<< "setting the emulation bank" << std::endl;
#endif
      emuBank = i;
      break;

   case 'P':
      persistentBank = i;
#ifdef DEBUG
      std::cerr<< "setting the persistent bank" << std::endl;
#endif

      break;

   default:
      return false;
      break;
   }

   //Set the bank on the hardware
   if(serial.isOpen())
   {
      serial.purgeTX();
      serial.purgeRX();
   }
   else
      return false;
   if( !buildCommand(bankCommand, 'S', c) )
   {
#ifdef DEBUG
      std::cerr << "Build command failed for bank set" << std::endl;
#endif

      return false;
   }
   if( sendCommands() )
   {
      //Need to check the return code here from the send commands
      //Ostrich should give back a 'O'

      if(serial.getByte(&tmp) && tmp == dataOK)
      {
#ifdef DEBUG
         std::cerr << "send commands for bank set succeeded" << std::endl;
#endif

         return true;
      }
#ifdef DEBUG
      std::cerr << "send commands for bank set succeeded, but device returned failure" << std::endl;
#endif

   }

#ifdef DEBUG
   std::cerr << "sendcommands failed for bank set" << std::endl;
#endif

   return false;
}

//This is a convience function to change all banks as set
//Assumes that the various bank settings are already loaded into
//the Bank variables
bool Ostrich::forceAllBanks(void)
{
   char tmp[bankTypes];

   if(serial.isOpen())
   {
      serial.purgeRX();
      serial.purgeTX();
   }
   else
      return false;

   for(int i = 0; i< bankTypes; i++)
      tmp[i] = 0;

   //if all bank settings are updated and the port says OK 3x over
   if(  ( buildCommand(bankCommand, 'S', 'U') && sendCommands())
         && ( buildCommand(bankCommand, 'S', 'E') && sendCommands())
         && ( buildCommand(bankCommand, 'S', 'P') && sendCommands()) )
   {
      if(serial.getBytes(tmp, 3) )
      {
         for(int i = 0; i< bankTypes; i++)
            if(tmp[i] != dataOK)
               return false;

         //if we get here all the bank set commands returned OK
         return true;
      }
   }

   return false;

}

// this will query the hardware to see which bank it is set to
// for the next read/write request returns 1 greater than the number of banks on error
// if the device returns something that Doesn't match the internal variables
// we update the internal variables
int Ostrich::getBank(char c)
{
   char tmp = 0;

   if(serial.isOpen())
   {
      serial.purgeRX();
      serial.purgeTX();
   }
   else
      return false;

   if(!	buildCommand(bankCommand, 'G', c))
   {
#ifdef DEBUG
      std::cerr << "BuildCommand failed in getbank" << std::endl;
#endif
   }
   if(!	sendCommands())
   {
#ifdef DEBUG
      std::cerr << "SendCommand failed in getbank" << std::endl;
#endif
   }
   if(!	serial.getByte(&tmp))
   {
#ifdef DEBUG
      std::cerr << "getByte failed after build and send succeeded in getbank" << std::endl;
#endif

      return wholeEnchilada+1;
   }

#ifdef DEBUG
   std::cerr << "got byte back from read: " << (int) tmp << std::endl;
#endif


   switch(c)
   {
   case 'U':
      if(tmp == updateBank)
         return updateBank;

      //This should only be the initial case
      else if (updateBank == wholeEnchilada+1)
      {
         if(currentBankSize == 0 && tmp < wholeEnchilada)
            currentBankSize = maxBinSize/banks;
         return updateBank = tmp;
      }
      else if( setBank(tmp, 'U') )
         return updateBank;
      else
         return wholeEnchilada+1;
      break;

   case 'E':
      if(tmp == emuBank)
         return emuBank;

      //This should only be the initial case
      else if (emuBank == wholeEnchilada+1)
      {
         if(currentBankSize == 0 && tmp < wholeEnchilada)
            currentBankSize = maxBinSize/banks;
         return emuBank = tmp;
      }
      else if( setBank(tmp, 'E') )
         return emuBank;
      else
         return wholeEnchilada+1;
      break;

   case 'P':
      if(tmp == persistentBank)
         return persistentBank;

      //This should only be the initial case
      else if (persistentBank == wholeEnchilada+1)
      {
         if(currentBankSize == 0 && tmp < wholeEnchilada)
            currentBankSize = maxBinSize/banks;
         return persistentBank = tmp;
      }
      else if( setBank(tmp, 'P') )
         return persistentBank;
      else
         return wholeEnchilada+1;
      break;

   default:
      return wholeEnchilada+1;
      break;
   }

   return wholeEnchilada+1;
}
// this will calculate the offset the file specified by
// the binFileName class variable needs to have when
// writing data to the ostrich
//
bool Ostrich::calculateOffset(void)
{
   int tmp;
   if(file.is_open())
      file.close();

   file.open(binFileName.c_str() , std::ios::in | std::ios::ate | std::ios::binary);
   if(!file.is_open())
      return false;

   tmp = currentBankSize - file.tellg();
   if(tmp >= 0)
   {
      if(tmp < currentBankSize)
         offset = tmp;

      else
         offset = 0;
      file.close();
      return true;
   }
   file.close();
   return	false;
}
int Ostrich::getOffset(void)
{
   return offset;
}

bool Ostrich::setBinFile(std::string s )
{
   binFileName = s;
   return true;
}

std::string Ostrich::getBinFile(void)
{
   return binFileName ;
}

bool Ostrich::setTraceFile(std::string s)
{
   traceFile = s;
   return true;
}

std::string Ostrich::getTraceFile(void)
{
   return traceFile;
}

bool Ostrich::getWriteTraceToFile(void)
{
   return traceToFile ;
}

bool Ostrich::setWriteTraceToFile(bool b)
{
   traceToFile = b;
   return true;
}

bool Ostrich::setComPort(std::string s)
{
   return serial.setPort(comPort = s);
}

std::string Ostrich::getComPort(void)
{
   return comPort;
}

bool Ostrich::sendCommands(void)
{
   int i, len;
   char tmpCmd[maxCommandLen+1];

   if( !resetChecksum() )
      return false;

   for(i = len = 0; i < maxCommandLen && command[i] != EOF; i++)
   {
      updateChecksum(command[i]);
#ifdef DEBUG
      std::cerr << " command byte i: " << i << " is: " << command[i] << std::endl;
#endif
      tmpCmd[len++] = command[i];
   }

#ifdef DEBUG
   std::cerr << " command length  is: " << len << std::endl;
#endif

   //The write data needs to be included in the checksum
   //So if it's a write don't include the checksum just yet
   if(	command[writeIdx] != writeCommand &&
         command[writeIdxBulk] != writeCommand &&
         command[0] != versionCommand )
      tmpCmd[len++] = getChecksum();

   return serial.sendBytes( tmpCmd, len );
}
int Ostrich::getBlockSize(void)
{
   return blockSize;
}

//This enforces 256 byte boundaries on large blocks
//Will set blockSize to next nearest size, and return failure if block is
//incorrectly sized
bool Ostrich::setBlockSize(int i)
{
   if(i > bulkBlockSize && i <= maxBulkBlockSize)
   {
      if(!(i % bulkBlockSize))
      {
         blockSize = i;
         return true;
      }
      else
      {
         blockSize = i - ( i % bulkBlockSize) ;
         return false;
      }

   }
   else if (i <= bulkBlockSize && i > 0)
   {
      blockSize = i;
      return true;
   }

   return false;
}

int Ostrich::getLastBlockSize(void)
{
   return lastBlockSize;
}

//This is passed in the amount of data left to write to the chip
//based upon address of current write, returns true if an adjustment
//to the block size was required
bool Ostrich::setLastBlockSize(int i)
{
   //If the amount left to write isn't on a bulk block aligned
   //address, write enough to align next write
   if( i >= bulkBlockSize  && i % bulkBlockSize )
   {
      lastBlockSize = i % bulkBlockSize;
#ifdef DEBUG
      std::cerr << "lastBlockSize set because of non-bulk-aligned offset to : " << lastBlockSize << std::endl;
#endif
   }
   //if the amount of space left to write
   //to can handle the block, leave the size alone
   else if( i>= blockSize)
   {
      lastBlockSize = blockSize;
#ifdef DEBUG
      std::cerr << "lastBlockSize left untouced at size : " << lastBlockSize << std::endl;
#endif
      return false;
   }
   //otherwise, if we're trying to write large blocks and we come up short of the
   //requested block size, but we can still use a bulk write, set the block size
   //to the next largest bulk size available, by removing the non-full  bulk block size from end
   //Any alignment issues should be handled in first 'if' case
   else if(i < blockSize && i > bulkBlockSize)
   {
      lastBlockSize = blockSize - ( blockSize - i );
#ifdef DEBUG
      std::cerr << "lastBlockSize set for end bulk write of size: " << lastBlockSize << std::endl;
#endif

   }
   //This branch should never be taken with current hardware since bulkBlockSize == maxHWBlockSize
   //put in just in case things change in future
   else if( i < bulkBlockSize && i > maxHWBlockSize)
      lastBlockSize = maxHWBlockSize;

   //Otherwise set the block size to the last bit of address space available
   else
      lastBlockSize = i;

#ifdef DEBUG
   std::cerr << "setlastBlockSize returns with lastBlockSize set to: " << lastBlockSize << std::endl;
#endif

   return true;
}

char Ostrich::getHardwareVersion(void)
{
   return hardwareVersion;
}

char Ostrich::getFirmwareVersion(void)
{
   return firmwareVersion;
}

char Ostrich::getHardwareVersionCH(void)
{
   return hardwareVersionCH;
}

bool Ostrich::getSerialNumber(char * cp)
{
   for(int i = 0; i < serialNumberLen; i++)
      cp[i]  = serialNumber[i];

   return true;
}
int Ostrich::getSerialNumberLen(void)
{
   return serialNumberLen;
}

char Ostrich::getVendorID(void)
{
   return vendorID;
}

bool Ostrich::getTraceWindowed(void)
{
   return traceIsWindowed;
}

bool Ostrich::setTraceWindowed(bool b )
{
   traceIsWindowed = b;
   if(traceIsWindowed && traceIsTriggered)
      traceIsTriggered = false;

   return true;
}

bool Ostrich::getTraceTriggered(void)
{
   return traceIsTriggered;
}

bool Ostrich::setTraceTriggered(bool b)
{
   traceIsTriggered = b;
   if(traceIsTriggered)
   {
      setTraceNonRedundant(b);
      setTraceWindowed(!b);
   }
   return true;
}

bool Ostrich::getTraceNonRedundant(void)
{
   return traceNonRedundant;
}

bool Ostrich::setTraceNonRedundant(bool b)
{
   traceNonRedundant = b;
   if(!traceNonRedundant && traceIsTriggered)
      traceIsTriggered = false;

   return true;
}

bool Ostrich::getTraceRelativeAddress(void)
{
   return traceRelativeAddress;
}

bool Ostrich::setTraceRelativeAddress(bool b)
{
   traceRelativeAddress = b;
   return true;
}

int Ostrich::getTraceStartAddress(void)
{
   return traceStartAddress;
}

bool Ostrich::setTraceStartAddress(int i )
{
   traceStartAddress = i ;
   return true;
}

int Ostrich::getTraceEndAddress(void)
{
   return traceEndAddress;
}

bool Ostrich::setTraceEndAddress(int i )
{
   traceEndAddress = i;
   return true;
}

char Ostrich::getTraceAddressBytes(void)
{
   return traceAddressBytes;
}

bool Ostrich::setTraceAddressBytes(char c )
{
   if(c > maxAddressBytes)
   {
      traceAddressBytes =  maxAddressBytes;
      return false;
   }
   else if ( c > 0 )
   {
      traceAddressBytes = c;
      return true;
   }
   else
   {
      traceAddressBytes = c;
      return false;
   }
}

bool Ostrich::setTraceAddrPerPacket(int i)
{
   if( i > 0 && i <= maxAddressesPerPacket)
   {
      addressesPerPacket = i;
      return true;
   }
   else
      addressesPerPacket = maxAddressesPerPacket;

   return false;

}
int Ostrich::getTraceAddrPerPacket(void)
{
   return (int) addressesPerPacket;
}

bool Ostrich::setPacketsPerTrace(char c )
{
   if( c > 0 && c <= maxPacketsPerTrace)
   {
      packetsPerTrace = c;
      return true;
   }
   else
      packetsPerTrace = maxPacketsPerTrace;

   return false;
}

char Ostrich::getPacketsPerTrace(void)
{
   return packetsPerTrace;
}

bool Ostrich::setTraceBuf(int * ip, int i)
{
   extTraceBuffer = ip;
   extTraceBufferSize = i;
   return true;
}
bool Ostrich::wasHit(int i)
{
   if( hitMap[i] )
      return true;

   return false;
}

bool Ostrich::resetHitMap(void)
{
   memset( (void *) hitMap, 0, maxBinSize);
   return true;
}

bool Ostrich::resetHitIdx(void)
{
   hitIdx = 0;
   return true;
}
//This will return EOF when it's walked the whole hitBuffer
//or -1 on error
int Ostrich::getNextAddressHit(void)
{
   int hitAddr = 0;

   if(hitBuffer[hitIdx] == dataOK && hitIdx == (traceAddressBytes * addressesPerPacket * packetsPerTrace)+2 )
      return EOF;

   if(hitIdx == 0)
      hitIdx++;

   if(hitIdx >= hitBufferMaxSize)
   {
      resetHitIdx();
      return -1;
   }
   if(traceAddressBytes == 3)
   {
      hitAddr = (hitBuffer[hitIdx++] << 16);
      hitAddr += (hitBuffer[hitIdx++] << 8);
      hitAddr += (hitBuffer[hitIdx++]);
   }
   else if(traceAddressBytes == 2)
   {
      hitAddr = (hitBuffer[hitIdx++] << 8);
      hitAddr += (hitBuffer[hitIdx++]);
   }
   else if (traceAddressBytes == 1)
   {
      hitAddr = (hitBuffer[hitIdx++]);
   }

   if(hitAddr < maxBinSize)
      return hitAddr;

   return -1;

}

// This will trace data and write it to a map
// that indicates each index that was hit since
// the map was last reset
//
bool Ostrich::getTraceToBufMap(void)
{
   int tmp = 0;
   if(	extTraceBuffer &&
         serial.isOpen() &&
         buildCommand(traceCommand, 0, 0) &&
         sendCommands() &&
         getTraceBlock() &&
         hitBuffer[0] == dataOK
     )
   {
      resetHitIdx();
      for(int i=0; i<extTraceBufferSize && tmp != EOF ; i++)
      {
         tmp = getNextAddressHit();
         if(tmp != -1)
         {
            extTraceBuffer[i] = tmp;
            hitMap[tmp] = 1;
            if(traceToFile && trace.is_open())
               trace << std::dec << tmp << std::endl;
         }
         else
            return false;
      }
      //if we updated everything in the buffer and the map
      //successfully, return OK
      if(tmp == EOF )
         return true;

      //otherwise keep trying to update the map, buffer may be short
      else
      {
         while((tmp = getNextAddressHit()) != EOF)
         {
            if( tmp != -1)
            {
               hitMap[tmp] = 1;
               if(traceToFile && trace.is_open())
                  trace << std::dec << tmp << std::endl;
            }
            else
               return false;
         }
      }
   }
   //we return false even if the map update was OK
   //becuse the buffer update failed
   return false;
}

// This reads the trace data from the Ostrich
// by building requesting a trace with the current flags
// as set in class variables, and requests a trace block
// to the hit buffer, and copies it into the external trace
// buffer as provided by user, also copies it to the file if used
//
bool Ostrich::getTraceToBuf(void)
{
   int tmp = !EOF;
   if(	extTraceBuffer &&
         serial.isOpen() &&
         buildCommand(traceCommand, 0, 0) &&
         sendCommands() &&
         getTraceBlock() &&
         hitBuffer[0] == dataOK
     )
   {
      resetHitIdx();
      for(int i=0; i<extTraceBufferSize && tmp != EOF; i++)
      {
         tmp = getNextAddressHit();
         if(tmp != -1)
         {
            extTraceBuffer[i] = tmp;
            if(traceToFile && trace.is_open())
               trace << std::dec << tmp << std::endl;
         }
         else
            return false;
      }
      return true;
   }

   return false;
}
bool Ostrich::getTraceToMap(void)
{
   int tmp = !EOF;
   if( 	serial.isOpen() &&
         buildCommand(traceCommand, 0, 0) &&
         sendCommands() &&
         getTraceBlock() &&
         hitBuffer[0] == dataOK
     )
   {
      resetHitIdx();

      while((tmp = getNextAddressHit()) != EOF)
      {
         if(tmp != -1)
         {
            hitMap[tmp] = 1;
            if(traceToFile && trace.is_open())
               trace << std::dec << tmp << std::endl;
         }
         else
            return false;
      }
      return true;
   }
   return false;
}
/* this sets up a trace and gets the data to a file
 * it will leave the trace file open across calls
 */
bool Ostrich::getTraceToFile(void)
{
   int tmp = !EOF;

   if(!trace.is_open())
      if(!openTraceFile())
         return false;

   if( 	serial.isOpen() &&
         buildCommand(traceCommand, 0, 0) &&
         sendCommands() &&
         getTraceBlock() &&
         hitBuffer[0] == dataOK
     )
   {
      while((tmp = getNextAddressHit()) != EOF)

      {
         if(tmp != -1 )
            trace << std::dec << tmp << std::endl;
         else
            return false;
      }
      return true;
   }
   return false;

}

/* This will open the trace file, needed to trace to a file
 * from either the map or  buffer trace functions
 *
 */
bool Ostrich::openTraceFile(void)
{
   trace.open(traceFile.c_str(), std::ios::out);
   return traceToFile = file.is_open();
}

bool Ostrich::closeTraceFile(void)
{
   trace.close();
   traceToFile = false;
   return true;
}

bool Ostrich::getHitMap(char * cp)
{
   if(cp == NULL)
      return false;
   return getHitMap( cp, 0, maxBinSize);

}
bool Ostrich::getHitMap(char * cp, int i )
{
   if(cp == NULL || i > maxBinSize)
      return false;

   return getHitMap( cp, 0, i);
}
bool Ostrich::getHitMap(char * cp, int start, int end)
{
   //wordls longest error check in a single if
   if( cp == NULL || start >maxBinSize || start < 0 || end < 0 || end > maxBinSize || start >= end)
      return false;

   for(int i=0; i < end-start; i++)
      cp[i] = hitMap[start+i];

   return true;
}
//Please note that on an ostrich this is *very* sensitive to amount of buffer space
//serial device has in OS or speed on Ubuntu under VMware setting the block size to 4k often
//results in failures to read a whole block + csum back, on OSX I can reliably read a 64k bin in
//on native HW one shot. so if you have issues try and reduce the block size
//sizes (<=256k) seem to be bulletproof, they succeed no matter where I try them
bool Ostrich::getDataBlock(void)
{
   int sz = 0;
   char tmp = 0;

   //if lastblocsize is set smaller than the
   //normal blocksize, we need short read/write
   if(lastBlockSize < blockSize)
      sz = lastBlockSize;
   else
      sz = blockSize;

   if( binIdx + sz <= currentBankSize)
   {
      if(!serial.getBytes( bin+binIdx, sz))
      {
#ifdef DEBUG
         std::cerr << "getBytes failed in getDataBlock size: "<< sz << std::endl;
#endif
         return false;
      }
      if(!serial.getByte(&tmp))
      {
#ifdef DEBUG
         std::cerr << "read failed to return checksum" << std::endl;
#endif
         return false;
      }
#ifdef DEBUG
      std::cerr << "getBytes read back: " << sz << " bytes." << std::endl;
#endif

   }
   else
   {
#ifdef DEBUG
      std::cerr << "getBytes failed because of overflow in bin array size of read block: " << sz << std::endl;
#endif

#ifdef DEBUG
      std::cerr << "getBytes failed because of overflow in bin array current index: " << binIdx << std::endl;
#endif

      return false;
   }
#ifdef DEBUG
   std::cerr << "getBytes about to verify checksum sz is : " << sz << std::endl;
#endif

   //reset the checksum and check it
   if( !resetChecksum())
      return false;

   for(int i=0; i<sz && binIdx+i<currentBankSize; i++)
   {
#ifdef DEBUG
      std::cerr << "checksumming byte " << binIdx+i << std::endl;
#endif

      updateChecksum(bin[binIdx+i]);
   }
   if(tmp==getChecksum())
   {
      binIdx+=sz;
      return true;
   }
   //If something went wrong, Reset the bin index and return a failue
#ifdef DEBUG
   std::cerr << "Checksum verification on getDataBlock failed returned sum is: " << (int) tmp;
   std::cerr << " computed sum is: " << (int) getChecksum() << std::endl;
#endif

   return false;
}
//Should consider adjusting the serial port timeouts
bool Ostrich::getTraceBlock(void)
{
   int sz;

   sz = (traceAddressBytes * addressesPerPacket * packetsPerTrace) + 2;

   if(sz <= hitBufferMaxSize && serial.getBytes(hitBuffer, sz))
      if(hitBuffer[0] == dataOK && hitBuffer[sz-1] == dataOK)
         return true;

   return false;
}
/* this shovels the requested block of data out to the ostrich
 * it uses the lastblocsize variable to see how big it's current write
 * to the serial port should be so that proper bulk and other sizes
 * are used for writes
 */
bool Ostrich::sendDataBlock(void)
{
   int sz;
   char tmp;

   //if lastblocsize is set smaller than the
   //normal blocksize, we need short read/write
   if( lastBlockSize < blockSize)
      sz = lastBlockSize;
   else
      sz = blockSize;

#ifdef DEBUG
   std::cerr << "sendng data to index: "<<std::hex << binIdx+offset << " of count: " << sz << std::endl;
#endif

   for(int i = 0; i < sz ; i++)
      updateChecksum(bin[binIdx+i]);

   tmp = getChecksum();

   //purge the receive buffers so we don't get a false return on
   //the dataOK check byte
   if(! serial.purgeRX() )
      return false;

   //attempt to send the data, checksum
   //retrieve the acknowledgement and verify it;
   if( 	serial.sendBytes(bin+binIdx, sz) &&
         serial.sendByte(&tmp) &&
         serial.getByte(&tmp) &&
         tmp == dataOK
     )
   {
      binIdx+=sz;
      return true;
   }
#ifdef DEBUG
   std::cerr << "Failed to send bytes at: " << binIdx << " of count: " << sz << std::endl;
#endif

   return false;

}

/* this function builds the command to send to the
 * ostrich, all requests in case of reads and writes
 * are setup to calc the lastblocsize variable that is
 * used in real write call, all trace options are also
 * set in the trace part of the switch
 *
 */
bool Ostrich::buildCommand(int cmd , int addr, int bank)
{
   int cmdIdx=0;
   int sz = 0;
   int leftToWrite = currentBankSize - addr;
#ifdef DEBUG
   std::cerr << "buildCommand got: ";
   std::cerr << (char) cmd << " " << (char) addr << " " << (char) bank ;
   std::cerr << "hex: " << std::hex<< cmd << " " <<  addr << " " << bank << std::endl;
#endif

   switch(cmd)
   {
   case versionCommand:
      command[cmdIdx++] = versionCommand;
      command[cmdIdx++] = versionCommand;
      command[cmdIdx] = EOF;
      break;

   case speedCommand:
      command[cmdIdx++] = speedCommand;
      command[cmdIdx++] = 0;
      command[cmdIdx] = EOF;
      break;

   case serialNumCommand:
      command[cmdIdx++] = serialNumCommand;
      command[cmdIdx++] = 'S';
      command[cmdIdx] = EOF;
      break;

   case readCommand:
   case writeCommand:
      //setup the current block size based upon the address space
      //Left to be written to
      setLastBlockSize( leftToWrite );
      sz = lastBlockSize;

      //Test if curent write size is in the bulk size region
      if(sz >= bulkBlockSize )
         command[cmdIdx++] = 'Z';

      command[cmdIdx++] = cmd;

      //If here, and the block is greater than a
      //bulkblocksize it should be evenly divisible
      if(sz >= bulkBlockSize)
         sz /= bulkBlockSize;

      //At this point the size should be down to where it fits in the
      //single byte and setup properly for bulk or normal reads/writes
      command[cmdIdx++] = sz;

      //Now we setup the address to write to for the command
      //when written to the serial port the upper part of int will be trunc'd
      //if reading/writing bulk blocks use correct addresses since it reads 256 byte chunks
      //lower address byte is multipled by 256 on device

      //this needs to take into account non-bulk-sized offsets
      if(lastBlockSize >= bulkBlockSize)
      {
         command[cmdIdx++] = (addr / bulkBlockSize) / 256 ;
         command[cmdIdx++] = addr / bulkBlockSize  ;
      }
      else
      {
         command[cmdIdx++] = addr / 256;
         command[cmdIdx++] = addr ;
      }
      command[cmdIdx] = EOF;
      break;

   case bankCommand:
      command[cmdIdx++] = bankCommand;
      if(addr == 'S')
      {
         if( bank == 'E')
         {
            command[cmdIdx++] = 'E';
            command[cmdIdx++] = emuBank;
            command[cmdIdx] = EOF;
         }
         else if( bank == 'P' )
         {
            command[cmdIdx++] = 'S';
            command[cmdIdx++] = persistentBank;
            command[cmdIdx] = EOF;
         }
         else if( bank == 'U' )
         {
            command[cmdIdx++] = 'R';
            command[cmdIdx++] = updateBank;
            command[cmdIdx] = EOF;
         }
         else
         {
            command[0] = EOF;
            return false;
            break;
         }
      }
      else if(addr == 'G')
      {
         if( bank == 'E')
         {
            command[cmdIdx++] = 'E';
            command[cmdIdx++] = 'R';
            command[cmdIdx] = EOF;
         }
         else if( bank == 'P' )
         {
            command[cmdIdx++] = 'E';
            command[cmdIdx++] = 'S';
            command[cmdIdx] = EOF;
         }
         else if( bank == 'U' )
         {
            command[cmdIdx++] = 'R';
            command[cmdIdx++] = 'R';
            command[cmdIdx] = EOF;
         }
         else
         {
            command[0] = EOF;
            return false;
            break;
         }
      }
      else
         return false;

      break;

   case traceCommand:
      command[cmdIdx++] = traceCommand;
      //Zero 2nd byte out and build bitmask
      command[cmdIdx] = 0x00;
      //The doc shows some specific req's for the triggered tracing
      //They're enforced in the set, but just in case one of the other
      //functions is called they're forced below
      if(traceIsTriggered)
      {
         command[cmdIdx] |= triggerStartAddress;
         //if the trigger end address is realistic use it to set a trace stop
         if(triggerEndAddress < maxBinSize && triggerEndAddress < currentBankSize)
            command[cmdIdx] |= triggerEndAddress;
         traceNonRedundant = true;
         traceIsWindowed = false;
      }
      else if(traceIsWindowed)
         command[cmdIdx] |= windowedTrace;

      if(traceNonRedundant)
         command[cmdIdx] |= nonRedundantTrace;

      if(traceRelativeAddress)
         command[cmdIdx] |= relativeAddress;

      if(traceAddressBytes == 1)
         command[cmdIdx] |= oneByteAddress;

      else if(traceAddressBytes == 2)
         command[cmdIdx] |= twoByteAddress;

      cmdIdx++;
      command[cmdIdx++] = 0;
      command[cmdIdx++] = 0;

      //Set number of hits in packet
      command[cmdIdx++] = addressesPerPacket;

      //Set number of packets for this trace
      command[cmdIdx++] = packetsPerTrace;
      //setup start and end addresses based upon
      //class variables, bank is used in trace from testing experience
//			command[cmdIdx++] = traceStartAddress >> 16;
      command[cmdIdx++] = emuBank;
      command[cmdIdx++] = traceStartAddress >> 8;
      command[cmdIdx++] = traceStartAddress ;

//			command[cmdIdx++] = traceEndAddress >> 16;
      command[cmdIdx++] = emuBank;
      command[cmdIdx++] = traceEndAddress >> 8;
      command[cmdIdx++] = traceEndAddress ;
      command[cmdIdx] = EOF;
      break;

   default:
      return false;
      break;
   }

   return true;

}
//This will check for an attached device on the open serial port
//and try and fill the version information and serial number
bool Ostrich::checkForDevice(void)
{
   char tmp = 0 ;

   if(!serial.isOpen())
   {
      if(	serial.openCommPort()  &&
            serial.setSpeedAndDataBits(921600,8,'n',1) &&
            serial.setTimeouts(10,0,0,0,0) &&
            serial.applySettings() );
      else
      {
#ifdef DEBUG
         std::cerr << "Open CommPort, setup rate, and apply, failed" << std::endl;
#endif
         return false;
      }
   }
   if(	serial.purgeRX() &&
         buildCommand(versionCommand, 0, 0) &&
         sendCommands() &&
         serial.getByte(&hardwareVersion) &&
         serial.getByte(&firmwareVersion) &&
         serial.getByte(&hardwareVersionCH) )
   {
      //If we got back a byte that matches the hardware type
      //try and get the vendor ID and s/n and verify it's checksum
      if( (hardwareVersion == ostrichHardwareByte || hardwareVersion == ostrichTwoHardwareByte) &&
            hardwareVersionCH == ostrichHardwareCH &&
            getSerialNumAndVendorFromHW() )
         return foundDevice = true;
   }
   else
   {
#ifdef DEBUG
      std::cerr << "failed to communicate with ostrich on initial open" << std::endl;
#endif
      hardwareVersion = firmwareVersion = hardwareVersionCH = 0x00;
   }
   //If it didn't work, drop to 115.2, ask to have the baud brought to 921.6
   //If no OK is received @ 115.2 just bail
   if(!	serial.setSpeedAndDataBits(115200,8,'n',1) &&
         serial.applySettings() &&
         buildCommand(speedCommand, 0, 0) &&
         serial.purgeRX() &&
         sendCommands() &&
         serial.getByte(&tmp) &&
         tmp == dataOK
     )
      return false;


   //if we got a return byte and the device honored our request
   //move back to high speed and get the version again
   if(!	serial.setSpeedAndDataBits(921600,8,'n',1) &&
         serial.applySettings())
      return false;

   if( 	serial.purgeRX() &&
         buildCommand(versionCommand, 0, 0) &&
         sendCommands() &&
         serial.getByte(&hardwareVersion) &&
         serial.getByte(&firmwareVersion) &&
         serial.getByte(&hardwareVersionCH) )
   {
      if( hardwareVersion == ostrichHardwareByte && getSerialNumAndVendorFromHW() )
         return foundDevice = true;
   }

   //If we get here everything was a failure so reset all the values and return an error

   vendorID =  hardwareVersion = firmwareVersion = hardwareVersionCH = 0x00;
   for(int j = 0; j < serialNumberLen; j++)
      serialNumber[j] =0;

   return false;

}
//This attempts to get the serial number and vendor ID back from the hardware
//I don't have hardware to test with so there is no corresponding set for these
bool Ostrich::getSerialNumAndVendorFromHW(void)
{
   char tmp = 0;
   char pchecksum;
   if(	serial.isOpen() &&
         serial.purgeRX() &&
         resetChecksum() &&
         buildCommand(serialNumCommand, 0, 0) &&
         sendCommands()
     )
   {
      //For some reason if the vendor ID and s/n are all 0,
      //the checksum echoed back is the checsum from the command
      //strace cofirms this, but it means the checksum is broXored
      pchecksum = getChecksum();
      resetChecksum();
      serial.getByte(&tmp);
      serial.getBytes(serialNumber, serialNumberLen);
      vendorID=tmp;
      updateChecksum(tmp);
      for(int j = 0; j< serialNumberLen; j++)
         updateChecksum(serialNumber[j]);

      if(serial.getByte(&tmp) && (tmp == getChecksum() || tmp == pchecksum) )
         return foundDevice = true;
   }

   for(int j = 0; j< serialNumberLen; j++)
      serialNumber[j] = 0;

   vendorID = 0;

   return false;
}
Ostrich::Ostrich()
{
   foundDevice = false;
   updateBank = emuBank = persistentBank = wholeEnchilada+1;
   offset = 0;
   //Default block size of 16K
   blockSize = bulkBlockSize * 64;
   foundDevice = false;
   hardwareVersion = firmwareVersion = hardwareVersionCH = vendorID = 0;
   for(int i =0; i < serialNumberLen; i++)
      serialNumber[i] = 0;

   extTraceBuffer = NULL;
   addressesPerPacket = packetsPerTrace = extTraceBufferSize = 0;
   currentBankSize = 0;
   serial.setTimeouts(1000,0,0,0,0);
   serial.applySettings();
}
Ostrich::~Ostrich()
{
   if(trace.is_open())
      trace.close();

   if(file.is_open())
      file.close();
}
