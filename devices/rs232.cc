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

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include <cstdio>

#include <ctime>

using namespace pxar;
using namespace std;

#define NULL_FD -1
//#define DEBUG_RS232

RS232Conn::RS232Conn(){
  portName = "";
  baudRate = 0;
  timeout = 1;
  port = NULL_FD;
  terminator = "\r\n";
  readSuffix = "";
}

RS232Conn::~RS232Conn(){
  if(port != NULL_FD){
    closePort();
  }
}

void RS232Conn::setPortName(const string &portName){
  if (port != NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot change port name while port is open";
    return;
  }
  this->portName = portName;
}

void RS232Conn::setBaudRate(int baudRate){
  if (this->port != NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot change baud rate while port is open";
    return;
  }
  this->baudRate = baudRate;
}

void RS232Conn::setFlowControl(bool flowControl){
  if (port != NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot change flow control while port is open";
    return;
  }
  this->flowControl = flowControl;
}

void RS232Conn::setParity(bool parity){
  if (port != NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot change parity while port is open";
    return;
  }
  this->parity = parity;
}

void RS232Conn::setReadSuffix(const std::string &suffix){
  readSuffix = suffix;
}

void RS232Conn::setTerminator(const std::string &term){
  terminator = term;
}

void RS232Conn::setRemoveEcho(bool removeEcho){
  this->removeEcho = removeEcho;
}

void RS232Conn::setTimeout(double timeout){
  this->timeout = timeout;
}

bool RS232Conn::openPort(){
  if(port != NULL_FD){
    LOG(logCRITICAL) << "[RS232] CCannot Open Port.\"" << portName << "\"Port already Open ";
    return false;
  }
  int baudr;
  switch(baudRate)
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
    default      : LOG(logCRITICAL) << "[RS232] Invalid baud rate " << baudRate << "!";
      return(1);
      break;
    }

  LOG(logINFO) << "[RS232] Opening com port: "<< portName<<" with baud: "<<baudRate;
  this->port = open(portName.c_str(), O_RDWR | O_NOCTTY);
  if(port==-1)
  {
    port = NULL_FD;
    LOG(logCRITICAL) << "[RS232] unable to open comport ";
    return false;
  }
  
  int rsError;
  rsError = tcgetattr(port, &oldPortSettings);
  if(rsError==-1){
    closePort();
    LOG(logCRITICAL) << "[RS232] unable to read portsettings ";
    return false;
  }
  
  struct termios newPortSettings;
  memset(&newPortSettings, 0, sizeof(newPortSettings));
  newPortSettings.c_cflag = CS8 | CLOCAL | CREAD;
  if(parity){
    newPortSettings.c_cflag |= PARENB | PARODD;
  } else{
    newPortSettings.c_iflag |= IGNPAR;
  }
  if(flowControl){
    newPortSettings.c_iflag = IXON | IXOFF;
  }
  cfsetspeed(&newPortSettings,baudr);
  
  rsError = tcsetattr(port, TCSANOW, &newPortSettings);
  if(rsError==-1){
    closePort();
    LOG(logCRITICAL) << "[RS232] unable to adjust portsettings ";
    return false;
  }
  
  int status = 0;
  rsError = ioctl(port, TIOCMGET, &status);
  if(rsError == -1){
    closePort();
    LOG(logCRITICAL) << "[RS232] unable to get portstatus";
    return false;
  }

  status |= TIOCM_DTR;    /* turn on DTR */
  status |= TIOCM_RTS;    /* turn on RTS */

  rsError = ioctl(port, TIOCMSET, &status);
  if(rsError == -1){
    closePort();
    LOG(logCRITICAL) << "[RS232] unable to set portstatus";
    return false;
  }

  tcflush(port, TCIOFLUSH); //drop any old data on port

  return true;
}

