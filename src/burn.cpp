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
#include "Burn.h"
#include <ctype.h>
#include <iostream>
using namespace std;

enum Action { NOTHING, ERASE, WRITE, READ, VERIFY, BLANKCHECK, HWCHECK };

static string usage =
   "Moates Burn1/2 command line interface\n"
   "\n"
   "Command can be used in one of the following fashions:\n"
   "moatesburn -p <com port> -h                     - Test for Burn1/2 hardware on port supplied in <com port>\n"
   "moatesburn -p <com port> -t <type> -e           - Erase and blank check chip of <type> on Burn1/2 attached to <com port>\n"
   "moatesburn -p <com port> -t <type> -b           - Blank check chip of <type> on Burn1/2 attached to <com port>\n"
   "moatesburn -p <com port> -t <type> -w <file>    - Write and verify <file> to chip of <type> on Burn1/2 attached to <com port>\n"
   "moatesburn -p <com port> -t <type> -r <file>    - Read chip of <type> on Burn1/2 attached to <com port> to <file>\n"
   "moatesburn -p <com port> -t <type> -v <file>    - Verify chip of <type> on Burn1/2 to <file>\n"
   "\n"
   "Examples:\n"
   "moatesburn -p /dev/ttyUSB0 -t SST27SF512 -e        -- Erase a SST 27sf512 on the burner located at /dev/ttyUSB0\n"
   "moatesburn -p /dev/ttyUSB0 -t M2732A -r 2732.bin   -- Read a M2732 to the file 2732.bin from burner at /dev/ttyUSB0\n"
   "moatesburn -p /dev/ttyUSB0 -h                      -- Check for hardware attached to ttyUSB0 - implied in other commands\n"
   "\n"
   "Known chip types and supported commands for each type:\n"
   "SST27SF512  - SST 27sf512     - Read/Write/Erase/Verify/Blank check\n"
   "AM29F040    - AMD 29f040      - Read/Write/Erase/Verify/Blank check\n"
   "EECIV       - Moates EEC 4    - Read/Write/Erase/Verify/Blank check\n"
   "AT29C256    - ATMEL 29c256    - Read/Write/Verify/Blank check\n"
   "M2732A      - Microchip 2732  - Read/Verify/Blank check\n"
   "\n"
   "\n" ;

