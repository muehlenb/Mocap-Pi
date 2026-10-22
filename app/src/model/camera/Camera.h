#pragma once

#define MAX_FRAMESET_SIZE 30

#include "src/model/camera/CameraCalibration.h"
#include "src/model/camera/CameraDeviceState.h"
#include "src/model/camera/CameraBuffer.h"

/**
 * Represents a single camera.
 */
struct Camera {
    /**
     * A buffer that stores the N newest camera frames for delayed
     * and interpolated processing.
     */
    CameraBuffer buffer;

    /**
     * Stores the current state of the device (contains also update
     * request which are sent repeatedly to controll the real device).
     */
    CameraDeviceState deviceState;

    /**
     * Current Calibration of this camera.
     */
    CameraCalibration calibration;

    /**
     * Contains the last sent 8-bit image from the camera device.
     */
    std::vector<uint8_t> rawImage;

    /**
     * Host time when the last 8-bit image from the camera device was
     * received.
     */
    double lastRawImageReceived = -1.;

    /**
     * Should this camera be ignored in triangulation?
     */
    bool ignore = false;

    /**
     * Creates a new camera with a buffer size that was specified in
     * `MAX_FRAMESET_SIZE`.
     */
    Camera() : buffer(MAX_FRAMESET_SIZE) {}
};
