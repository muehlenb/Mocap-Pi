#pragma once

#include "src/model/skeleton/Bone.h"
#include "src/model/skeleton/SkeletalMarker.h"
#include "src/model/skeleton/SkeletalRigidBody.h"

/**
 * Represents a skeleton (for skeletal mesh animation).
 */
struct Skeleton {
    /**
     * Skeletons name / label
     */
    std::string name;

    /**
     * The bones of this skeleton.
     */
    std::vector<Bone> bones;

    /**
     * The markers of this skeleton.
     */
    std::vector<SkeletalMarker> markers;

    /**
     * The rigid bodies of this skeleton.
     */
    std::vector<SkeletalRigidBody> rigidBodies;

    /**
     * Switch on/off tracking of the skeleton.
     */
    bool isTracked;

    /**
     * Gets bone ID by name.
     */
    int findBoneIdByName(std::string name);
};
