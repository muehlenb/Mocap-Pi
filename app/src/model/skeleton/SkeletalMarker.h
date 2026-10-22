#pragma once

#include "src/util/math/Vec4.h"

/**
 * Represents a marker that is attached to a skeleton.
 */
struct SkeletalMarker {
    /**
     * Marker ID.
     */
    int boneID = -1;

    /**
     * Default position of the marker in the skeleton.
     */
    Vec4f position;

    /**
     * Creates an uninitialized / invalid marker
     */
    SkeletalMarker(){}

    /**
     * Creates a marker which is attached to the given boneID
     * with an offset of the given position to the bone root.
     */
    SkeletalMarker(int boneID, Vec4f position)
        : boneID(boneID)
        , position(position){}

};
