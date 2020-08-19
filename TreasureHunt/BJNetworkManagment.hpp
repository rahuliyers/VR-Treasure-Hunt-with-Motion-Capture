//
//  BJNetworkManagment.hpp
//  TreasureHunt
//
//  Created by Rahul Iyer on 27/11/19.
//

#ifndef BJNetworkManagment_hpp
#define BJNetworkManagment_hpp

#include <stdio.h>
#include <unistd.h>
#include <map>
#include "UDPServer.h"
#include "rapidjson/document.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <unordered_set>
#include <vector>
// Note: This is the maximum number of messages being retrieved from the
// queue per call to the update() method. This value should be fine if
// there are about 2400 messages per second. Increase this if messages
// are being lost because the server is overloaded with messages.
#define MAX_MESSAGES_PER_UPDATE 30000
#define MAXLINE 1024

typedef enum{
    NMS_CHECKING_LOCAL_IP_AND_PORT = 0,
    NMS_FOUND_MULTIPLE_LOCAL_IP_AND_PORT, // Then we have a problem!
    NMS_FOUND_SINGLE_IP_AND_PORT,
    NMS_POSTING_VALUE_TO_SITE,
    
    NMS_CREATING_SERVER,
    NMS_WAIT_ONE_SECOND_TILL_PACKETS_ARRIVE,
    NMS_FIGURE_OUT_CORRECT_LOCAL_IP,
    NMS_SET_CURRENT_IP_AND_PORT,
    NMS_SET_WAITING_FOR_RESPONSE_FOR_SET_IP_AND_PORT,
    NMS_TESTING_GET_IP,
    NMS_TESTING_GET_IP_WAITING_FOR_RESPONSE,
    NMS_TESTING_GET_IP_COMPLETED,
    NMS_RECEIVED_IP_AND_PORT_MATCH,
    
    NMS_RECEIVING_PACKETS,
}NEUROMANCER_STATE;

class BJNetworkManagement{
private:
    UDPServer *udpserver_ = nullptr;

    NEUROMANCER_STATE _state = NMS_CREATING_SERVER;
    std::string _currentIp = ""; // Not set
    int _currentPort = 32000; // Not set
    float _time = 0;
    std::string _setIPAndPortURL = "";
    bool _isURLSet = false;
    std::map<std::string, std::string> addresses;
    std::unordered_set<std::string> _receivedIps;
    std::vector<std::array<int, 4>> _messages;

    void getIPAddresses(std::map<std::string, std::string>& addresses) {
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
    
    void sendUDPPacket(const std::string& content,const std::string& ipAddress,const int& port){
//        CCLOG("Trying to send packet");
        int sockfd;
        char buffer[MAXLINE];
        struct sockaddr_in     servaddr;
        
        // Creating socket file descriptor
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("socket creation failed");
            exit(EXIT_FAILURE); //TODO: Instead of this we should fail gracefully.
        }
        
        memset(&servaddr, 0, sizeof(servaddr));
        
        // Filling server information
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
//        CCLOG("Trying %s",ipAddress.c_str());
        servaddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());
        
        int n, len;
        auto message = content.c_str();
        sendto(sockfd, (const char *)message, strlen(message),
               0, (const struct sockaddr *) &servaddr,
               sizeof(servaddr));
//        CCLOG("UDP message sent.:\n %s",message);
        
        //          n = recvfrom(sockfd, (char *)buffer, MAXLINE,
        //                      MSG_WAITALL, (struct sockaddr *) &servaddr,
        //                      &len);
        //          buffer[n] = '\0';
        //          printf("Server : %s\n", buffer);
        
        close(sockfd);
    }
    
    //TODO: Need to implement this
    void setIPAndPort(){
//        bool isImmediate = true;
        _setIPAndPortURL = "https://rahul549.wixsite.com/mysite/_functions/updateIp/" + _currentIp + "/" + std::to_string(_currentPort);
        _isURLSet = true;
//
//        HttpRequest* request = new (std::nothrow) HttpRequest();
//        request->setUrl(url);
//        request->setRequestType(HttpRequest::Type::GET);
//        request->setResponseCallback(CC_CALLBACK_2(HelloWorld::onSetIPAndPortCompleted, this));
//        _state = NMS_SET_WAITING_FOR_RESPONSE_FOR_SET_IP_AND_PORT;
//        if (isImmediate)
//        {
//            request->setTag("GET immediate updateIp");
//            HttpClient::getInstance()->sendImmediate(request);
//        }else
//        {
//            request->setTag("GET getIp");
//            HttpClient::getInstance()->send(request);
//        }
//        request->release();
        
    }
    
