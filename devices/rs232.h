/*
***************************************************************************
*
* Author: Teunis van Beelen
*
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Teunis van Beelen
*
* teuniz@gmail.com
*
***************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
***************************************************************************
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
***************************************************************************
*/

/* last revision: Januari 31, 2014 */

/* For more info and how to use this libray, visit: http://www.teuniz.net/RS-232/ */

#ifndef rs232_H
#define rs232_H

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

int RS232_OpenComport(const char* portName, int baudrate);
int RS232_PollComport(char *buf, int size);
int RS232_SendByte(char byte);
int RS232_SendBuf(char *buf, int size);
void RS232_CloseComport();

int RS232_IsDCDEnabled();
int RS232_IsCTSEnabled();
int RS232_IsDSREnabled();

void RS232_enableDTR();
void RS232_disableDTR();
void RS232_enableRTS();
void RS232_disableRTS();


#include <string>
using namespace std;

int openComPort(string name, int baud);
void closeComPort();

int writeData(string data);
int readData(string &data, string endToken = "\r\n");
int writeReadBack(string dataOut, string &dataIn, string endToken = "\r\n");
#endif