int main (int argc, char **argv)
{

   ChipType chip = NONE;
   Burn MoatesBurn;
   Action cmd = NOTHING;
   string port;
   string file;
   string chipname;
   int index;
   int c;

   opterr = 0;

   while ((c = getopt (argc, argv, "p:t:w:r:v:ehb")) != -1)
      switch (c)
      {
      case 'p':
         port.assign(optarg);
         break;
      case 'w':
         cmd = WRITE;
         file.assign(optarg);
         break;
      case 'r':
         cmd = READ;
         file.assign(optarg);
         break;
      case 'v':
         cmd = VERIFY;
         file.assign(optarg);
         break;
      case 'e':
         cmd = ERASE;
         break;
      case 'b':
         cmd = BLANKCHECK;
         break;
      case 'h':
         cmd = HWCHECK;
         break;
      case 't':
         chipname.assign(optarg);
         if( chipname == "SST27SF512" )
         {
            chip = SST27SF512 ;
         }
         else if( chipname == "AM29F040" )
         {
            chip = AM29F040;
         }
         else if( chipname == "EECIV" )
         {
            chip = EECIV;
         }
         else if( chipname == "AT29C256" )
         {
            chip = AT29C256;
         }
         else if( chipname == "M2732A" )
         {
            chip = M2732A;
         }
         else
         {
            cerr<< "ERROR: Chip type: " << chipname << " unknown" << endl;
            cerr << usage;
            return false;
         }
         break;
      case '?':
         if (optopt != 'e'  && optopt != 'h' && optopt != 'b')
         {
            cerr << "ERROR: Option -"<< (char) optopt << "requires an argument" << endl;
            cerr<< usage;
            return false;
         }
         else if (isprint(optopt))
         {
            cerr << "ERROR: Unknown option -" <<	(char) optopt << endl;
            cerr << usage;
            return false;
         }
         else
         {
            cerr << "ERROR: Unknown option character " << hex << optopt << endl;
            cerr << usage;
            return false;
         }
      default:
      {
         cerr << usage;
         return false;
      }
      }

   if(port.empty())
   {
      cerr << "ERROR: Com port must be provided." << endl << usage;
      return false;
   }
   if(cmd != HWCHECK && chip == NONE)
   {
      cerr << "ERROR: Chip type must be provided for functions other than hardware check" << endl << usage;
      return false;
   }
   if((cmd == READ || cmd == WRITE || cmd == VERIFY) && file.empty())
   {
      cerr << "ERROR: Filename must be provided for Read/Write/Verify functions" << endl << usage;
      return false;
   }

   if( !MoatesBurn.setComPort(port) )
   {
      cerr << "ERROR: couldn't open com port " << port << endl;
      return false;
   }
   else
      cout << "Opened com port: " << port << " OK" << endl;

   if( ! MoatesBurn.checkForDevice() )
   {
      cerr << "ERROR: device not found" << endl;
      return false;
   }
   else
   {
      cout << "Found device! Version is: "
           << (int) MoatesBurn.getHardwareVersion() << "."
           << (int) MoatesBurn.getFirmwareVersion() << "."
           <<  MoatesBurn.getHardwareVersionCH()
           << endl;
      if( cmd == HWCHECK)
         return true;
   }
   switch(cmd)
   {
   case ERASE:
      if(chip ==SST27SF512 || chip == AM29F040 || chip == EECIV)
      {
         cout << "Erasing " << chipname  <<  ".... " << flush;
         if(	MoatesBurn.setChipType(chip) &&
               MoatesBurn.eraseChip() &&
               MoatesBurn.verifyChipIsBlank())
         {
            cout << " Success!" << endl;
            return true;
         }
         else
            cout << " Failed!" << endl;

      }
      else
         cout <<  "Can't erase chip of type: "  <<  chipname << endl;

      return false;
      break;

   case WRITE:
      if(chip ==SST27SF512 || chip == AM29F040 || chip == EECIV || chip == AT29C256)
      {
         cout << "Writing file: " << file << " to chip: "<< chipname  <<  ".... " << flush;
         if(	MoatesBurn.setChipType(chip) &&
               MoatesBurn.setBinFile(file) &&
               MoatesBurn.writeFileToChip())
         {
            cout << " Success!" << endl;
            return true;
         }
         else
            cout << " Failed!" << endl;

      }
      else
         cout<< "Cant write chip of type: " << chipname << endl;

      return false;
      break;

   case READ:
      if(chip ==SST27SF512 || chip == AM29F040 || chip == EECIV || chip == AT29C256 || chip == M2732A )
      {
         cout << "Reading chip: " << chipname << " to file: "<< file  <<  ".... " << flush;
         if(	MoatesBurn.setChipType(chip) &&
               MoatesBurn.setBinFile(file) &&
               MoatesBurn.readChipToMemory() &&
               MoatesBurn.writeMemoryToFile() )
         {
            cout << " Success!" << endl;
            return true;
         }
         else
            cout << " Failed!" << endl;
      }
      else
         cout<< "Can't read from chip of type: " << chipname << endl;

      return false;
      break;

   case VERIFY:
      if(chip ==SST27SF512 || chip == AM29F040 || chip == EECIV || chip == AT29C256 || chip == M2732A )
      {
         cout << "Verifing chip: " << chipname << " to file: "<< file  <<  ".... " << flush;
         if(	MoatesBurn.setChipType(chip) &&
               MoatesBurn.setBinFile(file) &&
               MoatesBurn.verifyChipToFile() )
         {
            cout << " Success!" << endl;
            return true;
         }
         else
            cout << " Failed!" << endl;
      }
      else
         cout<< "Can't verify against chip of type: " << chipname << endl;

      return false;
      break;

   case BLANKCHECK:
      if(chip ==SST27SF512 || chip == AM29F040 || chip == EECIV || chip == AT29C256 || chip == M2732A )
      {
         cout << "Verifing chip: " << chipname << " is blank " <<  ".... " << flush;
         if(	MoatesBurn.setChipType(chip) &&
               MoatesBurn.setBinFile(file) &&
               MoatesBurn.verifyChipIsBlank() )
         {
            cout << " Success!" << endl;
            return true;
         }
         else
            cout << " Failed!" << endl;
      }
      else
         cout<< "Can't blank check chip of type: " << chipname << endl;

      return false;


   }
   return false;
}

