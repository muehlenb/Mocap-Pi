#pragma once

#include "src/util/math/Vec4.h"

/**
 * Represents a 2D marker in a camera image.
 */
struct ImageMarker {
	/**
     * Center x position of marker in 2D camera image.
	 */
	float x;

	/**
	 * Center y position of marker in 2D camera image.
	 */
	float y;

    /**
     * The x-flow of this marker (determined by the FrameSet on insert!)
     */
    float flowX = 0;

    /**
     * The y-flow of this marker (determined by the FrameSet on insert!)
     */
    float flowY = 0;

	/** 
	 * Count of pixels of the 2D marker in the image (neighboring 
	 * pixels which exceeded a certain treshold).
	 */
    float pixels;
	
	/**
	 * Sum of intensity over all pixels.
	 */
    float intensity;

    /**
     * Corresponding 3D Marker Index.
     */
    int marker3DIdx = -1;

    /**
     * Returns the 2D coordinates of the marker.
     */
    Vec4f getCoordinates(){
        return Vec4f(x, y, 0, 0);
    }
};

/**
 * Represents an unique identifier for a image marker in a specific camera.
 */
struct ImageMarkerId {
    int camId = -1;
    int markerId = -1;

    ImageMarkerId(int camId, int markerId)
        : camId(camId)
        , markerId(markerId)
    {}

    bool operator==(const ImageMarkerId& t) const
    {
        return (this->camId == t.camId && this->markerId == t.markerId);
    }
};


namespace std {
template <>
struct hash<ImageMarkerId> {
    std::size_t operator()(const ImageMarkerId& k) const {
        return std::hash<int>()(k.markerId + k.camId * 123);
    }
};
}