void RS232Conn::closePort(){
  if(port == NULL_FD) return;
  int status;
  if(ioctl(port, TIOCMGET, &status) == -1){
    LOG(logCRITICAL) << "[RS232] Unable to get portstatus";
  }

  status &= ~TIOCM_DTR;    /* turn off DTR */
  status &= ~TIOCM_RTS;    /* turn off RTS */

  if(ioctl(port, TIOCMSET, &status) == -1){
    LOG(logCRITICAL) << "[RS232] Unable to set portstatus";
  }

  tcsetattr(port, TCSANOW, &oldPortSettings);
  close(port);
  port = NULL_FD;
}


int RS232Conn::writeBuf(const char *buf, int len){
  if (port == NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot write to non-open port";
    return 0;
  }
  int status = write(port, buf, len);
  if (status == -1){
    LOG(logCRITICAL) << "[RS232] Error writing to RS232 Port";
    return 0;
  }
  return status;
}

void RS232Conn::writeData(const string &data)
{
  LOG(logDEBUG) << "[RS232] Sending Data : \""<<data<<"\"";
  unsigned int bytesWritten = 0;
  string dataTerm = data + terminator;
  bytesWritten += writeBuf(dataTerm.c_str(), dataTerm.size());
  usleep(1E6*.02);
  if(removeEcho) {
    readEcho(data);
  }
  if(bytesWritten != (dataTerm.size())){
    LOG(logWARNING) << "[RS232] Missing Data on Write";
  }
}

int RS232Conn::pollPort(char &buf){
  if(port == NULL_FD){
    LOG(logCRITICAL) << "[RS232] Cannot poll non-open port!";
    return -1;
  }
  int status = read(port, &buf, 1);
  if(buf >= 0x11 && buf <= 0x14){
    return 0; //Filter out XON-XOFF Characters that
              //somehow make it to this point.
  }
  return status;
}

int RS232Conn::readStatus(const string &data){
  unsigned int dataSize = data.size();
  unsigned int suffixSize = readSuffix.size();
  unsigned int termSize = terminator.size();
  
  if(suffixSize > 0){ //suffix is ignored if it is unset. i.e. if readSuffix==""
    if(dataSize > suffixSize && data.substr(dataSize-suffixSize,suffixSize) == readSuffix)
      return MATCH_SUFFIX; //Indicates a match on the readSuffix
  }
  if(dataSize > termSize && data.substr(dataSize-termSize,termSize) == terminator){
    return MATCH_TERMINATOR; //Indicates a match on the line terminator
  } else{
    return CONTINUE; //Indicates no match no terminator or readSuffix
  }
}

bool RS232Conn::readData(string &data){
  data.clear();
  unsigned int dataSize = 0;
  time_t tStart;
  time_t tCurr;
  tStart = time(NULL);
  
  int readingStatus;
  do{
    char buf;
    int status = pollPort(buf);
    if(status == 1) {
      data.append(1,buf);
      dataSize++;
      tStart = time(NULL);
    }
    tCurr = time(NULL);
    if (difftime(tCurr,tStart) > timeout){
      LOG(logCRITICAL) << "[RS232] Serial Timeout";
      readingStatus = TIMEOUT;
      break;
    }
    readingStatus = readStatus(data);
  } while(readingStatus == CONTINUE);
  
  bool endLine;
  if(readingStatus == MATCH_SUFFIX){
    data = data.substr(0,dataSize-readSuffix.size());
    endLine = false;
  }else if(readingStatus == MATCH_TERMINATOR){
    data = data.substr(0,dataSize-terminator.size());
    endLine = true;
  }else{ //TIMEOUT
    endLine = false; 
  }
  LOG(logDEBUG) << "[RS232] Received Data : \""<<data<<"\"";
  return endLine;
}

bool RS232Conn::readEcho(const string &data){
  string readSuffixStore = readSuffix;
  setReadSuffix("");
  string echo;
  readData(echo);
  setReadSuffix(readSuffixStore);
  if( echo != data){
    LOG(logWARNING) << "[RS232] Echo \""<<echo<<"\" does not match write \""<<data<<"\"";
    return false;
  }
  return true;
}

void RS232Conn::writeReadBack(const string &dataOut, string &dataIn){
  writeData(dataOut);
  readData(dataIn);
}

