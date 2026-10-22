

#pragma once

#define MAX_CAMERA_NUM 12
#define HOST_LISTENING_PORT 4578
#define CAMERA_LISTENING_PORT 4579

#include <boost/asio.hpp>
#include <vector>
#include <cstring>
#include <thread>

#include "src/model/camera/CameraFrame.h"
#include "src/model/camera/CameraBuffer.h"

#include "src/model/DataModel.h"

#include "src/processors/Processor.h"

/**
 * Receives the markers from the cameras and injects them into the pipeline.
 */
class CameraDataProvider : public Processor {
private:
    // Network stuff:
    boost::asio::io_context io_service;
    boost::asio::ip::udp::endpoint remote_endpoint_;
    std::vector<char> recv_buffer_;

    // Network threads:
    std::thread receive_thread;
    std::thread broadcast_ping_thread;
    bool is_running_ = false;

    // Sockets:
    boost::asio::ip::udp::socket* receive_socket;
    boost::asio::ip::udp::socket* broadcast_socket;
    boost::asio::ip::udp::socket* send_socket;
    boost::asio::ip::udp::socket* reply_socket;

    /**
     * Deserializes a 'type 64' network packet into the 'RawFrame' structure.
     */
    void deserializeFrame(CameraFrame& paket, const uint8_t* buffer, std::size_t bytes_recvd, int processedUniqueCamID);

    /**
     * Correctly inserts the given RawFrame into the 'cameraFrameSets'-map or the
     * 'groundTruthFrameSet', depending on the data.
     */
    void storeFrame(CameraFrame& paket, boost::asio::ip::udp::endpoint& sender_endpoint);

    /**
     * Areas in the image that are masked.
     * (camera ID -> masked set)
     */
    std::map<int, std::vector<ImageMarker>> cameraMasks;

    /**
     * Add frames to camera mask:
     */
    void addFrameToCameraMask(CameraFrame& frame){
        if(cameraMasks.count(frame.uniqueCamID) == 0)
            cameraMasks.insert(std::pair<int, std::vector<ImageMarker>>(frame.uniqueCamID, std::vector<ImageMarker>()));

        std::vector<ImageMarker>& set = cameraMasks[frame.uniqueCamID];

        for(ImageMarker& marker : frame.markers){
            set.push_back(marker);
        }
    }

    std::chrono::steady_clock::time_point timepointStart;

    /**
     * Decodes the image.
     */
    std::vector<uint8_t> decodeImage(const uint8_t* encoded, unsigned int size);

    /**
     * Deserializes a 'type 65' network packet as a rawImage.
     */
    void deserializeAndStoreRawImage(const uint8_t* buffer, std::size_t bytes_recvd, boost::asio::ip::udp::endpoint& sender_endpoint);

    /**
     * Returns an unique camera ID.
     *
     * If the given 'cameraID' is greater than 0, the cameraID is simply returned - this
     * case is used in case of the dummy marker sender.
     *
     * Real cameras return 0 for the cameraID. In this case, we use the fourth ip block
     * as unique camera identifier.
     */
    int retrieveUniqueCameraID(int cameraID, boost::asio::ip::udp::endpoint endpoint);

    /**
     * A set that stores the newest ground truth frames.
     */
    CameraBuffer groundTruthFrameSet;

    std::vector<CameraFrame> recordedData;
    bool isRecording = false;
public:
    /**
     * Requested framerate for the cameras:
     */
    float requestedFramerate = 60;

    /**
     * Should record a mask?
     */
    bool shouldRecordAMask = false;

    /**
     * Should the broadcast address be updated in the next send broadcast
     * packet?
     */
    bool updateBroadcastAddrNextSend = false;

    /**
     * Initializes a camera mask for all available cameras.
     */
    void resetCameraMasks(){
        cameraMasks = std::map<int, std::vector<ImageMarker>>();
    }

    /**
     * Constructs the camera data provider.
     */

    CameraDataProvider(DataModel& model);

    /**
     * Destructs the camera data provider object.
     */
    ~CameraDataProvider();

    /**
     * Interprets a packet.
     */
    void interpretPacket(boost::asio::ip::udp::endpoint& sender_endpoint, uint8_t* recv_buffer, size_t length);

    /**
     * Replies to a network endpoint.
     */
    void reply(boost::asio::ip::udp::endpoint endpoint, const uint8_t* data, std::size_t length);

    /**
     * Returns the time in seconds since this class was created. This is considered
     * as the 'host' timestamp, the timestamp of this MocapFramework.
     */
    double getHostTimestamp();

    /**
     * Starts (or stops) recording of frames (into memory).
     */
    void recordFrames(bool shouldRecord = true) {isRecording = shouldRecord;};

    /**
     * Writes the recorded frames to files.
     */
    void writeFrames();

    /**
     * Injects the camera frame data into the pipeline.
     */
    virtual void process(TickData& state) override;
};
