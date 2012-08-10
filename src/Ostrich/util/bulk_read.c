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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <ctype.h>
#include <sys/ioctl.h>
extern int errno;
int main (int argc, const char * argv[]) 
{
	struct termios theTermios;
	int fd, i = 0;
	int j = 0;
	int tioc = 0;
	int bsize = (0x40 * 256);
	unsigned char read_cmd[] = {'Z', 'R', bsize/256 , 0x00, 0x00, 0x00};
	unsigned char checksum =0;
	unsigned char buf[65537];
	int count;

	if(argc < 2)
	{
		printf("Usage: %s <serial port device>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for(checksum=i=0; i< 5; i++)
		checksum += read_cmd[i];
	read_cmd[5] = checksum;


	printf("Read block command set with checksum: ");
	for(i = 0; i < 6; i++)
		printf("%02X ", read_cmd[i]);
	printf("\n");

	printf("Memset termios\n");
	memset(&theTermios, 0, sizeof(struct termios));


	printf("making raw for termios\n");
	cfmakeraw(&theTermios);

	printf("setting speed on the termios\n");
	cfsetspeed(&theTermios, 921600);

	theTermios.c_cflag |= (CREAD | CLOCAL | CS8);
	theTermios.c_cc[VMIN] = 0;
	theTermios.c_cc[VTIME] = 1;

	if(argc != 2)
		return 1;

	printf("Opening serial port\n");
	fd = open(argv[1], O_RDWR|O_NOCTTY);

	if(fd == -1)
	{
	    printf("%s", strcat("Unable to open", argv[1]));
	}

	if(-1 == ioctl(fd, TCSETS, &theTermios))
		perror("Error with ioctl: ");

	if( 0 != tcflush(fd, TCIFLUSH))
		perror("ioctl failed to flush read i/o buffer: ");

	if( 0 != tcflush(fd,TCOFLUSH))
		perror("ioctl failed to flush write i/o buffer: ");
/*
	printf("Setting termio\n");
	if( -1 == ioctl(fd, TIOCSETA, &theTermios))
		printf("ioctl failed to set options\n");

	if(-1 == ioctl(fd, TIOCFLUSH, &tioc))
		perror("ioctl failed to flush both i/o buffers: ");
*/
	printf("sending read block command\n");
	if( 6 != write(fd, read_cmd, 6))
		printf("Write failed\n");

	j=count = read( fd, buf, bsize + 1);
	while (j < bsize+1)
	{
		printf("received bytes: %d requested read of size: %d total read: %d\n", count, bsize+1, j);
		count = read( fd, buf, bsize + 1);
		j+=count;
	}
	printf("final received bytes: %d requested read of size: %d total read: %d\n", count, bsize+1, j);


	return EXIT_SUCCESS;
}
