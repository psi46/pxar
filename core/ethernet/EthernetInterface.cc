#include "EthernetInterface.h"
#include "rpc_error.h"
#include "config.h"

#include "log.h"
#include "exceptions.h"

using namespace std;
using namespace pxar;

void print_eth_packet(const unsigned char* packet, int len){
	printf("LEN:%d  ", len);
	for(int i = 0; i < len; i++){
		printf("%02X:",packet[i]);
	}
	printf("\n");
}

bool packet_equals(const unsigned char* pkt1, 
                   const unsigned char* pkt2, int size){
	for(int i = 0; i < size; i++){
		if(pkt1[i] != pkt2[i]) return false;
	}
	return true;
}

#ifdef __unix__
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>

#include <cstdio>
#include <cstdlib>
#include <string.h>

void Get_MAC(const char* if_name, unsigned char* buffer){
    ifaddrs * ifap = 0;
    if(getifaddrs(&ifap) == 0){
        ifaddrs * iter = ifap;
        while(iter){
            sockaddr_ll * sal = reinterpret_cast<sockaddr_ll*>(iter->ifa_addr);
            if(sal->sll_family == AF_PACKET){
                if(!strcmp(if_name, iter->ifa_name)){
                    for(int i = 0 ; i < sal->sll_halen; i++){
                        buffer[i] = sal->sll_addr[i];
                    }
                    break;
                }
            }
            iter = iter->ifa_next;
        }
        freeifaddrs(ifap);
    }
}
#endif

#ifdef __APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void Get_MAC(const char* if_name, unsigned char* buffer){
    int mib[6];
    size_t len;
    char            *buf;
    unsigned char        *ptr;
    struct if_msghdr    *ifm;
    struct sockaddr_dl    *sdl;

    mib[0] = CTL_NET;
    mib[1] = AF_ROUTE;
    mib[2] = 0;
    mib[3] = AF_LINK;
    mib[4] = NET_RT_IFLIST;
    if ((mib[5] = if_nametoindex(if_name)) == 0) {
        perror("if_nametoindex error");
        exit(2);
    }

    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
        perror("sysctl 1 error");
        exit(3);
    }

    if ((buf = (char*)malloc(len)) == NULL) {
        perror("malloc error");
        exit(4);
    }

    if (sysctl(mib, 6, buf, &len, NULL, 0) < 0) {
        perror("sysctl 2 error");
        exit(5);
    }

    ifm = (struct if_msghdr *)buf;
    sdl = (struct sockaddr_dl *)(ifm + 1);
    ptr = (unsigned char *)LLADDR(sdl);

	for(int i = 0; i < 6; i++){
		buffer[i] = ptr[i];
	}
}
#endif

#ifdef _WIN32 //Not finished
#include <winsock2.h>
#include <iphlpapi.h>

void Get_MAC(const char* if_name, unsigned char* buffer){

    std::vector<unsigned char> buf;
    DWORD bufLen = 0;
    GetAdaptersAddresses(0, 0, 0, 0, &bufLen);
    if(bufLen)
    {
         buf.resize(bufLen, 0);
         IP_ADAPTER_ADDRESSES * ptr =
                         reinterpret_cast<IP_ADAPTER_ADDRESSES*>(&buf[0]);
         DWORD err = GetAdaptersAddresses(0, 0, 0, ptr, &bufLen);
         if(err == NO_ERROR)
         {
              while(ptr)
             {
                  if(ptr->PhysicalAddressLength)
                  {
                     // get the mac bytes
                    // copy(ptr->PhysicalAddress,
                     // ptr->PhysicalAddress+ptr->PhysicalAddressLength,
                     // buffer);
                  }
                  ptr = ptr->Next;
              }
         }
    }
}
#endif




CEthernet::CEthernet(){
	unsigned int ipid = getpid();
	host_pid[0] = (unsigned char) (ipid>>8);
	host_pid[1] = (unsigned char) ipid;
    interface += ETHERNET_INTERFACE;
    is_open = false;
    LOG(logINTERFACE) << "Start initializing ethernet interface.";
    InitInterface();
}

CEthernet::~CEthernet(){
    if(is_open){
		Close();
		pcap_close(descr);
	}
}

