#pragma once

#include <cstdint>

struct CameraDeviceState {
    /**
     * Unique camera id
     */
    int uniqueCamID = 0;

    /**
     * Host time when the camera was seen the last time.
     */
    double lastTimeSeen = 0.;

    /**
     * The synchronized timestamp of the host when the image was recorded.
     */
    double hostTimestamp = 0.;

    /**
     * The synchronized timestamp of the camera device when the image was recorded.
     */
    uint64_t deviceTimestamp = 0;

    /**
     * The ping measured by the camera to the host.
     */
    float ping = 0;

    /**
     * Whether the camera currently sends also raw image data.
     */
    bool sendRawImage = false;

    /**
     * Current set exposure time of the camera.
     */
    uint64_t exposureTime = 0;

    /**
     * Current set lens position of the camera.
     */
    float lensPosition = 0.f;

    /**
     * Current brightness threshold (when a pixel is considered to be a part
     * of the marker based on it's intensity).
     */
    uint8_t brightnessThreshold = 0;

    /**
     * 'Real' measured framerate by the camera.
     */
    float measuredFramerate = 0;

    /**
     * Measured framedrops by the camera during the last second.
     */
    int frameDropsLastSecond = 0;

    /**
     * Current time synchronization error:
     */
    float timeSyncError = 0.f;

    /**
     * The delayed time between the image is captured on the camera device
     * until the frame is received and inserted in the frameSet on host side:
     */
    float captureToAvailableDelay = 0.f;

    /**
     * Requested brightness threshold.
     *
     * Note: This value is sent repeatedly to the camera to update it on change.
     */
    int requestedBrightnessThreshold = 20;

    /**
     * Requested exposure time.
     *
     * Note: This value is sent repeatedly to the camera to update it on change.
     */
    int requestedExposureTime = 2000;

    /**
     * Requested lens position.
     *
     * Note: This value is sent repeatedly to the camera to update it on change.
     */
    float requestLensPosition = 0.f;

    /**
     * Should the raw image be requested?
     *
     * Note: This value is sent repeatedly to the camera to update it on change.
     */
    bool requestRawImage = false;
};
