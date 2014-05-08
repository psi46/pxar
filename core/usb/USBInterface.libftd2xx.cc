#ifndef WIN32
#include <libusb.h>
#include <cstring>
#include <unistd.h>
#include <time.h> // needed for usleep function
#endif

#include <cstdio>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "log.h"
#include "exceptions.h"

#include "USBInterface.h"

using namespace std;
using namespace pxar;


CUSB::CUSB(){
  m_posR = m_sizeR = m_posW = 0;
  isUSB_open = false;
  ftHandle = 0;
  ftdiStatus = 0;
  enumPos = enumCount = 0;
  m_timeout = 150000; // maximum time to wait for read call in ms
 }

 CUSB::~CUSB(){
   Close();
 }

const char* CUSB::GetErrorMsg(int error)
{
	switch (error)
	{
	case FT_OK:                          return "ok";
	case FT_INVALID_HANDLE:              return "invalid handle";
	case FT_DEVICE_NOT_FOUND:            return "device not found";
	case FT_DEVICE_NOT_OPENED:           return "device not opened";
	case FT_IO_ERROR:                    return "io error";
	case FT_INSUFFICIENT_RESOURCES:      return "insufficient resource";
	case FT_INVALID_PARAMETER:           return "invalid parameter";
	case FT_INVALID_BAUD_RATE:           return "invalid baud rate";
	case FT_DEVICE_NOT_OPENED_FOR_ERASE: return "device not opened for erase";
	case FT_DEVICE_NOT_OPENED_FOR_WRITE: return "device not opened for write";
	case FT_FAILED_TO_WRITE_DEVICE:      return "failed to write device";
	case FT_EEPROM_READ_FAILED:          return "eeprom read failed";
	case FT_EEPROM_WRITE_FAILED:         return "eeprom write failed";
	case FT_EEPROM_ERASE_FAILED:         return "eeprom erase failed";
	case FT_EEPROM_NOT_PRESENT:          return "eeprom not present";
	case FT_EEPROM_NOT_PROGRAMMED:       return "eeprom not programmed";
	case FT_INVALID_ARGS:                return "invalid args";
	case FT_NOT_SUPPORTED:               return "not supported";
	case FT_OTHER_ERROR:                 return "other error";
	}
	return "unknown error";
}


bool CUSB::EnumFirst(uint32_t &nDevices)
{
	ftdiStatus = FT_ListDevices(&enumCount, NULL, FT_LIST_NUMBER_ONLY);
	if (ftdiStatus != FT_OK)
	{
		nDevices = enumCount = enumPos = 0;
		return false;
	}

	nDevices = enumCount;
	enumPos = 0;
	return true;
}


bool CUSB::EnumNext(char name[])
{
	if (enumPos >= enumCount) return false;
	ftdiStatus = FT_ListDevices((PVOID)enumPos, name, FT_LIST_BY_INDEX);
	if (ftdiStatus != FT_OK)
	{
		enumCount = enumPos = 0;
		return false;
	}

	enumPos++;
	return true;
}


bool CUSB::Enum(char name[], uint32_t pos)
{
	enumPos=pos;
	if (enumPos >= enumCount) return false;
	ftdiStatus = FT_ListDevices((PVOID)enumPos, name, FT_LIST_BY_INDEX);
	if (ftdiStatus != FT_OK)
	{
		enumCount = enumPos = 0;
		return false;
	}

	return true;
}


