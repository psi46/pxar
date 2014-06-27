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
#define NUMPORTS 31

int Cport[NUMPORTS],
  rs_error;

struct termios new_port_settings,
  old_port_settings[NUMPORTS];

char comports[NUMPORTS][30]={"/dev/ttyS0","/dev/ttyS1","/dev/ttyS2","/dev/ttyS3","/dev/ttyS4","/dev/ttyS5",
                       "/dev/ttyS6","/dev/ttyS7","/dev/ttyS8","/dev/ttyS9","/dev/ttyS10","/dev/ttyS11",
                       "/dev/ttyS12","/dev/ttyS13","/dev/ttyS14","/dev/ttyS15","/dev/ttyUSB0",
                       "/dev/ttyUSB1","/dev/ttyUSB2","/dev/ttyUSB3","/dev/ttyUSB4","/dev/ttyUSB5",
                       "/dev/ttyAMA0","/dev/ttyAMA1","/dev/ttyACM0","/dev/ttyACM1",
                       "/dev/rfcomm0","/dev/rfcomm1","/dev/ircomm0","/dev/ircomm1","/dev/tty.KeySerial1"};

int comport_number=16; // "/dev/ttyUSB0"

int RS232_OpenComport(int comport_number, int baudrate)
{
  int baudr, status;

  if((comport_number>=NUMPORTS)||(comport_number<0))
    {
      LOG(logCRITICAL) << "Illegal comport number " << comport_number << "!";
      return(1);
    }

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

  rs_error = 0;
  Cport[comport_number] = open(comports[comport_number], O_RDWR | O_NOCTTY | O_NDELAY);
  if(Cport[comport_number]==-1)
    {
      perror("unable to open comport ");
      return(1);
    }

  rs_error = tcgetattr(Cport[comport_number], old_port_settings + comport_number);
  if(rs_error==-1)
    {
      close(Cport[comport_number]);
      perror("unable to read portsettings ");
      return(1);
    }

  memset(&new_port_settings, 0, sizeof(new_port_settings));  /* clear the new struct */

  new_port_settings.c_cflag = baudr | CS8 | CLOCAL | CREAD;
  new_port_settings.c_iflag = IGNPAR;
  new_port_settings.c_oflag = 0;
  new_port_settings.c_lflag = FLUSHO;
  new_port_settings.c_cc[VMIN] = 0;      /* block untill n bytes are received */
  new_port_settings.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */
  rs_error = tcsetattr(Cport[comport_number], TCSANOW, &new_port_settings);
  if(rs_error==-1)
    {
      close(Cport[comport_number]);
      perror("unable to adjust portsettings ");
      return(1);
    }

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
      return(1);
    }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
      return(1);
    }

  return(0);
}


int RS232_PollComport(int comport_number, char *buf, int size)
{
  int n;

  n = read(Cport[comport_number], buf, size);

  return(n);
}


int RS232_SendByte(int comport_number, unsigned char byte)
{
  int n;

  n = write(Cport[comport_number], &byte, 1);
  if(n<0)  return(1);

  return(0);
}


int RS232_SendBuf(int comport_number, unsigned char *buf, int size)
{
  return(write(Cport[comport_number], buf, size));
}

// Need this function to talk to the Keithlez 2410
int RS232_SendBufString(int comport_number, char *buf, int size)
{
  return(write(Cport[comport_number], buf, size));
}


void RS232_CloseComport(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
    }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
    }

  tcsetattr(Cport[comport_number], TCSANOW, old_port_settings + comport_number);
  close(Cport[comport_number]);
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

int RS232_IsDCDEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CAR) return(1);
  else return(0);
}

int RS232_IsCTSEnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_CTS) return(1);
  else return(0);
}

int RS232_IsDSREnabled(int comport_number)
{
  int status;

  ioctl(Cport[comport_number], TIOCMGET, &status);

  if(status&TIOCM_DSR) return(1);
  else return(0);
}

void RS232_enableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
    }

  status |= TIOCM_DTR;    /* turn on DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
    }
}

void RS232_disableDTR(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
    }

  status &= ~TIOCM_DTR;    /* turn off DTR */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
    }
}

