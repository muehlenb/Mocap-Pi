#pragma once

#include <vector>
#include "src/util/math/RBPredictor.h"
#include "src/util/math/Mat4.h"
#include "src/util/math/Vec4.h"
#include "RawMarker.h"

/**
 * A definition of a marker within a rigid body.
 */
struct RigidBodyMarker {
    int id;
    Vec4f position;

    std::vector<float> dist;

    RigidBodyMarker(Vec4f position) :
        position(position){}
};

/**
 * Represents a rigid body with its 6D pose
 * 
 */
struct RigidBody {
    /**
	 * Name of the rigid body.
	 */
	std::string label;

    /**
     * Pose of rigid body
     */
    Mat4f pose;

    /**
     * switch on/off tracking of rigid body
     */
    bool isTracked;

    /**
     * Average Point (arithmetic mid) of all markers.
     *
     * Important when Rigid Body has only 3 markers
     * (for SVD based Transformation Estimation).
     */
    Vec4f averagePoint;

    /**
     * Was the body found in the current frame?
     */
    bool bodyFound = false;

    /**
     * How many frames passed since the body was last visible?
     */
    int notSeenSince = 999;

    /**
     * The rigid body markers
     */
    std::vector<RigidBodyMarker> markers;

    /**
     * Creates an uninitialized rigid body without markers.
     */
    RigidBody(){};

    /**
     * Creates an initialized rigid body with the given marker positions.
     */
    RigidBody(std::vector<Vec4f> markerPositions){
        for(Vec4f& position : markerPositions){
            markers.push_back(RigidBodyMarker(position));
        }
        init();
    }

    /**
     * Creates an initialized rigid body with the given markers.
     */
    RigidBody(std::vector<RigidBodyMarker>& markers)
        : markers(markers){
        init();
    }

    /**
     * Calculate distances between the attached rigid body markers
     * to be able to track it efficiently.
     */
    void init(){
        // Calculate average point:
        averagePoint = Vec4f();
        for(RigidBodyMarker& marker : markers){
            averagePoint = averagePoint + marker.position;
        }
        averagePoint = averagePoint / markers.size();
        averagePoint.w = 1;

        // Clear distances:
        for(RigidBodyMarker& marker : markers){
            marker.dist.clear();
        }

        // Reinit distances:
        for(int i=0; i<markers.size(); ++i) {
            std::vector<float> dist;
            for(int j=0; j<markers.size(); ++j) {
                if(i==j)
                    continue;
                dist.push_back(markers[i].position.distanceTo(markers[j].position));
            }
            markers[i].dist = dist;
        }
    }

    /**
     * Returns the number of available markers
     */
    unsigned int getMarkerNum(){
        return markers.size();
    }
};
