#ifndef NETWORKHELPER_H
#define NETWORKHELPER_H

#include <ifaddrs.h>
#include <arpa/inet.h>

#include <map>
#include <string>

// Get IP Addresses
void getNeuroAddresses(std::map<std::string, std::string>& addresses) {
    struct ifaddrs *interfaces = NULL;
    struct ifaddrs *temp_addr = NULL;
    // Retrieve the current interfaces - returns 0 on success
    int success = getifaddrs(&interfaces);
    if (success == 0) {
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while (temp_addr != NULL) {
            if (temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                addresses[temp_addr->ifa_name] = inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr);
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
}


#endif /* NETWORKHELPER_H */