void RS232_enableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
    }

  status |= TIOCM_RTS;    /* turn on RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
    }
}

void RS232_disableRTS(int comport_number)
{
  int status;

  if(ioctl(Cport[comport_number], TIOCMGET, &status) == -1)
    {
      perror("unable to get portstatus");
    }

  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(Cport[comport_number], TIOCMSET, &status) == -1)
    {
      perror("unable to set portstatus");
    }
}


void RS232_cputs(int comport_number, const char *text)  /* sends a string to serial port */
{
  while(*text != 0)   RS232_SendByte(comport_number, *(text++));
}

//---------------------------------------------------------------------------
int openComPort(const int comPortNumber,const int baud)
{
  comport_number=comPortNumber;
    
  if(RS232_OpenComport(comport_number, baud))
    {
      LOG(logCRITICAL) << "Cannot open COM port!";
      return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
void closeComPort()
{
  RS232_CloseComport(comport_number);
}

//---------------------------------------------------------------------------
int writeCommand(const char *command)
{
  char cmd[256];
  char buf[10] = { 0 };
  int  timeout;
  int  rb;
  int  len;

  strncpy(cmd, command, 250);
  cmd[250] = 0;

  // terminate command with CR LF
  if (!strstr(cmd, "\r\n")) {
    strcat(cmd, "\r\n");
  }

  len = strlen(cmd);

  int i;
  for (i = 0; i < len; i++) {
    RS232_SendByte(comport_number, cmd[i]);

    // read echo
    timeout = 10;
    do {
      rb = RS232_PollComport(comport_number, buf, 1);
      usleep(10000);
      timeout--;
    } while ( (rb == 0) && (timeout > 0) );

    if (cmd[i] != buf[0]) {
      return 0;
    }
  }

  usleep(20000);

  return 1;
}

//---------------------------------------------------------------------------
// Allows writing commands to the Keithley 2410
int writeCommandString(const char *command) {
  int  len;
  char cmd[256];  

  strncpy(cmd, command, 250);
  cmd[250] = 0;

  // terminate command with CR+ LF
  if (!strstr(cmd, "\r\n")) {
    strcat(cmd, "\r\n");
  }

  len = strlen(cmd);
  RS232_SendBufString(comport_number, cmd, len);

  usleep(20000);

  return 1;
}


//---------------------------------------------------------------------------
int writeCommandAndReadAnswer(const char *command, char *answer) {
  LOG(logDEBUGRPC) << "RS232 Command: " << command;
        
  int  bytesRead;
  char inbuf[256];
  int  to = 0;
  char *p;

  if (!answer) {
    return 0;
  }

  // write command to HV device
  if (!writeCommand(command)) {
    return 0;
  }

  // init buffer
  *answer = 0;

  // read answer (terminated with CR + LF)
  to = 0;
  do {
    bytesRead = RS232_PollComport(comport_number, inbuf, sizeof(inbuf));

    if (bytesRead > 0) {
      inbuf[bytesRead] = 0;
      strcat(answer, inbuf);
      to = 0;
    }
    usleep(10000);
  } while ( (strstr(answer, "\r\n") == 0) && (++to < 80) ); // wait for CR+LF or timeout

  //         // clear trailing CR + LF
  if ( (p  = strstr(answer, "\r\n")) ) {
    *p = 0;
  }

  usleep(100000);  
  LOG(logDEBUGRPC) << "RS232 Answer: " << answer;
  return 1;
}

//---------------------------------------------------------------------------
// Allows writing commands to the Keithley 2410
int writeCommandStringAndReadAnswer(const char *command, char *answer, int delay) {
  LOG(logDEBUGRPC) << "RS232 Command: " << command;

  int  bytesRead;
  char inbuf[256];
  int  to = 0;
  char *p;

  if (!answer) {
    return 0;
  }

  // write command to HV device
  if (!writeCommandString(command)) {  
    return 0;
  }
   
  usleep(delay*1000000);

  // init buffer
  *answer = 0;   

  // read answer (terminated with CR)
  to = 0;
  do {
    bytesRead = RS232_PollComport(comport_number, inbuf, sizeof(inbuf));
  
    if (bytesRead > 0) {
      inbuf[bytesRead] = 0;
      strcat(answer, inbuf);
      to = 0;
    }
    usleep(10000);
  } while ( (strstr(answer, "\r\n") == 0) && (++to < 80) ); // wait for CR or t$
  
  //         // clear trailing CR
  if ( (p  = strstr(answer, "\r\n")) ) {
    *p = 0;
  }

  usleep(100000);
  LOG(logDEBUGRPC) << "RS232 Answer: " << answer;
  return 1;
}


