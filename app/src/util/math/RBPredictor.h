#pragma once

#include "src/util/math/Vec4.h"
#include "src/util/math/Quat.h"

/**
 * Class that quite simply predicts the movement of rigid bodys.
 */
class RBPredictor {
    Vec4f velocity = Vec4f(0.0, 0.0, 0.0, 0.0);
    Quat<float> quatVelocity;

    float prevTime = 0.f;
    Mat4f prevPose;

    void integrateRotation(Mat4f pose, float time){
        Quat<float> currentQ(pose);
        Quat<float> prevQ(prevPose);

        Quat<float> deltaQ = currentQ * prevQ.inverse();

        float dt = time - prevTime;
        if(dt > 1e-6f){
            // Quaternion now:
            Quat<float> now = Quat<float>::slerp(Quat<float>(), deltaQ, 1.0f / dt);

            // Smooth angle velocity a little bit:
            quatVelocity = Quat<float>::slerp(quatVelocity, now, 0.5f).normalized();
        }
    }

    void integrateMovement(Mat4f pose, float time){
        float dt = time - prevTime;
        Vec4f now = (pose.getPosition() - prevPose.getPosition()) * (1.f/dt);

        velocity = velocity * 0.5 + now * 0.5;
    }

public:
    Mat4f prediction;

    void predict(Mat4f pose, float time){
        float dt = time - prevTime;

        if (dt < 0)
            dt = 0;

        Quat<float> deltaRotation = Quat<float>::slerp(Quat<float>(), quatVelocity, dt);
        Vec4f newPos = pose.getPosition() + velocity * dt * 0.001;

        Mat4f newTransformation = (Quat<float>(pose) * deltaRotation).rotationMatrix();
        newTransformation.data[12] = newPos.x;
        newTransformation.data[13] = newPos.y;
        newTransformation.data[14] = newPos.z;

        prediction = newTransformation;
    }

    void integrate(Mat4f pose, float time){
        if(prevTime > 0){
            integrateRotation(pose, time);
            integrateMovement(pose, time);
        }

        prediction = pose;

        prevPose = pose;
        prevTime = time;
    }
};
