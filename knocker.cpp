#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream> // files
#include <unistd.h> // sleep()

#define SIZE 1000
typedef unsigned int uint;

using namespace std;

// Global list of sequence of ports
vector<uint> ports;

// This function reads the ports in the configuration file.
void readCfg(char* cfgFile) {
    fstream file(cfgFile, fstream::in);
    string line;

    if (file.is_open()) {
        while(getline(file, line)) {
            unsigned int port = strtoul(line.c_str(), NULL, 0);
            ports.push_back(port);
        }
    }
}

int main(int argc, char* argv[]) {

    // Check if 2 arguments are received.
    if (argc < 3) {
        cout<<"Syntax is: ./knocker ports.cfg <target-ip>"<<endl;
        return 1;
    } 

    // Get all the ports to which the packet needs to be sent to.
    readCfg(argv[1]);

    // Read the IP and store in strIP
    char strIP[20] = {0};
    strcpy(strIP, argv[2]);

    // Open a socket and start sending the UDP packets.
    char buff[SIZE];
    memset(buff, 0, SIZE);
    strcpy(buff, "DEAD");

    int cd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in saddr, serv_addr;
    socklen_t sl = sizeof(saddr);

    saddr.sin_family = AF_INET;
    inet_aton(strIP, &saddr.sin_addr);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 7080;

    // Bind to a particular port.
    if (0 != bind(cd, (sockaddr*) &serv_addr, sizeof(serv_addr)) ) {
        cout<<"Bind failed"<<endl;
        return 1;
    }

    // Send the packets with 100ms delay to ensure packets are received in sequence.
    for (vector<uint>::const_iterator it = ports.begin(); it != ports.end(); it++) {
        saddr.sin_port = htons(*it);
        int n = sendto(cd, buff, strlen(buff), 0, (struct sockaddr*) &saddr, sl);
	    usleep(100000);
    }

    return 0;
}
