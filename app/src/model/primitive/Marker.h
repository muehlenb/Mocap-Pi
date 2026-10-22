#pragma once

#include <string>
#include "src/model/DataModel.h"
#include "src/util/math/Vec4.h"

struct Marker {
    /**
     * Label / Name of the marker. May be empty.
     */
    std::string label;

	/**
	 * World position of the 3D marker.
	 */
	Vec4f position;

    /**
     * Speed in 3D world space.
     */
    Vec4f speed;

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

    int obsoleteCounter = 0;

    Marker(RawMarker raw) : position(raw.position) {};
    Marker() {};
    
};
