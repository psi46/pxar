#ifndef PXAR_ETHERNET_H
#define PXAR_ETHERNET_H

#include <deque>
#include <string>
#include <vector>
#include <ctime>

#include <unistd.h> //needed for unique pid

#include <pcap.h>

#include "rpc_io.h"


#define RX_FRAME_SIZE 2048
#define TX_FRAME_SIZE 2048

#define MAX_TX_DATA 1500
#define ETH_HEADER_SIZE 19


class CEthernet : public CRpcIo
{
    void InitInterface();
    
    void Hello();
    bool Claim(const unsigned char* MAC, bool force);
    bool Unclaim();

    //pcap data structures
    pcap_t* descr;
    struct pcap_pkthdr header;
    
    std::string interface;
    std::vector<std::string > MAC_addresses;
    int MAC_counter;
    
    unsigned char host_pid[2];
    
    std::deque<unsigned char>    rx_buffer;
    unsigned char      tx_frame[TX_FRAME_SIZE];
    unsigned char      dtb_mac[6];
    unsigned char      host_mac[6];
    
    unsigned int       tx_payload_size;//data in tx buffer minus header(14 bytes)
    bool                is_open;
public:
    CEthernet();
    ~CEthernet();
    
    const char* Name() { return "Ethernet";};
    
    int GetLastError() { return 0; }
    const char* GetErrorMsg(int /*error*/){return NULL;};

    void SetTimeout(unsigned int /*timeout*/) {};
    
    bool EnumFirst(unsigned int &nDevices);
    bool EnumNext(char name[]);
    bool Enum(char name[], unsigned int pos);
    bool Open(char MAC_address[]);
    bool Connected() { return is_open; };

    void Write(const void *buffer, unsigned int size);
    void Flush();
    void Clear();
    void Read(void *buffer, unsigned int size);
    void Close(CRpcIo* io);
    void Close();
    bool IsOpen();
};

#endif
