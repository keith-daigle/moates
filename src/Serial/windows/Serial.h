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
 * Class to encapsulate serial port for use in 
 * comms with moates hardware and ecu
 * Intended to hide win32's 
 * archaic settings and structs from main programs
 *
 */
#include "Windows.h"
#include <string>
class Serial{


public:
	bool setSpeedAndDataBits(int, int, int, int);
	bool getSpeedAndDataBits(int *);
	bool setTimeouts(int, int, int, int, int);
	bool getTimeouts(int *);
	bool setPort(std::string);
	std::string getPort(void);
	bool openCommPort(void);
	bool purgeRX(void);
	bool purgeTX(void);
	bool sendByte(char *);
	bool sendBytes(char *, int);
	bool getByte(char *);
	bool getBytes(char *, int);
	bool applySettings(void);
	bool setRXBufferSize(int);
	int getRXBufferSize(void);
	bool setTXBufferSize(int);
	int getTXBufferSize(void);
	Serial(void);
	~Serial(void);

private:

	bool makeDCB(void);
	bool deleteDCB(void);
	bool applyDCB(void);
	bool makeTimeouts(void);
	bool deleteTimeouts(void);
	bool applyTimeouts(void);
	bool portIsOpen;
	int baudRate;
	int dataBits;
	int parityBits;
	int stopBits;
	int readIntervalTimeout;
	int readTotalTimeoutMultiplier;
	int readTotalTimeoutConstant;
	int writeTotalTimeoutMultiplier;
	int writeTotalTimeoutConstant;
	int rxBufferSize;
	int txBufferSize;
	std::string port;
	//Windows specific nastiness below here
	DWORD bytesRead;
	DWORD bytesWritten;
	DCB * dcb;
	COMMTIMEOUTS * timeouts;
	OVERLAPPED * overlap;
	HANDLE commHandle;
};
