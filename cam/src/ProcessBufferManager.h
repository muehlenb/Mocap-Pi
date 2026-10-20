#pragma once

#include "src/Vec4f.h"
#include "src/Semaphore.h"

/**
 * Represents an Process Buffer
 */
struct ProcessBuffer {
    unsigned int sequence;
    uint64_t timestamp;
    void* sourceBuffer;
    unsigned int sourceLength;
    uint8_t* processBuffer;
    std::vector<Vec4f> markers;

    Semaphore sourceSemaphore;
    Semaphore copiedSemaphore;
    Semaphore markersSemaphore;
    Semaphore processedSemaphore;

    float copyDuration;
    float findMarkerDuration;

    ProcessBuffer(unsigned int width, unsigned int height) 
        : sourceBuffer(nullptr)
        , processBuffer(new uint8_t[width*height])
        , sourceSemaphore(0)
        , copiedSemaphore(0)
        , markersSemaphore(0)
        , processedSemaphore(1)
    {}

    ~ProcessBuffer(){
        delete[] processBuffer;
    }
};

/**
 * 
 */
class ProcessBufferManager {
    std::vector<std::unique_ptr<ProcessBuffer>> buffers;

    int sourceIdx = 0;
    int copyIdx = 0;
    int markIdx = 0;
    int processIdx = 0;

public:
    ProcessBufferManager(int bufferNum, unsigned int width, unsigned int height){
        for(int i=0; i < bufferNum; ++i){
            buffers.emplace_back(std::make_unique<ProcessBuffer>(width, height));
        }
    }    

    void releaseAll(){
        for(int i=0; i < int(buffers.size()); ++i){
            ProcessBuffer& result = *buffers[i];
            result.sourceSemaphore.release();
            result.copiedSemaphore.release();
            result.markersSemaphore.release();
            result.processedSemaphore.release();
        }  
    }

    ProcessBuffer& getBufferForSourceBind(){
        ProcessBuffer& result = *buffers[sourceIdx++];

        if(sourceIdx >= int(buffers.size()))
            sourceIdx = 0;

        return result;
    }

    ProcessBuffer& getBufferForCopying(){
        ProcessBuffer& result = *buffers[copyIdx++];

        if(copyIdx >= int(buffers.size()))
            copyIdx = 0;

        return result;
    }
    
    ProcessBuffer& getBufferForMarkerFinder(){
        ProcessBuffer& result = *buffers[markIdx++];

        if(markIdx >= int(buffers.size()))
            markIdx = 0;

        return result;
    }

    ProcessBuffer& getBufferForProcessing(){
        ProcessBuffer& result = *buffers[processIdx++];
        
        if(processIdx >= int(buffers.size()))
            processIdx = 0;

        return result;
    }
};
