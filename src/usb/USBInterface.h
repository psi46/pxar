// Class provides basic functionalities to use the USB interface
// IMPORTANT: there are two implementations for this class, each using a different USB library.
// What implementation is being used is determined by the arguments to the configure script
// and then passed through the makefiles to the compiler.
// Please implement and test your modifications for both versions.

// TODO:
// - Move WaitForQueue() into read method in ftd2xx implementation


#ifndef USB_H
#define USB_H

#ifdef HAVE_LIBFTDI
#include "ftdi.h"
#else
#include "ftd2xx.h"
#endif

#include "rpc_io.h"

#include <inttypes.h>

#define USBWRITEBUFFERSIZE  150000
#define USBREADBUFFERSIZE   150000


#define ESC_EXTENDED 0x8f

class CUSB : public CRpcIo
{
  bool isUSB_open;

  int ftdiStatus;

#ifndef HAVE_LIBFTDI
  FT_HANDLE ftHandle;
#endif

  uint32_t enumPos, enumCount;
  uint32_t m_timeout; // maximum time to wait for read/write call in ms

  uint32_t m_posW;
  unsigned char m_bufferW[USBWRITEBUFFERSIZE];

  uint32_t m_posR, m_sizeR;
  unsigned char m_bufferR[USBREADBUFFERSIZE];

  bool FillBuffer(uint32_t minBytesToRead);

public:
  CUSB();
  ~CUSB();
  int32_t GetLastError() { return ftdiStatus; }
#ifdef HAVE_LIBFTDI
  const char* GetErrorMsg();
  const char* GetErrorMsg(int){GetErrorMsg();};
#else
  const char* GetErrorMsg(int error);
#endif
  bool EnumFirst(uint32_t &nDevices);
  bool EnumNext(char name[]);
  bool Enum(char name[], uint32_t pos);
  bool Open(char serialNumber[]);
  void Close();
  bool Connected() { return isUSB_open; };
  void Write(uint32_t bytesToWrite, const void *buffer);
  void WriteCommand(unsigned char x);
  void Flush();
  void Read(uint32_t bytesToRead, void *buffer, uint32_t &bytesRead);
  void Read(void *buffer, uint32_t bytesToRead){
    uint32_t bytesRead;
    Read(bytesToRead, (unsigned char *)buffer, bytesRead);
    if (bytesRead != bytesToRead) throw CRpcError(CRpcError::READ_ERROR);
  }
  void Write(const void *buffer, uint32_t bytesToWrite) { 
      Write(bytesToWrite, buffer); 
  }

  void Clear();

  bool Show();
  int GetQueue();
  bool WaitForFilledQueue(int pSize,int pMaxWait=10000);
  void SetTimeout(unsigned int timeout){m_timeout = timeout;}


  // read methods

  bool Read_CHAR(char &x) { Read(&x, sizeof(char)); }

  bool Read_CHARS(char *x, uint16_t count)
  { Read(x, count*sizeof(char)); }

  bool Read_UCHAR(unsigned char &x)	{ Read(&x, sizeof(char)); }

  bool Read_UCHARS(unsigned char *x, uint32_t count)
  { Read(x, count*sizeof(char)); }

  bool Read_SHORT(int16_t &x)
  { Read((unsigned char *)(&x), sizeof(int16_t)); }

  bool Read_SHORTS(int16_t *x, uint16_t count)
  { Read(x, count*sizeof(int16_t)); }

  bool Read_USHORT(uint16_t &x)
  { Read((unsigned char *)(&x), sizeof(int16_t)); }

  bool Read_USHORTS(uint16_t *x, uint16_t count)
  { Read(x, count*sizeof(int16_t)); }

  bool Read_INT(int32_t &x)
  { Read((unsigned char *)(&x), sizeof(int32_t)); }

  bool Read_INTS(int32_t *x, uint16_t count)
  { Read(x, count*sizeof(int32_t)); }

  bool Read_UINT(uint32_t &x)
  { Read((unsigned char *)(&x), sizeof(int32_t)); }

  bool Read_UINTS(uint32_t *x, uint16_t count)
  { Read(x, count*sizeof(int32_t)); }

  bool Read_LONG(int32_t &x)
  { Read((unsigned char *)(&x), sizeof(int32_t)); }

  bool Read_LONGS(int32_t *x, uint16_t count)
  { Read(x, count*sizeof(int32_t)); }

  bool Read_ULONG(uint32_t &x)
  { Read((unsigned char *)(&x), sizeof(int32_t)); }

  bool Read_ULONGS(uint32_t *x, uint16_t count)
  { Read(x, count*sizeof(int32_t)); }

  bool Read_String(char *s, uint16_t maxlength);


  // -- write methods

  bool Write_CHAR(char x) { Write(&x, sizeof(char)); }

  bool Write_CHARS(const char *x, uint16_t count)
  { Write(x, count*sizeof(char)); }

  bool Write_UCHAR(const unsigned char x) { Write(&x, sizeof(char)); }

  bool Write_UCHARS(const unsigned char *x, uint32_t count)
  { Write(x, count*sizeof(char)); }

  bool Write_SHORT(const int16_t x) { Write(&x, sizeof(int16_t)); }

  bool Write_SHORTS(const int16_t *x, uint16_t count)
  { Write(x, count*sizeof(int16_t)); }

  bool Write_USHORT(const uint16_t x)
  { Write(&x, sizeof(int16_t)); }

  bool Write_USHORTS(const uint16_t *x, uint16_t count)
  { Write(x, count*sizeof(int16_t)); }

  bool Write_INT(const int32_t x) { Write(&x, sizeof(int32_t)); }

  bool Write_INTS(const int32_t *x, uint16_t count)
  { Write(x, count*sizeof(int32_t)); }

  bool Write_UINT(const uint32_t x) { Write(&x, sizeof(int32_t)); }

  bool Write_UINTS(const uint32_t *x, uint16_t count)
  { Write(x, count*sizeof(int32_t)); }

  bool Write_LONG(const int32_t x) { Write(&x, sizeof(int32_t)); }

  bool Write_LONGS(const int32_t *x, uint16_t count)
  { Write(x, count*sizeof(int32_t)); }

  bool Write_ULONG(const uint32_t x) { Write(&x, sizeof(int32_t)); }

  bool Write_ULONGS(const uint32_t *x, uint16_t count)
  { Write(x, count*sizeof(int32_t)); }

  bool Write_String(const char *s);
};

#endif
