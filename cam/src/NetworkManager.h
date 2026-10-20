#pragma once

#define CAMERA_LISTENING_PORT 4579
#define HOST_LISTENING_PORT 4578

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 

#include "src/PiCamera.h"

/**
 * Network manager which sends status, marker & images to the Mocap Framework
 * and receives status information and control commands from that Framework.
 */
class NetworkManager {
    struct UnixNetworkStuff {
        /** Unix Socket */
        int sockfd;

        /** Listen to all addresses */
        sockaddr_in listening_addr;

        /** Sending only to MocapFramework */
        sockaddr_in sending_addr;

        bool sending_addr_configured = false;
    };

    // Stores the socket stuff:
    UnixNetworkStuff net;

    // Pi Camera to control:
    PiCamera& camera;

    // The marker finder to control:
    MarkerFinder& markerFinder;

    // Should listen to socket?
    bool shouldListen;

    // Listening thread:
    std::thread listeningThread;

    // Last Ping time:
    float pingInMs = -1;

    // Should the raw image be sent for debugging?
    bool shouldSendRawImage = 0;

    // The timestamp of the cam, when a time measurement happened on the host side:
    uint64_t lastBroadcastCamTimestmap = 0;

    // The timestamp of the host, when a time measurement happened on the host side:
    double lastBroadcastHostTimestamp = 0;
    
    // The delta / error to the host timestamp:
    double lastBroadcastHostTimestampDelta = 0;
    
    // Request a raw image?
    bool requestRawImage = false;
    
    /**
     * Configures the address.
     */
    void configureAddress(sockaddr_in& socketaddr, int port, in_addr_t inet_address){
        memset(&socketaddr, 0, sizeof(socketaddr));
        socketaddr.sin_family = AF_INET;
        socketaddr.sin_port = htons(port);
        socketaddr.sin_addr.s_addr = inet_address;
    }

    bool sendPacket(char* message, uint32_t message_len){
        if(!net.sending_addr_configured)
            return false;

        if (sendto(net.sockfd, message, message_len, 0, (struct sockaddr *)&net.sending_addr, sizeof(net.sending_addr)) < 0) {
            return false;
        }
        
        std:: cout << "Sent packet of type: " << int(message[0])  << " to " << inet_ntoa(net.sending_addr.sin_addr) << ":" << net.sending_addr.sin_port << std::endl;
        return true;
    }

    /**
     * This is the timestamp that is also returned from libcamera 
     * when an image was recorded. 
     */
    inline uint64_t getCurrentTimestamp(){
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1000000000 + ts.tv_nsec;
    }

    /**
     * Returns the timestamp that the host currently has.
     */
    inline double getHostTimestamp(uint64_t camTimestamp = 0){
        if(camTimestamp == 0)
            camTimestamp = getCurrentTimestamp();

        // Calculate the difference since the host timestamp was recorded:
        int64_t diffToLastCamTS = int64_t(camTimestamp) - lastBroadcastCamTimestmap;

        // Return the host timestamp + the difference above:
        return lastBroadcastHostTimestamp + (diffToLastCamTS / 1000000000.0);
    }

    void sendStatusPing(){
        const int msgSize = 1 + 4 + 8 + 4 + 1 + 8 + 4 + 1 + 4 + 4 + 4;
        uint8_t message[msgSize];
        
        // Message Type:
        message[0] = uint8_t(1);

        // Unique Camera ID (0 means identified by IP):
        *((int*)(&message[1])) = 0;
        
        // Current Timestamp:
        *((uint64_t*)(&message[5])) = getCurrentTimestamp();
        std::cout << "Sent timestamp: " << *((uint64_t*)(&message[10])) << std::endl;

        // Ping:
        *((float*)(&message[13])) = pingInMs;

        // Should Raw Image be sent:
        message[17] = shouldSendRawImage ? 1 : 0;
        
        // Exposure time:
        *((uint64_t*)(&message[18])) = int(camera.getExposureTime());
        
        // Lens Position:
        *((float*)(&message[26])) = camera.getLensPosition();
        
        // Brightness treshold:
        message[30] = markerFinder.getBrightnessThreshold();

        // Actual maintained framerate:
        *((float*)(&message[31])) = camera.getMeasuredFramerate();

        // Actual maintained framerate:
        *((int*)(&message[35])) = camera.countFrameDropsLastSecond();

        *((float*)(&message[39])) = float(lastBroadcastHostTimestampDelta);

        // Send message:
        sendPacket((char*) message, msgSize);
    }

