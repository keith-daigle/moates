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
#include "Serial.h"
#include <iostream>

extern int errno;

bool Serial::isOpen(void)
{
   return portIsOpen;
}

bool Serial::setSpeedAndDataBits(int speed, int dbits, int parity, int stop)
{
   baudRate = speed;
   dataBits = dbits;
   parityBits = parity;
   stopBits = stop;
   return true;
}

bool Serial::getSpeedAndDataBits(int * a)
{
   a[0] = baudRate;
   a[1] = dataBits;
   a[2] = parityBits;
   a[3] = stopBits;
   return true;
}

//Read interval timeout is really the only one that's set
//for unix, and it's in .1 s increments
bool Serial::setTimeouts(int interval, int rmult, int rconst, int wmult, int wconst)
{
   readIntervalTimeout = interval;
   readTotalTimeoutMultiplier = rmult;
   readTotalTimeoutConstant = rconst;
   writeTotalTimeoutMultiplier = wmult;
   writeTotalTimeoutConstant = wconst;
   return true;
}

bool Serial::getTimeouts(int * a)
{
   a[0] = readIntervalTimeout;
   a[1] = readTotalTimeoutMultiplier;
   a[2] = readTotalTimeoutConstant;
   a[3] = writeTotalTimeoutMultiplier;
   a[4] = writeTotalTimeoutConstant;
   return true;
}

bool Serial::setPort(std::string s)
{
   port = s;
   return true;
}

std::string Serial::getPort(void)
{
   return port;
}

bool Serial::openCommPort(void)
{
   if(-1 == (fd = open( port.c_str(), O_RDWR|O_NOCTTY)))
   {
      perror(port.c_str());
      return false;
   }
   return portIsOpen = true;
}

#ifdef FREEBSD
bool Serial::purgeRX(void)
{
   int i = TIOCPKT_FLUSHREAD;
   return ( 0 == ioctl(fd, TIOCFLUSH, &i)) ;
}
#else
bool Serial::purgeRX(void)
{
   return ( 0 == tcflush(fd, TCIFLUSH)) ;
}
#endif

#ifdef FREEBSD
bool Serial::purgeTX(void)
{
   int i = TIOCPKT_FLUSHWRITE;
   return ( 0 == ioctl(fd, TIOCFLUSH, &i)) ;
}
#else
bool Serial::purgeTX(void)
{
   return ( 0 == tcflush(fd,TCOFLUSH)) ;
}
#endif

bool Serial::sendByte(char * buf)
{
   bytesWritten = 0;

   if( portIsOpen )
      bytesWritten = write( fd, buf, 1);

   if( bytesWritten == 1)
      return true;

   return false;
}

bool Serial::sendBytes(char * buf, int count)
{
   bytesWritten = 0;

   if( portIsOpen )
   {
      bytesWritten = write( fd, buf, count);

      if(bytesWritten == count)
         return true;

      bytesWritten += write(fd, buf+bytesWritten, count-bytesWritten);

      if( bytesWritten == count)
         return true;
   }
   return false;
}

bool Serial::getByte(char * buf)
{
   bytesRead = 0;

   if( portIsOpen )
      bytesRead = read( fd, buf, 1);

   if( bytesRead == 1)
      return true;

   return false;
}

// this will try to read bytes up to count by reading until read returns 0
bool Serial::getBytes(char * buf, int count)
{
   int tmp = 0;
   if( portIsOpen )
   {
      tmp = bytesRead = read( fd, buf, count);
      //std::cerr << "First attempt at read gave: " << std::dec << tmp;

      while(bytesRead < count && tmp != 0)
      {
         tmp = read(fd, buf+bytesRead, count-bytesRead);
         //	std::cerr << " looped read gave: " << std::dec << tmp << std::endl;
         bytesRead += tmp;
      }
      if(bytesRead == count)
         return true;
   }

   return false;
}

//This will update and apply the termio struct to the port
//if the port is open
bool Serial::applySettings(void)
{
   if (!portIsOpen || termio == NULL)
      return false;

   //Zeros memory out
   memset(termio, 0, sizeof(struct termios) );

   //sets the termio struct up for raw i/o
   cfmakeraw(termio);

   //sets the speed of the connection
   cfsetspeed(termio, baudRate);

   //disable flow control
   termio -> c_iflag &= ~(IXON|IXOFF);

   //setup data bits member
   if( dataBits >=5 && dataBits <= 8)
   {
      termio -> c_cflag &= ~(CS8|CS7|CS6|CS5);
      if(dataBits == 5)
         termio -> c_cflag |= CS5;

      else if(dataBits == 6)
         termio -> c_cflag |= CS6;

      else if(dataBits == 7)
         termio -> c_cflag |= CS7;

      else
         termio -> c_cflag |= CS8;
   }
   else
      return false;

   //setup parity bits member
   if( parityBits == 'n' || parityBits == 'e' || parityBits == 'o' )
   {
      termio -> c_cflag &= ~(PARENB|PARODD);

      //enable parity
      if(parityBits == 'e')
      {
         termio -> c_cflag |= PARENB;
         termio -> c_iflag |= INPCK;
         termio -> c_iflag &= ~(IGNPAR);
      }

      //enable and set for odd
      else if(parityBits == 'o')
      {
         termio -> c_cflag |= (PARENB|PARODD);
         termio -> c_iflag |= INPCK;
         termio -> c_iflag &= ~(IGNPAR);
      }

   }
   else
      return false;

   //setup stop bits
   if( stopBits == 2 || stopBits == 1)
   {
      if(stopBits == 2)
         termio -> c_cflag |= CSTOPB;
   }
   else
      return false;

   termio -> c_cflag |= (CREAD|CLOCAL);
   termio -> c_cc[VMIN] = 0;
   termio -> c_cc[VTIME] = readIntervalTimeout;

   //This will return true if ioctl returns something other
   //than -1, false if it's -1, should probably setup something
   //to check errno and perror in future
   //Should look into http://linux.die.net/man/3/termios
   //and see if we can ditch the #defines
#ifdef FREEBSD
   if (-1 == ioctl(fd, TIOCSETA, termio))
#else
   if (-1 == ioctl(fd, TCSETS, termio))
#endif
   {
      perror("Error with ioctl: ");
      return false;
   }

   return true;

}

//The i/o buffers are hard coded as 1 page in linux kernel based upon
//some googling I did, so these have no effect, but they're set and returned
bool Serial::setRXBufferSize(int i )
{
   rxBufferSize = i;
   return true;
}
int Serial::getRXBufferSize(void)
{
   return rxBufferSize;
}
bool Serial::setTXBufferSize(int i )
{
   txBufferSize = i;
   return true;
}
int Serial::getTXBufferSize(void)
{
   return txBufferSize;
}

Serial::Serial(void)
{
   termio = (termios *)malloc(sizeof(struct termios) );
   fd = 0;

   readIntervalTimeout = 1;
   readTotalTimeoutMultiplier = 0;
   readTotalTimeoutConstant = 0;
   writeTotalTimeoutMultiplier = 0;
   writeTotalTimeoutConstant = 0;

   baudRate = 9600;
   dataBits = 8;
   parityBits = 'n';
   stopBits = 1;

   rxBufferSize = txBufferSize = getpagesize();
}
Serial::~Serial(void)
{
   close(fd);
   free(termio);
}
