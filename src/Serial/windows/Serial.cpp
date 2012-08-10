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
 * Serial class
 * Class implementation for serial port encapsulation
 * This is win32 based implementation
 * 
 *
 */

#include "Serial.h"

bool Serial::setSpeedAndDataBits(int baud, int data, int parity, int stop)
{
	baudRate = baud;
	dataBits = data;
	parityBits = parity;
	stopBits = stop;
	return true;
}

//This expects an array of 4 ints to copy data into
bool Serial::getSpeedAndDataBits(int * a)
{
	a[0] = baudRate;
	a[1] = dataBits;
	a[2] = parityBits;
	a[3] = stopBits;
	return true;
}

bool Serial::setTimeouts(int readInterval, int readMultiplier, int readConstant, int writeMultiplier, int writeConstant)
{
	readIntervalTimeout = readInterval;
	readTotalTimeoutMultiplier = readMultiplier;
	readTotalTimeoutConstant = readConstant;
	writeTotalTimeoutMultiplier = writeMultiplier;
	writeTotalTimeoutConstant =  writeConstant;
	return true;
}

//This expects an array of 5 ints to copy the timeouts into
bool Serial::getTimeouts(int * a)
{
	a[0] = readIntervalTimeout;
	a[1] = readTotalTimeoutMultiplier;
	a[2] = readTotalTimeoutConstant;
	a[3] = writeTotalTimeoutMultiplier;
	a[4] = writeTotalTimeoutConstant;
	return true;
}

bool Serial::setPort(std::string s) {port = s; return true;}

std::string Serial::getPort(void) {return port;}

bool Serial::openCommPort(void)
{
	//The additional BS below is needed for ports above 9
	std::string prepend(  "\\\\.\\" + port );

	//Close a port if it's already open
	if(portIsOpen)
	{
		CloseHandle(commHandle);
		commHandle = INVALID_HANDLE_VALUE;
		portIsOpen = false;
	}

 	commHandle = CreateFile( prepend.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
					
	if (commHandle == INVALID_HANDLE_VALUE)
		return false;

 	if( ! SetupComm(commHandle, rxBufferSize,txBufferSize) )
		return false;

	return applySettings();
}

bool Serial::purgeRX(void) { return PurgeComm(commHandle, PURGE_RXCLEAR); }

bool Serial::purgeTX(void) { return PurgeComm(commHandle, PURGE_TXCLEAR); }

bool Serial::sendByte(char * cp)
{
	if(portIsOpen)
	{
		WriteFile(commHandle, cp , 1, &bytesWritten, overlap) ;
		return (bytesWritten == 1);
	}
	return false;
}
/*
bool Serial::sendBytes(char * cp, int num )
{
	if(portIsOpen)
	{
		WriteFile(commHandle, cp, num, &bytesWritten, overlap);
		return (bytesWritten == num);
	}
	return false;
}
*/
bool Serial::sendBytes(char * cp, int num )
{
	int tmp = 0;
	if(portIsOpen)
	{
		WriteFile(commHandle, cp, num, &bytesWritten, overlap);
		return (bytesWritten == num);
	}
	return false;
}
bool Serial::getByte(char * cp)
{ 
	if(portIsOpen)
	{
		ReadFile(commHandle, cp, 1, &bytesRead, overlap) ; 
		return (bytesRead == 1);
	}
	return false;
}
/*
bool Serial::getBytes(char * cp, int num )
{
	if(portIsOpen)
	{
		ReadFile(commHandle, cp, num, &bytesRead, overlap) ;
		return (bytesRead == num);
	}
	return false;
}
*/
bool Serial::applySettings(void)
{
	if(	portIsOpen &&
		makeDCB() &&
		applyDCB() &&
		makeTimeouts() &&
		applyTimeouts() 
	  }
		return true;

	return false;
}

bool Serial::makeTimeouts(void)
{
	if(timeouts != NULL)
		if(!deleteTimeouts())
			return false;	
	
	timeouts = new COMMTIMEOUTS;

	timeouts -> ReadIntervalTimeout = readIntervalTimeout;
	timeouts -> ReadTotalTimeoutMultiplier = readTotalTimeoutMultiplier;
	timeouts -> ReadTotalTimeoutConstant = readTotalTimeoutConstant;
	timeouts -> WriteTotalTimeoutMultiplier = writeTotalTimeoutMultiplier;
	timeouts -> WriteTotalTimeoutConstant = writeTotalTimeoutConstant;

	return true;
}

bool Serial::deleteTimeouts(void)
{
	if(timeouts != NULL)
	{
		delete timeouts;	
		timeouts = NULL;
	}
	return true;
}

bool Serial::applyTimeouts(void) { return SetCommTimeouts(commHandle, timeouts); }

bool Serial::makeDCB(void)
{
	char dcbstr[20];

	if(!portIsOpen)
		return false;

	if(dcb != NULL)
		if(!deleteDCB())
			return false;	
	
	dcb = new DCB;
	FillMemory((void *) dcb, sizeof(*dcb),0);

	_snprintf(dcbstr,19,"%d,%c,%d,%d",baudRate,tolower(parityBits), dataBits, stopBits);
	if(!BuildCommDCB(dcbstr, dcb))
		return false;

	dcb -> DCBlength = sizeof(*dcb);
	dcb -> fOutxCtsFlow = FALSE;
	dcb -> fOutxDsrFlow = FALSE;
	dcb -> fOutX = FALSE;
	dcb -> fInX = FALSE;

	return true;			
}
bool Serial::applyDCB(void) { return SetCommState(commHandle, dcb) ; }

bool Serial::deleteDCB(void)
{
	if(dcb != NULL)
	{
		delete dcb;
		dcb = NULL;
	}
	return true;
}

//If buffer sizes are changed the openPort call needs to be made again
//They are only set upon opening the port
bool Serial::setRXBufferSize(int i) {rxBufferSize = i; return true;}

int Serial::getRXBufferSize(void) {return rxBufferSize;}

bool Serial::setTXBufferSize(int i) {txBufferSize = i; return true;}

int Serial::getTXBufferSize(void) {return txBufferSize;}

//This is the default constructor, the timeouts being 0 cause blocking i/o on serial
//port is com1, speed is set to 9600,8,n,1, and buffers are setup for defaults
Serial::Serial()
{
	portIsOpen = false;
	commHandle = INVALID_HANDLE_VALUE;
	dcb = NULL;
	timeouts = NULL;
	overlap = NULL;

	readIntervalTimeout = 0 ;
	readTotalTimeoutMultiplier = 0;
	readTotalTimeoutConstant = 0;
	writeTotalTimeoutMultiplier = 0;
	writeTotalTimeoutConstant = 0;

	baudRate = 9600;
	dataBits = 8;
	parityBits = 'n';
	stopBits = 1;
	port = "com1";

	rxBufferSize=8192;
	txBufferSize=8192;
}
Serial::~Serial()
{
	deleteDCB();
	deleteTimeouts();
	CloseHandle(commHandle);
	commHandle = INVALID_HANDLE_VALUE;
	portIsOpen = false;
}