public:
    
    BJNetworkManagement(){
        udpserver_ = new UDPServer(_currentPort);

    }
    
    NEUROMANCER_STATE getState(){
        return _state;
    }
    
        void update(float dt){
    //        _timeSinceStart += dt;
            switch (_state) {
                case NMS_CREATING_SERVER:{
//                    CCLOG("NMS_CREATING_SERVER");
                    udpserver_->setupServer();
                    sleep(2);
                    if(udpserver_->isReady()){
                        _currentPort = udpserver_->getPort();
                        _state = NMS_FIGURE_OUT_CORRECT_LOCAL_IP;
                        getIPAddresses(addresses);
                        for (const auto& address : addresses) {
//                            CCLOG("%s: %s", address.first.c_str(), address.second.c_str());
                            if(address.second == "127.0.0.1"){
                                continue;
                            }
                            //TODO: Send packets to each ip address and port combination, and see which ones receive it.
                            
                            sendUDPPacket(address.second, address.second, _currentPort);
                        }
                        _state = NMS_WAIT_ONE_SECOND_TILL_PACKETS_ARRIVE;
                        _time = 0;
                    }else{
//                        CCLOG("UDP Server not ready yet, trying next port");
                        udpserver_->nextPort();
                    }
                    break;
                }
                case NMS_WAIT_ONE_SECOND_TILL_PACKETS_ARRIVE:{
//                    CCLOG("NMS_WAIT_ONE_SECOND_TILL_PACKETS_ARRIVE");
                    std::array<int,4> current;
                    while(udpserver_->getIPAddress(current)){
                        std::string ipstring = "";
                        for(int i = 0; i<current.size();i++){
                            ipstring += std::to_string(current.at(i));
                            
                            if(i<current.size()-1){
                                ipstring+=".";
                            }
                        }
                        _receivedIps.insert(ipstring);
                    }
                    
                    _time += dt;
                    if(_time > 1.0f){
                        _state = NMS_FIGURE_OUT_CORRECT_LOCAL_IP;
                    }
                    break;
                }
                case NMS_FIGURE_OUT_CORRECT_LOCAL_IP:{
//                    CCLOG("NMS_FIGURE_OUT_CORRECT_LOCAL_IP");
                    auto ipsreceived = _receivedIps.size();
                    switch (ipsreceived) {
                        case 0:{
                            //TODO: What do we do if we don't receive any ips
//                            CCLOG("No idea what the IP is!. Restart the app");
                            break;
                        }case 1:{
                            //Perfect! Received exactly one ip
                            for (auto& ip : _receivedIps){
                                _currentIp = ip;
                                break;
                            }
                            _state = NMS_SET_CURRENT_IP_AND_PORT;
                            break;
                        }
                        default:{
                            //TODO: What do we do if we receive too many ips
//                            CCLOG("WARNING! Received more than one IP! Don't know which one to use. Trying the first");
                            for (auto& ip : _receivedIps){
                                _currentIp = ip;
                                break;
                            }
                            _state = NMS_SET_CURRENT_IP_AND_PORT;
                            break;
                        }
                    }
                    break;
                }
                case NMS_SET_CURRENT_IP_AND_PORT:{
//                    CCLOG("NMS_SET_CURRENT_IP_AND_PORT");
                    setIPAndPort();
                    udpserver_->setFoundIP();
                    break;
                }
                case NMS_SET_WAITING_FOR_RESPONSE_FOR_SET_IP_AND_PORT:{
//                    CCLOG("NMS_SET_WAITING_FOR_RESPONSE_FOR_SET_IP_AND_PORT");
                    break;
                }
                case NMS_TESTING_GET_IP:{
                    //                CCLOG("NMS_TESTING_GET_IP");
                    //                checkIPAndPort();
                    //                _state = NMS_TESTING_GET_IP_WAITING_FOR_RESPONSE;
                    break;
                }case NMS_TESTING_GET_IP_WAITING_FOR_RESPONSE:{
                    //                CCLOG("NMS_TESTING_GET_IP_WAITING_FOR_RESPONSE");
                    //                //Do nothing
                    break;
                }case NMS_RECEIVING_PACKETS:{
                    int count = 0;
                    do {
                        std::array<int, 4> message;
                        if (udpserver_->getMessage(message)) {
                            count++;
    //                        CCLOG("ID: %d; Pitch:%d;Roll: %d; Yaw: %d",message[0],message[1],message[2],message[3]);
                            // Do something with the message. This is how you can access the values of the messages:
                            // Value 1: message[0]
                            // Value 2: message[1]
                            // Value 3: message[2]
                            // Value 4: message[3]
    //                        if(_boneMap.find(message[0])!=_boneMap.end()){
    //                            auto boneName = _boneMap[18];
    //                            sendInputRotation(boneName, message[1], message[3], message[2]);
    //                        }
                            //TODO: Need to store the message in a datastructure that ios can read.
//                            mapSensorInput(message);
                            _messages.push_back(message);
                        } else {
                            // The queue is empty, so we stop trying to get messages.
                            break;
                        }
                    } while (count < MAX_MESSAGES_PER_UPDATE);
                    break;
                }
                default:
                    break;
            }
        }
    
    std::string getCurrentIP(){
        return _currentIp;
    }
    
    int getCurrentPort(){
        return _currentPort;
    }
    
    std::string getSetIPAndPortURL(){
        return _setIPAndPortURL;
    }
    
    bool isURLSet(){
        return _isURLSet;
    }
    
    void updateStateTo(NEUROMANCER_STATE state){
        _state = state;
    }
    
    std::vector<std::array<int,4>> getMessages(){
        return _messages;
    }
    
    void clearMessages(){
        _messages.clear();
    }
};
#endif /* BJNetworkManagment_hpp */
