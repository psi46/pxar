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

/* For more info and how to use this library, visit: http://www.teuniz.net/RS-232/ */

#include "rs232.h"
#include "log.h"

using namespace pxar;
using namespace std;

int port;
int rs_error;

struct termios new_port_settings,
               old_port_settings;

int RS232_OpenComport(const char* portName, int baudrate)
{
  int baudr, status;

  switch(baudrate)
    {
    case      50 : baudr = B50;
      break;
    case      75 : baudr = B75;
      break;
    case     110 : baudr = B110;
      break;
    case     134 : baudr = B134;
      break;
    case     150 : baudr = B150;
      break;
    case     200 : baudr = B200;
      break;
    case     300 : baudr = B300;
      break;
    case     600 : baudr = B600;
      break;
    case    1200 : baudr = B1200;
      break;
    case    1800 : baudr = B1800;
      break;
    case    2400 : baudr = B2400;
      break;
    case    4800 : baudr = B4800;
      break;
    case    9600 : baudr = B9600;
      break;
    case   19200 : baudr = B19200;
      break;
    case   38400 : baudr = B38400;
      break;
    case   57600 : baudr = B57600;
      break;
    case  115200 : baudr = B115200;
      break;
    case  230400 : baudr = B230400;
      break;
    default      : LOG(logCRITICAL) << "Invalid baud rate " << baudrate << "!";
      return(1);
      break;
    }


  printf("Opening com port: %s\n", portName);
  rs_error = 0;
  port = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
  if(port==-1)
  {
    perror("unable to open comport ");
    return(1);
  }

  rs_error = tcgetattr(port, &old_port_settings);
  if(rs_error==-1)
  {
    close(port);
    perror("unable to read portsettings ");
    return(1);
  }
  
  memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */

  new_port_settings.c_cflag = CS8 | CLOCAL | CREAD;
  new_port_settings.c_iflag = IGNPAR;
  //new_port_settings.c_lflag = ECHO; //TODO: See how this jives with echo readback below

  cfsetspeed(&new_port_settings,baudr);
  
  rs_error = tcsetattr(port, TCSANOW, &new_port_settings);
  if(rs_error==-1)
  {
    close(port);
    perror("unable to adjust portsettings ");
    return(1);
  }

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
    return(1);
  }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
    return(1);
  }

  return(0);
}

int RS232_PollComport(char *buf, int size)
{
  return read(port, buf, size);
}


int RS232_SendByte(const char byte)
{
  return write(port, &byte, 1);
}


int RS232_SendBuf(const char *buf, int size)
{
  return write(port, buf, size);
}

void RS232_CloseComport()
{
  int status;

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }

  tcsetattr(port, TCSANOW, &old_port_settings);
  close(port);
}

/*
  Constant  Description
  TIOCM_LE  DSR (data set ready/line enable)
  TIOCM_DTR DTR (data terminal ready)
  TIOCM_RTS RTS (request to send)
  TIOCM_ST  Secondary TXD (transmit)
  TIOCM_SR  Secondary RXD (receive)
  TIOCM_CTS CTS (clear to send)
  TIOCM_CAR DCD (data carrier detect)
  TIOCM_CD  Synonym for TIOCM_CAR
  TIOCM_RNG RNG (ring)
  TIOCM_RI  Synonym for TIOCM_RNG
  TIOCM_DSR DSR (data set ready)

  http://linux.die.net/man/4/tty_ioctl
*/

int RS232_IsDCDEnabled()
{
  int status;

  ioctl(port, TIOCMGET, &status);

  if(status&TIOCM_CAR) return(1);
  else return(0);
}

int RS232_IsCTSEnabled()
{
  int status;

  ioctl(port, TIOCMGET, &status);

  if(status&TIOCM_CTS) return(1);
  else return(0);
}

int RS232_IsDSREnabled()
{
  int status;

  ioctl(port, TIOCMGET, &status);

  if(status&TIOCM_DSR) return(1);
  else return(0);
}

void RS232_enableDTR()
{
  int status;

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_DTR;    /* turn on DTR */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_disableDTR()
{
  int status;

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_enableRTS()
{
  int status;

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

void RS232_disableRTS()
{
  int status;

  if(ioctl(port, TIOCMGET, &status) == -1)
  {
    perror("unable to get portstatus");
  }

  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(port, TIOCMSET, &status) == -1)
  {
    perror("unable to set portstatus");
  }
}

bool stringEndsWith(string &data, string &suffix){
  int dataSize = data.size();
  int suffixSize = suffix.size();
  if (dataSize < suffixSize)
    return false;
  else
    return data.substr(dataSize-suffixSize,suffixSize) == suffix;
}

////////////////////////////////////////
//External Interface
////////////////////////////////////////

int openComPort(string name, int baud)
{
  if(RS232_OpenComport(name.c_str(), baud))
  {
    LOG(logCRITICAL) << "Cannot open COM port!";
    return 0;
  }
  return 1;
}

void closeComPort()
{
  RS232_CloseComport();
}

int writeData(string data)
{
  int len = data.size();
  string suffix("\r\n");
  if (len < 2 || !stringEndsWith(data, suffix)) {
    data += suffix;
  }
  int status = RS232_SendBuf(data.c_str(), data.size());
  return (status == -1) ? 0 : 1;
}

int readData(string &data, string endToken)
{
  unsigned int tokenSize = endToken.size();
  unsigned int dataSize = data.size();
  unsigned int readSize = 0;
  while (data.size() < tokenSize || !stringEndsWith(data,endToken))
  {
    char buf;
    int status = RS232_PollComport(&buf, 1);
    if(status == 1) {
      data.append(buf,1);
      readSize++;
    }
    dataSize = data.size();
  }
  data = data.substr(0,dataSize-tokenSize);
  return readSize;
}

int writeReadBack(string dataOut, string &dataIn, string endToken){
  if(!writeData(dataOut)){
    return 0;
  }
  return readData(dataIn, endToken);
}
