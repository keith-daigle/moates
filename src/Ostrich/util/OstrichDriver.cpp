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
 * Test class for Ostrich device
 * Creates object, makes a few calls
 * shovels some data out the serial port
 *
 */
#include "Ostrich.h"

#include <iostream>
using namespace std;
int main(int argc, char * argv[])
{
	Ostrich emu;
	char sn[emu.serialNumberLen];
	int bank;

	if(argc != 4)
	{
		cerr<< "Usage: " << argv[0] << " <Serial device> <binary output file> <binary input file> " << endl;
		return false;
	}

	std::string pname(argv[1]);
	std::string ofname(argv[2]);
	std::string ifname(argv[3]);

	cerr << "setComPort(): ";
	if(emu.setComPort(pname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "checkForDevice(): ";
	if(emu.checkForDevice())
	{
		cerr << "OK" << endl;
		cerr << "Hardware version: 0x" << hex << static_cast<unsigned int>(emu.getHardwareVersion()) << endl;
		cerr << "Frimware version: 0x" << hex << static_cast<unsigned int>(emu.getFirmwareVersion()) << endl;
		cerr << "Hardware version char: " <<  emu.getHardwareVersionCH() << endl;
		cerr << "Vendor ID: 0x" <<  hex << static_cast<unsigned int>(emu.getVendorID()) << endl;
		if(emu.getSerialNumber(sn))
		{
			cerr << "SerialNumber: " ;
			for(int i = 0; i < emu.serialNumberLen; i++)
				cerr << " 0x" << hex << static_cast<int>(sn[i]);
			cerr << endl;
		}
		else
		{
			cerr << "getSerialNumber failed" << endl;
			return false;
		}
	}
	else
		cerr << "NOK" << endl;

	cerr << "get persistent bank: ";
	if((bank = emu.getBank('P')) < emu.wholeEnchilada+1)
		cerr << "OK " << endl << "current persistent bank is: "<< dec<< bank << endl;
	else
		cerr << "NOK - value is: "<< dec<< bank << endl;

	cerr << "get emulation bank: ";
	if((bank = emu.getBank('E')) < emu.wholeEnchilada+1)
		cerr << "OK " << endl << "current emulation bank is: "<< dec<< bank << endl;
	else
		cerr << "NOK - value is: "<< dec<< bank << endl;

	cerr << "get update bank: ";
	if((bank = emu.getBank('U')) < emu.wholeEnchilada+1)
		cerr << "OK " << endl << "current read/write bank is: "<< bank << endl;
	else
		cerr << "NOK - value is: "<< bank << endl;

	cerr << "setBlockSize(): ";
	if(emu.setBlockSize(16384))
		cerr << "OK - value is: "<< emu.getBlockSize() << endl;
	else
		cerr << "NOK - value is: "<< emu.getBlockSize() << endl;
/*
	cerr << "readBankToMemory(): ";
	if(emu.readBankToMemory())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "output setBinFile(): ";
	if(emu.setBinFile(ofname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "output writeMemoryToFile(): ";
	if(emu.writeMemoryToFile())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "set update bank: ";
	if(emu.setBank(1, 'U') < emu.wholeEnchilada+1)
		cerr << "OK " << endl ;
	else
		cerr << "NOK " << endl;

	cerr << "get update bank: ";
	if((bank = emu.getBank('U')) < emu.wholeEnchilada+1)
		cerr << "OK " << endl << "current read/write bank is: "<< bank << endl;
	else
		cerr << "NOK - value is: "<< bank << endl;
*/
	cerr << "input setBinFile(): ";
	if(emu.setBinFile(ifname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "calculateOffset(): ";
	if(emu.calculateOffset())
		cerr << "OK - offset is: " << hex << emu.getOffset() << endl;
	else
		cerr << "NOK" << endl;

	cerr << "input readFileToMemory(): ";
	if(emu.readFileToMemory())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "writeMemoryToBank(): ";
	if(emu.writeMemoryToBank())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "verifyBankToFile(): ";
	if(emu.verifyBankToFile())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	return true;
}