    void interpretPaket(sockaddr_in recv_addr, uint8_t* buffer, unsigned int length){
        std::cout << "Receive packet of type: " << int(buffer[0]) << " with " << length << " Bytes" << std::endl;

        if(length > 0){
            uint8_t msgType = buffer[0];

            // Type 0: Global Ping from Mocap Framework:
            if(msgType == 0){
                // Ensure buffer is big enough:
                if(length >= 5){
                    // Set framerate to globally desired framerate:
                    camera.setFramerate(*((float*)(&buffer[1])));
                    configureAddress(net.sending_addr, HOST_LISTENING_PORT, recv_addr.sin_addr.s_addr);
                    net.sending_addr_configured = true;

                    std::cout << "Received MocapFramework ping!" << std::endl;
                    // Send ping back:
                    sendStatusPing();
                }
            } else
            // Type 2: Ping Reply from Mocap Framework:
            if(msgType == 2){
                // Ensure buffer is big enough:
                if(length >= 31){
                    // Set framerate to globally desired framerate:
                    uint64_t requestTimestamp = *((uint64_t*)(&buffer[1]));
                    double hostTimestamp = *((double*)(&buffer[9]));

                    requestRawImage = *((uint8_t*)(&buffer[17])) > 0;
                    markerFinder.setBrightnessThreshold(*((uint8_t*)(&buffer[18])));
                    camera.setExposureTime(*((uint64_t*)(&buffer[19])));
                    camera.setLensPosition(*((float*)(&buffer[27])));

                    uint64_t currentTimestamp = getCurrentTimestamp();

                    // Set ping in ms for next status ping:
                    pingInMs = (currentTimestamp - requestTimestamp) / 1000000.f;
                    
                    // Approximate the time when the host generated the message with it's timestamp:
                    uint64_t camTSApprox = (requestTimestamp + currentTimestamp) / 2;

                    // Calculate the difference/error to the previous timestamp:
                    lastBroadcastHostTimestampDelta = (getHostTimestamp(camTSApprox) - hostTimestamp);

                    // Or set directly, when not set yet:
                    lastBroadcastCamTimestmap = camTSApprox;
                    lastBroadcastHostTimestamp = hostTimestamp;

                    std::cout << "Sync Cam-Timestamp " << requestTimestamp << " with Host " << hostTimestamp << "(Error to last: " << (lastBroadcastHostTimestampDelta * 1000.f) << "ms)" << std::endl;
                }
            } else 
            // Type 16: Ping Reply from Mocap Framework:
            if(msgType == 16){
                if(length >= 14){
                    shouldSendRawImage = buffer[1] > 0;
                    camera.setExposureTime(*((int*)(&buffer[2])));
                    camera.setLensPosition(*((float*)(&buffer[10])));
                    markerFinder.setBrightnessThreshold(*((uint8_t*)(&buffer[14])));
                }
            }
        }
    }

    inline uint8_t reducePixel(uint8_t pixel) {
        return static_cast<uint8_t>(pixel / 17);
    }