void CEthernet::Write(const void *buffer, unsigned int size){
    for(unsigned int i = 0 ; i < size; i++){
        if(tx_payload_size == MAX_TX_DATA){
            Flush();
        }
        tx_frame[ETH_HEADER_SIZE + tx_payload_size] = ((char*)buffer)[i];
        tx_payload_size++;
    }
}
void CEthernet::Flush(){
	for(int i =0; i < 6; i++){
		tx_frame[i] = dtb_mac[i];
		tx_frame[i+6] = host_mac[i];
	}
	tx_frame[12] = 0x08;
	tx_frame[13] = 0x09;
	tx_frame[14] = host_pid[0];
	tx_frame[15] = host_pid[1];
	tx_frame[16] = 0x0;
	
    tx_frame[17] = tx_payload_size >> 8;
    tx_frame[18] = tx_payload_size;
    
    IFLOG(logINTERFACE) {
      std::stringstream st;
      st << std::uppercase << std::hex;
      for(size_t i = 0; i < tx_payload_size + ETH_HEADER_SIZE; i++) {
	st << std::setw(2) << std::setfill('0') << tx_frame[i];
      }
      st << std::nouppercase << std::dec;
      LOG(logINTERFACE) << "Sent packet: " << st.str();
    }
    
    pcap_sendpacket(descr, tx_frame, tx_payload_size + ETH_HEADER_SIZE);
    tx_payload_size = 0;
}
void CEthernet::Clear(){
    tx_payload_size = 0;
    rx_buffer.clear();
}
void CEthernet::Read(void *buffer, unsigned int size){
    int timeout = 10000;
    for(unsigned int i = 0; i < size; i++){
        if(!rx_buffer.empty()){
            ((unsigned char*)buffer)[i] = rx_buffer.front();
            rx_buffer.pop_front();
        } else{
            const unsigned char* rx_frame = pcap_next(descr, &header);
            if(rx_frame == NULL){
                timeout--;
                i--;
                if(timeout == 0){
                    printf("Error reading from ethernet.\n");
                    throw CRpcError(CRpcError::TIMEOUT);
                }
            }

	    IFLOG(logINTERFACE) {
	      std::stringstream st;
	      st << std::uppercase << std::hex;
	      for(size_t i = 0; i < header.len; i++){
		st << std::setw(2) << std::setfill('0') << rx_frame[i];
	      }
	      st << std::nouppercase << std::dec;
	      LOG(logINTERFACE) << "Received packet: " << st.str();
	    }

            if(header.len < ETH_HEADER_SIZE){ // malformed message
                i--;
                continue;
            }
            
            if(!packet_equals(rx_frame,host_mac,6) || 
               !packet_equals(rx_frame+14,host_pid,2) ||
               rx_frame[16] != 0) {
				   i--;
				   continue;
			   }
	    LOG(logINTERFACE) << "Passed Filter.";
			
            unsigned int rx_payload_size = rx_frame[17];
            rx_payload_size = (rx_payload_size << 8) | rx_frame[18];

            for(unsigned int j =0; j < rx_payload_size;j++){
                rx_buffer.push_back(rx_frame[j + ETH_HEADER_SIZE]);
            }
            i--;
        }
    }
}

void CEthernet::InitInterface(){
    rx_buffer.resize(0);
    for(int i =0; i < TX_FRAME_SIZE; i++){
        tx_frame[i] = 0;
    }
    tx_payload_size = 0;
    
    char errbuf[PCAP_ERRBUF_SIZE];
    descr = pcap_open_live(interface.c_str(), BUFSIZ,0,100,errbuf);
    if(descr == NULL) {
      LOG(logINTERFACE) << "pcap_open_live() failed:";
      LOG(logINTERFACE) << interface << " | " << errbuf;
      throw CRpcError(CRpcError::IF_INIT_ERROR);
    }
    
    Get_MAC(interface.c_str(), host_mac); 
    for(int i = 0; i < 6; i++) tx_frame[i+6] = host_mac[i];
}

bool CEthernet::EnumFirst(unsigned int &nDevices){
	Hello();
	nDevices = MAC_addresses.size();
	return true;
}
bool CEthernet::EnumNext(char name[]){
	string MAC = MAC_addresses[MAC_counter];
	MAC_counter = (MAC_counter + 1) % MAC_addresses.size();
	
	for(size_t i = 0; i < MAC.size(); i++){
	  name[i] = MAC.at(i);
	}
	return true;
}
bool CEthernet::Enum(char name[], unsigned int pos){
	string MAC = MAC_addresses[pos];
	
	for(int i = 0; i < 10; i++){
		name[i] = MAC[i];
	}
	return true;
}