bool CUSB::Open(char serialNumber[])
{
	if (isUSB_open) { ftdiStatus = FT_DEVICE_NOT_OPENED; return false; }


  m_posR = m_sizeR = m_posW = 0;
  ftdiStatus = FT_OpenEx(serialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
  if( ftdiStatus != FT_OK)
#ifdef _WIN32
    return false;
#else
  {
    /** maybe the ftdi_sio and usbserial kernel modules are attached to 
        the device. Try to detach them using the libusb library directly
    */

    /* prepare libusb structures */
    libusb_device ** list;
    libusb_device_handle *handle;
    struct libusb_device_descriptor descriptor;

    /* initialise libusb and get device list*/
    libusb_init(NULL);
    ssize_t ndevices = libusb_get_device_list(NULL, &list);
    if( ndevices < 0)
      return false;

    char serial [20];

    bool found = false;

    /* loop over all USB devices */
    for( int dev = 0; dev < ndevices; dev++) {
      /* get the device descriptor */
      int ok = libusb_get_device_descriptor(list[dev], &descriptor);
      if( ok != 0)
        continue;

      /* we're only interested in devices with one vendor and two possible product ID */
      if( descriptor.idVendor != 0x0403 && (descriptor.idProduct != 0x6001 || descriptor.idProduct != 0x6014))
        continue;

      /* open the device */
      ok = libusb_open(list[dev], &handle);
      if( ok != 0)
        continue;

      /* Read the serial number from the device */
      ok = libusb_get_string_descriptor_ascii(handle, descriptor.iSerialNumber, (unsigned char *) serial, 20);
      if( ok < 0)
        continue;

      /* Check the device serial number */
      if( strcmp(serialNumber, serial) == 0) {
        /* that's our device */
        found = true;

        /* Detach the kernel module from the device */
        ok = libusb_detach_kernel_driver(handle, 0);
        if( ok == 0)
          printf("Detached kernel driver from selected testboard.\n");
        else
          printf("Unable to detach kernel driver from selected testboard.\n");
        break;
      }

      libusb_close(handle);
    }

    libusb_free_device_list(list, 1);

    /* if the device was not found in the previous loop, don't try again */
    if( !found)
      return false;

    /* try to re-open with the detached device */
    ftdiStatus = FT_OpenEx(serialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
    if( ftdiStatus != FT_OK)
      return false;
  }
#endif
	
  // set synchronous bit bang mode (see: http://www.ftdichip.com/Support/Documents/DataSheets/ICs/DS_FT232H.pdf page 34ff)
  ftdiStatus = FT_SetBitMode(ftHandle, 0xFF, 0x40);
  if (ftdiStatus != FT_OK) UsbConnectionError("Error setting FTDI synchronous bit-bang mode.");
  // set the baud rate
  ftdiStatus = FT_SetBaudRate(ftHandle, 9600);
  if (ftdiStatus != FT_OK) UsbConnectionError("Error setting FTDI baud rate.");
  // set usb transfer size parameters (see: http://www.ftdichip.com/Support/Knowledgebase/ft_setusbparameters.htm)
  ftdiStatus = FT_SetUSBParameters(ftHandle, 8192, 8192); // default: 4096, must be multiple of 64
  if (ftdiStatus != FT_OK) UsbConnectionError("Error setting USB transfer size parameters.");


  FT_SetTimeouts(ftHandle,m_timeout,m_timeout);
  isUSB_open = true;
  return true;
}


void CUSB::Close()
{
	if (!isUSB_open) return;
	FT_Close(ftHandle);
	isUSB_open = 0;
}


void CUSB::Write(uint32_t bytesToWrite, const void *buffer)
{
  if (!isUSB_open) throw UsbConnectionError("Attempt to write to USB without open connection.");
	uint32_t k=0;
	for (k=0; k < bytesToWrite; k++)
	{
		if (m_posW >= USBWRITEBUFFERSIZE) { Flush(); }
		m_bufferW[m_posW++] = ((unsigned char*)buffer)[k];
	}
}

void CUSB::WriteCommand(unsigned char x){
  const unsigned char CommandChar = ESC_EXTENDED; 
  Write(sizeof(char), &CommandChar); // ESC_EXTENDED 
  Write(sizeof(char),&x);
}


void CUSB::Flush()
{
	DWORD bytesWritten;
	DWORD bytesToWrite = m_posW;
	m_posW = 0;

	if (!isUSB_open) throw UsbConnectionError("Attempt to write to USB without open connection.");

	if (!bytesToWrite) return;

	ftdiStatus = FT_Write(ftHandle, m_bufferW, bytesToWrite, &bytesWritten);

	if (ftdiStatus != FT_OK) throw UsbConnectionError("Failure writing to USB");
	if (bytesWritten != bytesToWrite) { ftdiStatus = FT_IO_ERROR; throw UsbConnectionError("Incomplete write to USB."); }
}


bool CUSB::FillBuffer(uint32_t minBytesToRead)
{
	if (!isUSB_open) return false;

	DWORD bytesAvailable, bytesToRead;

	ftdiStatus = FT_GetQueueStatus(ftHandle, &bytesAvailable);
	if (ftdiStatus != FT_OK) return false;

	if (m_posR<m_sizeR) return false;

	bytesToRead = (bytesAvailable>minBytesToRead)? bytesAvailable : minBytesToRead;
	if (bytesToRead>USBREADBUFFERSIZE) bytesToRead = USBREADBUFFERSIZE;

	ftdiStatus = FT_Read(ftHandle, m_bufferR, bytesToRead, &m_sizeR);
        if (m_sizeR < bytesToRead) {
          LOG(logCRITICAL) << "Requested to read " << bytesToRead 
			   << "b, but read " << m_sizeR 
			   << "b - " << (bytesToRead-m_sizeR) << "b missing!";
        }
	m_posR = 0;
	if (ftdiStatus != FT_OK)
	{
	  LOG(logCRITICAL) << "FTD2XX error occured: " << GetErrorMsg(ftdiStatus);
	  m_sizeR = 0;
	  return false;
	}
	return true;
}


void CUSB::Read(uint32_t bytesToRead, void *buffer, uint32_t &bytesRead)
{

	if (!isUSB_open) throw UsbConnectionError("Attempt to read from USB without open connection.");

	bool timeout = false;
	bytesRead = 0;

	uint32_t i;

	for (i=0; i<bytesToRead; i++)
	{
		if (m_posR<m_sizeR)
			((unsigned char*)buffer)[i] = m_bufferR[m_posR++];

		else if (!timeout)
		{
			uint32_t n = bytesToRead-i;
			if (n>USBREADBUFFERSIZE) n = USBREADBUFFERSIZE;

			if (!FillBuffer(n)) throw UsbConnectionError("Error writing to USB");
			if (m_sizeR < n) timeout = true;

			if (m_posR<m_sizeR)
				((unsigned char*)buffer)[i] = m_bufferR[m_posR++];
			else
			{   // timeout (bytesRead < bytesToRead)
				bytesRead = i;
				throw UsbConnectionTimeout("Read from USB timed out.");
			}
		}

		else
		{
			bytesRead = i;
			throw UsbConnectionTimeout("Read from USB timed out.");
		}
	}

	bytesRead = bytesToRead;
}


void CUSB::Clear()
{ 
	if (!isUSB_open) return;

	ftdiStatus = FT_Purge(ftHandle, FT_PURGE_RX|FT_PURGE_TX);
	m_posR = m_sizeR = 0;
	m_posW = 0;
}

bool CUSB::Show()
{
  if( !isUSB_open ) return false;
  std::cout << "USB";
  DWORD bytesAvailable;
  ftdiStatus = FT_GetQueueStatus( ftHandle, &bytesAvailable );
  if( ftdiStatus != FT_OK ) {
    std::cout << " not OK\n";
    return false;
  }
  std::cout << ": bytesAvailable = " << bytesAvailable;
  std::cout << ", m_sizeR = " << m_sizeR;
  std::cout << ", m_posR = " << m_posR;
  std::cout << ", m_posW = " << m_posW;
  std::cout << std::endl;

  return true;
}


void CUSB::SetTimeout(unsigned int timeout)
{
  m_timeout = timeout;
  if( !isUSB_open ) return;
  FT_SetTimeouts(ftHandle,m_timeout,m_timeout);
}

void CUSB::Read_String(char *s, uint16_t maxlength)
{
	char ch = 0;
	uint16_t i=0;
	do
	{
	  Read_CHAR(ch);
	  if (i<maxlength) { s[i] = ch; i++; }
	} while (ch != 0);
	if (i >= maxlength) s[maxlength-1] = 0;
}


void CUSB::Write_String(const char *s)
{
	do
	{
	  Write_CHAR(*s);
	  s++;
	} while (*s != 0);
}