    std::vector<uint8_t> encodeImage(uint8_t* image, size_t width, size_t height, size_t reduce = 4){
        std::vector<uint8_t> encoded;

        uint8_t currentPixel = reducePixel(image[0]);
        uint8_t runLength = 1;
        
        for (unsigned int y = 0; y < height; y += reduce) {
            for (unsigned int x = (y == 0 ? reduce : 0); x < width; x += reduce) {
                uint8_t newPixel = reducePixel(image[y * width + x]);
                
                if(newPixel == currentPixel && runLength < 15){
                    ++runLength;
                } else {
                    // Add previous pixels to encoded result:
                    uint8_t encodedByte = (currentPixel << 4) | runLength;
                    encoded.push_back(encodedByte);

                    // Set new pixel to the current with a runLength of 1:
                    currentPixel = newPixel;
                    runLength = 1;
                }
            }
        }
        
        uint8_t encodedByte = (currentPixel << 4) | runLength;
        encoded.push_back(encodedByte);

        return encoded;
    }

public:
    NetworkManager(PiCamera& camera, MarkerFinder& markerFinder)
        : camera(camera)
        , markerFinder(markerFinder)
        , shouldListen(true)
        , shouldSendRawImage(false)
    {
        // Creates a socket:
        net.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (net.sockfd < 0) {
            std::cerr << "Error creating the socket." << std::endl;
            return;
        }

        // Configure listening address (any, to receive also broadcast messages):
        // (messages from other cameras should not be received, since they are
        // are bound to the IP of the MocapFramework and should be filtered 
        // beforehand / by the router):
        configureAddress(net.listening_addr, CAMERA_LISTENING_PORT, htonl(INADDR_ANY));

        // Bind listening address to socket:
        if (bind(net.sockfd, (struct sockaddr *)&net.listening_addr, sizeof(net.listening_addr)) < 0) {
            std::cerr << "Error binding the listening address to socket." << std::endl;
            close(net.sockfd);
        }

        listeningThread = std::thread([this](){
            std::cerr << "Started UDP Listening thread" << std::endl;
            while(shouldListen){
                // Nachricht empfangen
                char buffer[1024];
                sockaddr_in recv_addr;
                socklen_t recv_addr_len = sizeof(recv_addr);
                ssize_t recv_len = recvfrom(net.sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
                if (recv_len >= 0) {
                    interpretPaket(recv_addr, (uint8_t*)(buffer), recv_len);
                }
            }
            std::cerr << "Ended UDP Listening Thread" << std::endl;
        });
    }

    ~NetworkManager(){    
        shouldListen = false;
        close(net.sockfd);
        if(listeningThread.joinable())
            listeningThread.join();
    }

    void sendMarker(ProcessBuffer& processBuffer){
        uint8_t buffer[60000];

        int markerNum = processBuffer.markers.size();
        if(markerNum > 2048)
            markerNum = 2048;

        // Message Type 64:
        buffer[0] = 64;

        // Unique Camera ID (0 means identified by IP):
        *((int*)(&buffer[1])) = 0;

        // Timestamp:
        *((double*)(&buffer[5])) = getHostTimestamp(processBuffer.timestamp);

        // Marker Number:
        *((int*)(&buffer[13])) = markerNum;

        // Copy markers: 
        std::memcpy(&buffer[17], processBuffer.markers.data(), markerNum*16);

        sendPacket((char*)buffer, 17 + markerNum*16);

        if(requestRawImage){
            unsigned int width = 1280;
            unsigned int height = 720;
            unsigned int reduce = 4;

            buffer[0] = 65;
            // Unique Camera ID (0 means identified by IP):
            *((int*)(&buffer[1])) = 0;
            *((uint32_t*)(&buffer[5])) = width;
            *((uint32_t*)(&buffer[9])) = height;
            *((uint32_t*)(&buffer[13])) = reduce;

            //high_resolution_clock::time_point time_measure_start = high_resolution_clock::now();
            std::vector<uint8_t> encodedImage = encodeImage(processBuffer.processBuffer, width, height, reduce);
            //double time = duration_cast<microseconds>(high_resolution_clock::now() - time_measure_start).count() / 1000.f;
            unsigned int encodedSize = encodedImage.size();
            if(encodedSize > 59900)
                encodedSize = 59900;

            *((uint32_t*)(&buffer[17])) = encodedSize;

            std::memcpy(&buffer[21], &encodedImage[0], encodedSize);
            sendPacket((char*)buffer, 21 + encodedImage.size());

            //std::cout << "Encoded Image with " << size << " Bytes in " << time  << "ms" << std::endl;
        }
    }
};
