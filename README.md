# Port Knocker
Port Knocker is a mechanism in which a malicious command is executed on a vulnerable system by externally generating a set sequence of connection attempts to closed ports of the vulnerable system. In this case, the backdoor on the vulenerable system sends an HTTP Request to the passed URL when the correct sequence is hit.   
  
  
## Knocker program:
The knocker acts as simple UDP client that fetches the port sequence from the configuration-file and sends UDP packets with dummy data to the destination IP address and port sequence in the correct order.  
The knocker exits once all UDP packets have been transmitted.  
  
  
## Backdoor program:
For the Backdoor I have created a raw socket which would be monitoring all the UDP packets that pass through the Ethernet interfaces.
The Backdoor checks the IPv4 and UDP headers of each packet[1] to ascertain whether it is destined for the local host.
I use a STL map to keep a record whether the UDP packet is destined to the first port or the successive port present in the configuration-file.  
If either of the above criteria match then I store the index of the next expected port into the map and wait for this next port to be hit.
Once the Backdoor receives a UDP packet which completes all hits in the port sequence in the correct order then it will initiate a HTTP Request.  
I have implemented the HTTP Request using the cURL library[2] which is present in Ubuntu distributions by default.
The Backdoor waits for a command in the response. It fetches the command from the response and executes it on the local machine.
After this it monitors for more incoming packets that meet the port sequence. The Backdoor doesn't exit.  
  
  
## Packages Required:
Even though all cURL binary packages are present by default for running the backdoor, I need the "libcurl4-openssl-dev" package which contains the cURL header files for a successful build.  
  
  
## Makefile:
This is a simple makefile to build both the knocker and backdoor.
  
  
## References:
[1] Advanced TCP/IP - THE RAW SOCKET PROGRAM EXAMPLES - http://www.tenouk.com/Module43a.html  
[2] libcurl API Tutorial - https://curl.haxx.se/libcurl/c/
