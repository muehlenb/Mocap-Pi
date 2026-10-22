#include <iostream>
#include <chrono>
#include <cstring>

#include "src/PiCamera.h"
#include "src/ProcessBufferManager.h"
#include "src/MarkerFinder.h"
#include "src/NetworkManager.h"

#include "Semaphore.h"

/**
 * Setups the camera and starts processing.
 */
int main()
{
    // Initialize camera:
    PiCamera camera;
    camera.start();

    // Initialize Marker Finder:
    MarkerFinder markerFinder;

    // Initialize Network manager:
    NetworkManager networkManager(camera, markerFinder);

    // Initialize Image Buffer Manager:
    ProcessBufferManager bufferManager(3, 1280, 720);

    // Is true as long as the processing threads should run:
    bool shouldProcess = true;

    // Copy image thread (from Framebuffer into a local uint8_t* buffer):
    std::thread copyingThread([&shouldProcess, &bufferManager](){
        while(shouldProcess){
            ProcessBuffer& buffer = bufferManager.getBufferForCopying();

            // Requires that source bind happened:
            buffer.sourceSemaphore.acquire();
            if(!shouldProcess)
                return;

            high_resolution_clock::time_point time_measure_start = high_resolution_clock::now();
            
            
            #pragma omp parallel for
            for(int i = 0; i < 2; ++i){
                int startIdx = 1280 * 360 * i;
                int length = 1280 * 360;

                memcpy(buffer.processBuffer + startIdx, buffer.sourceBuffer + startIdx, length);
            }


            munmap(buffer.sourceBuffer, buffer.sourceLength);
            buffer.copyDuration = duration_cast<microseconds>(high_resolution_clock::now() - time_measure_start).count() / 1000.f;
            
            // Notify that copying happened:
            buffer.copiedSemaphore.release();
        }
    });
    
    // Find markers thread:
    std::thread markerFinderThread([&markerFinder, &shouldProcess, &bufferManager](){
        while(shouldProcess){
            ProcessBuffer& buffer = bufferManager.getBufferForMarkerFinder();

            // Requires that copying happened:
            buffer.copiedSemaphore.acquire();
            if(!shouldProcess)
                return;

            high_resolution_clock::time_point time_measure_start = high_resolution_clock::now();
            buffer.markers = markerFinder.findInImage(buffer.processBuffer, 1280, 720);
            buffer.findMarkerDuration = duration_cast<microseconds>(high_resolution_clock::now() - time_measure_start).count() / 1000.f;
            
            // Notify that processing happened:
            buffer.markersSemaphore.release();
        }
    }); 
    
    // Sending thread:
    std::thread sendThread([&markerFinder, &shouldProcess, &bufferManager, &networkManager](){
        uint64_t firstTimestamp = 0;

        while(shouldProcess){
            ProcessBuffer& buffer = bufferManager.getBufferForProcessing();

            // Requires that copying happened:
            buffer.markersSemaphore.acquire();
            if(!shouldProcess)
                return;

            if(firstTimestamp == 0)
                firstTimestamp = buffer.timestamp;

            std::cout << "Image " << buffer.sequence << " ( " << (buffer.timestamp)  << " ) | Found " << buffer.markers.size() << " Markers | " << "( " << buffer.copyDuration << " , " << buffer.findMarkerDuration << " )" << std::endl;
            networkManager.sendMarker(buffer);

            // Notify that processing happened:
            buffer.processedSemaphore.release();
        }
    });

    // Callback, when image is received:
    // (This function doesn't block (e.g. for copying), so that directly the next image can be read)
    camera.registerImageCallback([&markerFinder, &bufferManager]
            (void* source, unsigned int length, unsigned int, unsigned int, unsigned int sequence, uint64_t timestamp)
    {
        // Assign content:
        ProcessBuffer& buffer = bufferManager.getBufferForSourceBind();
        buffer.processedSemaphore.acquire();
        {
            buffer.sourceBuffer = source;
            buffer.sourceLength = length;
            buffer.sequence = sequence;
            buffer.timestamp = timestamp;
        }
        buffer.sourceSemaphore.release();
    });

    // BLOCKS, AS LONG AS APP SHOULD BE RUNNING:
    std::this_thread::sleep_for(1800000s);

    // Stop camera:
    camera.stop();

    // Terminate:
    shouldProcess = false;

    // Release all semaphores so that the threads can terminate:
    bufferManager.releaseAll();

    // Join all threads:
    if(copyingThread.joinable())
        copyingThread.join();
    if(markerFinderThread.joinable())
        markerFinderThread.join();
    if(sendThread.joinable())
        sendThread.join();
}