bool CEthernet::Open(char MAC_address[]){
	if(is_open) Close();
	bool success = Claim(((unsigned char*)MAC_address)+7,true);
	if(!success) return false;
	for(int i =0; i < 6; i++){
		dtb_mac[i] = MAC_address[7+i];
	}
	is_open = true;
	return true;
}

void CEthernet::Close(){
	if(!is_open) return;
	bool success = Unclaim();
	if(!success) throw CRpcError(CRpcError::ETH_ERROR);
	for(int i =0; i < 6; i++){
		dtb_mac[i] = 0x00;
		tx_frame[i] = dtb_mac[i];
	}
	is_open = false;
}

void CEthernet::Hello(){
    unsigned char packet[17];
	
	for(int i = 0; i < 6; i++){
		packet[i] = 0xFF;
		packet[i+6] = host_mac[i];
	}
	packet[12] = 0x08;
	packet[13] = 0x09;
	packet[14] = host_pid[0];
	packet[15] = host_pid[1];
	packet[16] = 0x1;
	
    pcap_sendpacket(descr, packet, sizeof(packet));
	
	
	MAC_addresses.clear();
	MAC_counter = 0;
    //wait at least 1 second for responses.
    time_t startTime = time(NULL);
	const unsigned char* rx_frame;
    while(time(NULL) - startTime < 2){ 
		rx_frame = pcap_next(descr, &header);
		if(rx_frame == NULL) continue;
		if(header.len < 17) continue;
		
		if(rx_frame[12] != 0x08 || rx_frame[13] != 0x09) continue;
		
		if(!packet_equals(rx_frame + 14, host_pid, 2)) continue;
		
		
		if(packet[16] != 0x1) continue;
		
		
		string mac("DTB_ETH      ");
		for(int i = 0; i < 6; i++) mac[7+i] = rx_frame[6+i];
		MAC_addresses.push_back(mac);
	}
}

bool CEthernet::Claim(const unsigned char* MAC, bool force){
    unsigned char packet[17];
	
	for(int i = 0; i < 6; i++){
		packet[i] = MAC[i];
		packet[i+6] = host_mac[i];
	}
	packet[12] = 0x08;
	packet[13] = 0x09;
	packet[14] = host_pid[0];
	packet[15] = host_pid[1];
	packet[16] = (force) ? 0x4 : 0x2;
	
    pcap_sendpacket(descr, packet, sizeof(packet));
	
	
	MAC_addresses.clear();
	MAC_counter = 0;
    //wait at least 1 second for response.
    time_t startTime = time(NULL);
	const unsigned char* rx_frame;
    while(time(NULL) - startTime < 2){ 
		rx_frame = pcap_next(descr, &header);
		if(rx_frame == NULL) continue;
		if(header.len < 17) continue;
		
		if(rx_frame[12] != 0x08 || rx_frame[13] != 0x09) continue;
		if(!packet_equals(rx_frame + 14, host_pid, 2)) continue;
		if(!packet_equals(rx_frame, host_mac, 6)) continue;
		return rx_frame[16] == 0x1; //value of 1 indicates claim was successful
		
	}
	return false;
}

bool CEthernet::Unclaim(){
    unsigned char packet[17];
	
	for(int i = 0; i < 6; i++){
		packet[i] = dtb_mac[i];
		packet[i+6] = host_mac[i];
	}
	packet[12] = 0x08;
	packet[13] = 0x09;
	packet[14] = host_pid[0];
	packet[15] = host_pid[1];
	packet[16] = 0x3; //unclaim
	
    pcap_sendpacket(descr, packet, sizeof(packet));
	
	MAC_addresses.clear();
	MAC_counter = 0;
    //wait at least 1 second for response.
    time_t startTime = time(NULL);
	const unsigned char* rx_frame;
    while(time(NULL) - startTime < 2){ 
		rx_frame = pcap_next(descr, &header);
		if(rx_frame == NULL) continue;
		if(header.len < 17) continue;
		
		if(rx_frame[12] != 0x08 || rx_frame[13] != 0x09) continue;
		if(!packet_equals(rx_frame + 14, host_pid, 2)) continue;
		if(!packet_equals(rx_frame, host_mac, 6)) continue;
		
		return rx_frame[16] == 0x1; //value of 1 indicates unclaim was successful
	}
	return false;
}
