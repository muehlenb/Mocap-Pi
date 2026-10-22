#pragma once

#include "src/model/primitive/RigidBody.h"

/**
 * Represents a marker that is attached to a skeleton.
 */
struct SkeletalRigidBody {
    /**
     * Marker ID.
     */
    int boneID = -1;

    /**
     * Rigid Body
     */
    RigidBody rigidBody;

    SkeletalRigidBody(int boneID, RigidBody rigidBody)
        : boneID(boneID)
        , rigidBody(rigidBody){}
};
