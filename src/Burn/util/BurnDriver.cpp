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
 * Test class for Burn device
 * Creates object, makes a few calls
 * shovels some data out the serial port
 */
#include "Burn.h"

#include <iostream>
using namespace std;
int main(int argc, char * argv[])
{
	Burn one;
	ChipType type = SST27SF512;

	if(argc != 4)
	{
		cerr<< "Usage: " << argv[0] << " <Serial device> <binary output file> <binary input file> " << endl;
		return false;
	}

	std::string pname(argv[1]);
	std::string ofname(argv[2]);
	std::string ifname(argv[3]);

	cerr << "setComPort(): ";
	if(one.setComPort(pname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "setChipType(): ";
	if(one.setChipType(type))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "checkForDevice(): ";
	if(one.checkForDevice())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;
/*
	cerr << "setBlockSize(): ";
	if(one.setBlockSize(108))
		cerr << "OK - value is: "<< one.getBlockSize() << endl;
	else
		cerr << "NOK - value is: "<< one.getBlockSize() << endl;

	cerr << "readChipToMemory(): ";
	if(one.readChipToMemory())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "output setBinFile(): ";
	if(one.setBinFile(ofname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "output writeMemoryToFile(): ";
	if(one.writeMemoryToFile())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "calculateChipOffset(): ";
	if(one.calculateChipOffset())
		cerr << "OK - offset is: " << one.getOffset() << endl;
	else
		cerr << "NOK" << endl;

	cerr << "verifyChipIsBlank(): ";
	if(one.verifyChipIsBlank())
		cerr << "OK - chip is blank" << endl;
	else
		cerr << "NOK - chip not blank" << endl;

	cerr << "input setBinFile(): ";
	if(one.setBinFile(ifname))
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "input readFileToMemory(): ";
	if(one.readFileToMemory())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "writeMemoryToChip(): ";
	if(one.writeMemoryToChip())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;
	cerr << "verifyChipToFile(): ";
	if(one.verifyChipToFile())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;
*/
	cerr << "verifyChipIsBlank(): ";
	if(one.verifyChipIsBlank())
		cerr << "OK - chip is blank" << endl;
	else
		cerr << "NOK - chip not blank" << endl;

	cerr << "eraseChip(): ";
	if(one.eraseChip())
		cerr << "OK" << endl;
	else
		cerr << "NOK" << endl;

	cerr << "verifyChipIsBlank(): ";
	if(one.verifyChipIsBlank())
		cerr << "OK - chip is blank" << endl;
	else
		cerr << "NOK - chip not blank" << endl;

	return true;
}

