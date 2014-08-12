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

#include <termios.h>
#include <string>

class RS232Conn{
    std::string portName;
    std::string writeSuffix;
    std::string readSuffix;
    int port;
    int baudRate;
    struct termios oldPortSettings;
    
    int pollPort(char &buf);
    int writeBuf(const char *buf, int len);
    
  public:
    RS232Conn();
    RS232Conn(const std::string &portName, int baudrate);
    ~RS232Conn();
    
    bool openPort();
    void closePort();
    
    void setPortName(const std::string &portName);
    void setBaudRate(int baudRate);
    void setReadSuffix(const std::string &suffix);
    void setWriteSuffix(const std::string &suffix);
    
    int writeData(const std::string &data);
    int readData(std::string &data);
    void writeReadBack(const std::string &dataOut, std::string &dataIn);

    bool isDCDEnabled();
    bool isCTSEnabled();
    bool isDSREnabled();

    void enableDTR();
    void disableDTR();
    void enableRTS();
    void disableRTS();
};
#endif
