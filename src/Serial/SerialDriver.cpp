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
 * Test class for Serial port encapsulation
 * Creates object, makes a few calls
 * shovels some data out the serial port
 *
 */
#include "Serial.h"
#include <iostream>
using namespace std;
int main(int argc, char * argv[])
{
   char newline = '\n';
   char cmd[2] = {'V', 'V'};
   char blockread[] = { '5', 'R' , 0x00, 0x00, 0x00, 0x00 };
   char block[257];
   unsigned char sum;

   int tmp, i, j ;
   char tmpc;
   Serial serialPort;

   if(argc != 3)
   {
      cerr<< "Usage: " << argv[0] << " <Serial device> <string to send>" << endl;
      return false;
   }

   std::string pname(argv[1]);
   std::string command(argv[2]);

   if(serialPort.setPort(pname))
      cerr << "setPort OK" << endl;
   else
      cerr << "setPort NOK" << endl;

   if(serialPort.openCommPort())
      cerr << "openCommPort OK " << endl;
   else
      cerr << "openCommPort NOK: " << pname << endl;

   serialPort.setSpeedAndDataBits(921600,8,'n',1);

   serialPort.setTimeouts(5,50,50,50,50);

   if( serialPort.applySettings())
      cerr << "applySettings OK" << endl;
   else
      cerr << "applySettings NOK" << endl;

   if (!serialPort.sendBytes(( char *) command.c_str(), command.length()))
   {
      cerr << "Error with sending command" << endl;
      return false;
   }
   else
      cerr << "sendBytes OK" << endl;

   while(serialPort.getByte(&tmpc))
      cout << "returned byte is: " << hex << (int) tmpc << endl;

   for( i = 0; i < 256; i++)
   {
      cerr << "reading block: " << i ;
      blockread[3] = i;

      for(sum = j = 0; j < 5; j++)
         sum += blockread[j];

      blockread[5] = sum;

      if(!serialPort.sendBytes(blockread, 6))
         cerr << " send commands for blockread failed" << endl;

      else if(!serialPort.getBytes(block, 257))
         cerr << " read data failed" << endl;

      for(sum = j = 0; j < 256; j++)
         sum += block[j];

      if( sum != block[256] )
         cerr << "checksum invalid " << endl;
      else
         cerr << " OK " << endl;

   }
   return true;
}

