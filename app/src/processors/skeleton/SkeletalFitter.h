#pragma once

#include <vector>
#include "src/model/DataModel.h"

#include "src/processors/Processor.h"

#include "src/model/skeleton/Skeleton.h"
#include "src/util/animation/Animation.h"

using namespace std::chrono;

class SkeletalFitter : public Processor {
public:
    float currentTime = 0;
    Skeleton skeleton;
    Animation animation;

    steady_clock::time_point startTime;

    SkeletalFitter(DataModel& model)
        : Processor(model){
        startTime = steady_clock::now();
    }

    virtual void process(TickData& tick) override {
        if(model.skeletons.size() > 0){
            Skeleton& skeleton = model.skeletons[0];

            for(SkeletalRigidBody& skeletalBody : skeleton.rigidBodies){
                if(skeletalBody.rigidBody.bodyFound){
                    Bone& bone = skeleton.bones[skeletalBody.boneID];
                    bone.animatedRelativeTransform = skeletalBody.rigidBody.pose;

                    bone.finalTransform = (bone.animatedRelativeTransform);
                    bone.display = true;
                }
            }
        }

        // Calculate final transforms:
        for(Bone& bone : skeleton.bones){
            Mat4f animatedTransform = bone.animatedRelativeTransform;

            bone.finalTransform = (animatedTransform * bone.offsetMatrix);
        }

    }
};
