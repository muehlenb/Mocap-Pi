#pragma once

#include "src/model/primitive/RawMarker.h"
#include "src/model/primitive/Marker.h"
#include "src/model/primitive/RigidBody.h"
#include "src/model/skeleton/Skeleton.h"

#include "src/model/camera/Camera.h"

#include "src/util/Semaphore.h"

/**
 * Data of a current tick that was measured at a specific
 * timepoint.
 */
struct TickData {
    /**
     * The timestamp when the images where recorded.
     */
    double timestamp;

    /**
     * A list of interpolated frames (one for each camera).
     */
    std::vector<CameraFrame> frames;

    /**
     * List of ground truth marker positions.
     */
    std::vector<Vec4f> markersGTPos;

    /**
     * List of found 3D markers without any labels etc.
     */
    std::vector<RawMarker> markers;

    /**
     * List of intersections (for debugging).
     */
    std::vector<Vec4f> triangulationIntersections;

    /**
     * List of Rigid Body Pointers that should be processed
     * and identified this frame.
     *
     * References the rigid bodies in the data model (either
     * model.rigidBodies or model.skeletons[x].rigidBodies).
     */
    std::vector<RigidBody*> allActiveRigidBodies;

    /**
     * List of Skeleton Pointers that should be processed
     * and identified this frame.
     *
     * References the skeletons
     */
    std::vector<Skeleton*> activeSkeletons;
};



/**
 * This data model contains all the current data that exists in
 * the scene, including predefined rigid bodies and skeletons.
 *
 * This data will be kept over multiple frames and are processed
 * by the processors (e.g. the rigid body will be repositioned).
 */
struct DataModel {
    /**
     * Last state data that was fully processed by the pipeline
     * to make it visible in the GUI.
     *
     * Note: This data should NOT be accessed by `processors`!
     */
    TickData lastTick;

	/**
     * List of cameras in the scene (maps the unique id to the
     * camera).
	 */
    std::map<int, Camera> cameras;

    /**
     * List of labeled markers in the scene.
     */
    std::vector<Marker> markers;

	/**
     * List of rigid bodies that are in the scene.
	 */
	std::vector<RigidBody> rigidBodies;

    /**
     * List of skeletons that are in the scene.
     */
    std::vector<Skeleton> skeletons;

    /**
     * This semaphore is used to block concurrent write access.
     */
    Semaphore semaphore;

    
    std::vector<Marker> getVisibleLabeledMarkers() {
        std::vector<Marker> visibleMarkers;
        for(const auto& m : markers) {
            if(m.obsoleteCounter == 0)
                visibleMarkers.push_back(m);
        }

        return visibleMarkers;
    };


    /**
     * Initializes the data model.
     */
    DataModel()
        : semaphore(1){

        /*
        RigidBody rigidBody = RigidBody(std::vector<Vec4f>({
            Vec4f(0.04, 0.04, 0.02, 1.0f),
            Vec4f(-0.04, 0.04, 0.02, 1.0f),
            Vec4f(-0.03, -0.03, 0.02, 1.0f),
        }));
        rigidBody.label = "RB1";
        rigidBodies.push_back(rigidBody);

        RigidBody rigidBody2 = RigidBody(std::vector<Vec4f>({
            Vec4f(0.04, 0.04, 0.02, 1.0f),
            Vec4f(-0.04, 0.02, 0.02, 1.0f),
            Vec4f(-0.01, -0.03, 0.02, 1.0f),
        }));
        rigidBody2.label = "RB2";
        rigidBodies.push_back(rigidBody2);

        RigidBody rigidBody3 = RigidBody(std::vector<Vec4f>({
            Vec4f(0.04, 0.03, 0.02, 1.0f),
            Vec4f(-0.04, 0.02, 0.02, 1.0f),
            Vec4f(0.02, -0.02, 0.02, 1.0f),
            Vec4f(-0.02, -0.02, 0.02, 1.0f),
        }));
        rigidBody3.label = "RB3";
        rigidBodies.push_back(rigidBody3);

        RigidBody rigidBody4 = RigidBody(std::vector<Vec4f>({
            Vec4f(0.04, 0.03, 0.02, 1.0f),
            Vec4f(-0.04, 0.04, 0.02, 1.0f),
            Vec4f(0.02, 0.0, 0.02, 1.0f),
            Vec4f(-0.02, -0.02, 0.02, 1.0f),
        }));
        rigidBody4.label = "RB4";
        rigidBodies.push_back(rigidBody4);


        {
            RigidBody rigidBody = RigidBody(std::vector<Vec4f>({
                Vec4f(-0.058, 0.059, 0.00, 1.0f),
                Vec4f(-0.0625, -0.042, 0.00, 1.0f),
                Vec4f(0.0625, -0.042, 0.00, 1.0f),
            }));
            rigidBody.label = "Rigid Body 1";
            rigidBodies.push_back(rigidBody);
        }
*/

        {
            RigidBody rigidBody(std::vector<Vec4f>{
                Vec4f(-0.050063f,  0.005115f, -0.041902f, 1.0f), // Marker1
                Vec4f( 0.035620f,  0.002300f, -0.043965f, 1.0f), // Marker2
                Vec4f( 0.064371f, -0.003887f,  0.017008f, 1.0f), // Marker3
                Vec4f(-0.049928f, -0.003528f,  0.068859f, 1.0f), // Marker4
            });
            rigidBody.label = "Hand Left";
            rigidBody.isTracked = true;
            rigidBodies.push_back(rigidBody);
        }

        {
            RigidBody rigidBody(std::vector<Vec4f>{
                Vec4f( 0.008051f,  0.009128f, -0.039792f, 1.0f), // Marker1
                Vec4f(-0.026035f,  0.012265f, -0.049442f, 1.0f), // Marker2
                Vec4f(-0.052489f, -0.012464f,  0.055698f, 1.0f), // Marker3
                Vec4f( 0.070473f, -0.008928f,  0.033536f, 1.0f), // Marker4
            });
            rigidBody.label = "Hand Right";
            rigidBody.isTracked = true;
            rigidBodies.push_back(rigidBody);
        }

        {
            RigidBody rigidBody(std::vector<Vec4f>{
                Vec4f(-0.103488f,  0.062090f,  0.025943f, 1.f),  // Marker1
                Vec4f(-0.052044f,  0.017044f,  0.070950f, 1.f),  // Marker2
                Vec4f(-0.093307f, -0.051774f,  0.057748f, 1.f),  // Marker3
                Vec4f( 0.134210f, -0.024087f, -0.052542f, 1.f),  // Marker4
                Vec4f( 0.114630f, -0.003274f, -0.102099f, 1.f)   // Marker5
            });
            rigidBody.label = "Head";
            rigidBody.isTracked = true;
            rigidBodies.push_back(rigidBody);
        }
    }
};
