#pragma once

#include "src/util/math/Mat4.h"

/**
 * Represents a bone of a skeleton.
 */
struct Bone {
    /**
     * Name of the bone.
     */
    std::string name;

    /**
     * Default world pos of this bone (bind pose).
     */
    Vec4f defaultPos;

    /**
     * Transforms a vertex from model space into bone space (bind pose).
     */
    Mat4f offsetMatrix;

    /**
     * Current relative transform from animation.
     */
    Mat4f animatedRelativeTransform;

    /**
     * Final transformation matrix to apply animation to a vertex.
     */
    Mat4f finalTransform;

    /**
     * All the children of this bone.
     */
    int parentID = -1;

    /**
     * @brief display
     */
    bool display = true;
};
