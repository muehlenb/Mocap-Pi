#pragma once

#include "src/model/DataModel.h"

/**
 * Generates a TickData object for the pipeline.
 */
class TickProvider {
private:
    DataModel& model;

public:
    TickProvider(DataModel& model)
        : model(model){}

    TickData generateTick(double timestamp){
        TickData tick;
        tick.timestamp = timestamp;

        // Add rigid bodies of the scene:
        for(RigidBody& body : model.rigidBodies){
            if(body.isTracked)
                tick.allActiveRigidBodies.push_back(&body);
        }

        // Add skeletons of the scene:
        for(Skeleton& skeleton : model.skeletons){
            if(skeleton.isTracked){
                tick.activeSkeletons.push_back(&skeleton);

                for(SkeletalRigidBody& skeletalBody : skeleton.rigidBodies){
                    skeletalBody.rigidBody.isTracked = true;
                    tick.allActiveRigidBodies.push_back(&skeletalBody.rigidBody);
                }
            }
        }

        return tick;
    }
};
