#pragma once

#include <sys/mman.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <chrono>

#include <libcamera/libcamera.h>

using namespace std::chrono_literals;
using namespace std::chrono;

/**
 * PiCamera.
 *
 * Uses libcamera to control and access the Raspberry v3 Camera.
 */
class PiCamera {
private:
    /**
     * LibCamera Camera Manager.
     */
    std::unique_ptr<libcamera::CameraManager> cm = std::make_unique<libcamera::CameraManager>();

    /**
     * LibCamera Camera.
     */
    std::shared_ptr<libcamera::Camera> camera;

    /**
     * Controls the exposure time (in µs).
     */
    int64_t exposureTime = 2000;
    
    /**
     * Controls the lens position.
     */
    int lensPosition = 0; 
    
    /**
     * Controls the framerate.
     */
    float framerate = 80;

    /**
     * Image Callback.
     */
    std::function<void(void*, unsigned int, unsigned int, unsigned int, unsigned int, uint64_t)> imageCallback;
    
    /**
     * Processes a frame of the camera.
     */
    void requestComplete(libcamera::Request *request);

    /**
     * Framebuffer allocator.
     */
    libcamera::FrameBufferAllocator* allocator;

    /**
     * Stream
     */
    libcamera::Stream* stream;

    /**
     * All the requests which are used.
     */
    std::vector<std::unique_ptr<libcamera::Request>> requests;

    /**
     * Last measurement for framerate measurement.
     */
    steady_clock::time_point lastFPSMeasureTimePoint = steady_clock::now();

    uint64_t lastMeasuredTimestamp;

    /**
     * The actual maintained framerate:
     */
    double measuredFramerate = 0;

    /**
     * 
     */
    bool frameDropBuffer[100];
    int frameDropBufferIdx = 0;

public:
    /**
     * Initializes the PiCamera and starts streaming from it.
     */
    void start();

    /**
     * Stops streaming from the PiCamera
     */
    void stop();

    /**
     * Registers a callback that is called every time a new frame is available.
     */
    void registerImageCallback(std::function<void(void*, unsigned int, unsigned int, unsigned int, unsigned int, uint64_t)> cb);

    /**
     * Sets the framerate in frames per second [1,100].
     */
    void setFramerate(float pFramerate){
        framerate = std::min(100.f, std::max(1.f, pFramerate));
    }
    
    /**
     * Sets the lens position [0,1].
     */
    void setLensPosition(float pLensPosition){
        lensPosition = std::min(1.f, std::max(0.f, pLensPosition));
    }
    
    /**
     * Sets the exposure time in microseconds [100, 1000000/fps]
     */
    void setExposureTime(int64_t pExposureTime){
        int64_t maxExposureTime = int64_t(1000000.f/framerate);
        exposureTime = std::min(maxExposureTime, std::max(int64_t(100), pExposureTime));
    }

    /**
     * Returns the desired framerate.
     */
    float getFramerate(){ return framerate; };

    /**
     * Returns the measured framerate.
     */
    float getMeasuredFramerate(){ return measuredFramerate; };

    /**
     * Returns the lens position [0, 1].
     */
    float getLensPosition(){ return lensPosition; };

    /**
     * Returns the exposure time in milliseconds
     */
    int64_t getExposureTime(){ return exposureTime; };

    /**
     * Returns the number of frame drops in the last second. 
     */
    int countFrameDropsLastSecond(){
        int framedrops = 0;
        for(int i=0; i < framerate; ++i){
            if(frameDropBuffer[(frameDropBufferIdx + i)%100])
                ++framedrops;
        }

        return framedrops;
    }
};
