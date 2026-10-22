#pragma once

#include "src/util/math/Vec4.h"
#include "src/util/math/Quat.h"
#include "src/model/skeleton/Skeleton.h"
#include "src/model/skeleton/Bone.h"

template<typename T>
struct Keyframe {
    T value;
    float time;

    Keyframe(T value, float time) : value(value), time(time) {};
};

/**
 * An animation channel.
 *
 * Based on: https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
 */
struct AnimChannel {
    std::string name = "";
    std::vector<Keyframe<Vec4f>> positions;
    std::vector<Keyframe<Quatf>> rotations;
    std::vector<Keyframe<Vec4f>> scales;

    /**
     * Returns the transformation matrix at the given time.
     */
    Mat4f getTransformation(float animationTime){
        Mat4f translation = positions.size() > 0 ? interpolatePosition(animationTime) : Mat4f();
        Mat4f rotation = rotations.size() > 0 ? interpolateRotation(animationTime) : Mat4f();
        Mat4f scale = scales.size() > 0 ? interpolateScaling(animationTime) : Mat4f();
        return translation * rotation * scale;
    }

    /**
     * Returns the current index in the given list for the given time.
     */
    template<typename T>
    inline int getIndex(std::vector<Keyframe<T>>& list, float animationTime){
        int index = 0;

        // As long as index is smaller listSize-1 and animTime is smaller than keyframe:
        while(index < list.size() - 1 && list[index].time < animationTime){
            ++index;
        }

        return std::max(0, index-1);
    }

    /**
     * Returns the relative progress between two frames with the given timestamps
     * `lastTime` and `nextTime`.
     */
    inline float getRelativeProgress(float lastTime, float nextTime, float animationTime){
        return (animationTime - lastTime) / (nextTime - lastTime);
    }

    Mat4f interpolatePosition(float animationTime){
        int p0Idx = getIndex(positions, animationTime);
        int p1Idx = p0Idx + 1;

        // If it's the last index, directly return it:
        if (p0Idx == positions.size() - 1)
            return Mat4f::translation(positions[p0Idx].value.x, positions[p0Idx].value.y,positions[p0Idx].value.z);

        float progress = getRelativeProgress(positions[p0Idx].time, positions[p1Idx].time, animationTime);
        Vec4f finalPosition = positions[p0Idx].value * (1-progress) + positions[p1Idx].value * progress;

        return Mat4f::translation(finalPosition.x, finalPosition.y, finalPosition.z);
    }

    Mat4f interpolateRotation(float animationTime){
        int p0Idx = getIndex(rotations, animationTime);
        int p1Idx = p0Idx + 1;

        // If it's the last index, directly return it:
        if (p0Idx == rotations.size() - 1)
            return rotations[p0Idx].value.rotationMatrix();

        float progress = getRelativeProgress(rotations[p0Idx].time, rotations[p1Idx].time, animationTime);

        Quatf finalRotation = Quatf::slerp(rotations[p0Idx].value, rotations[p1Idx].value, progress).normalized();
        return finalRotation.rotationMatrix();
    }

    Mat4f interpolateScaling(float animationTime){
        int p0Idx = getIndex(scales, animationTime);
        int p1Idx = p0Idx + 1;

        // If it's the last index, directly return it:
        if (p0Idx == scales.size() - 1)
            return Mat4f::scale(scales[p0Idx].value.x, scales[p0Idx].value.y, scales[p0Idx].value.z);

        float progress = getRelativeProgress(scales[p0Idx].time, scales[p1Idx].time, animationTime);

        Vec4f finalScale = scales[p0Idx].value * (1-progress) + scales[p1Idx].value * progress;
        return Mat4f::scale(finalScale.x, finalScale.y, finalScale.z);
    }
};

/**
 * Represents an animation.
 */
class Animation {
public:
    std::vector<AnimChannel> channels;
    float duration = 0;

    /**
     * Updates the bone of the channel.
     */
    void apply(Skeleton& skeleton, float animationTime){
        std::map<std::string, Bone*> boneByName;

        for(Bone& bone : skeleton.bones){
            boneByName.insert(std::pair<std::string, Bone*>(bone.name, &bone));
        }

        for(AnimChannel& channel : channels){
            if(boneByName.count(channel.name) > 0){
                boneByName[channel.name]->animatedRelativeTransform = channel.getTransformation(animationTime);
            }
        }
    }
};
