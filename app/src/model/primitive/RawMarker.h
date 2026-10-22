#pragma once

#include <string>

#include "src/util/math/Vec4.h"

/**
 * Represents a 3D marker in world coordinates.
 */
struct RawMarker {
	/**
	 * Label / Name of the marker. May be empty.
	 */
	std::string label;
	
	/**
	 * World position of the 3D marker.
	 */
	Vec4f position;

    /**
     *  Id of marker to track between frames.
     *  if id is -1, no prev
     */
    int id = -1;

    /**
     * If true, markers are highlighted in the viewport
     * Additionally rigid bodies can be created from highlighted markers
     */
    bool isSelected = false;

    RawMarker(Vec4f position)
        : position(position){}
};
