#include "SkeletonTemplates.h"

#include "src/util/skeleton/SkeletonLoader.h"

int SkeletonTemplates::selectedSkeletonTemplate = 0;
const char* SkeletonTemplates::skeletonTemplateNames[] = {"X-Bot (Marker Config A)"};
unsigned int SkeletonTemplates::skeletonTemplateNum = 1;


Skeleton SkeletonTemplates::getXBotSkeletonTemplateA(){
    std::pair<Skeleton, Animation> loadedSkeleton = SkeletonLoader::load(CMAKE_SOURCE_DIR "/data/skeletal_test/BellyDance.fbx");
    Skeleton skeleton = loadedSkeleton.first;

    {
        int boneID = skeleton.findBoneIdByName("Spine2");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.05,1.420392,-0.142816),
            /*bone.offsetMatrix * */Vec4f(-0.05,1.420392,-0.142816),
            /*bone.offsetMatrix * */Vec4f(-0.0,1.420392,-0.142816),
            /*bone.offsetMatrix * */Vec4f(-0.03,1.382135,-0.131135)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("RightHand");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(-0.742945,1.476186,-0.085037),
            /*bone.offsetMatrix * */Vec4f(-0.742565,1.471845,-0.035227),
            /*bone.offsetMatrix * */Vec4f(-0.77249,1.470987,-0.085265),
            /*bone.offsetMatrix * */Vec4f(-0.772186,1.467514,-0.045417)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("LeftHand");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.743332,1.476997,-0.075484),
            /*bone.offsetMatrix * */Vec4f(0.743332,1.473511,-0.035637),
            /*bone.offsetMatrix * */Vec4f(0.772876,1.469192,-0.046053),
            /*bone.offsetMatrix * */Vec4f(0.772876,1.471807,-0.075938)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("RightFoot");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(-0.070125,0.096548,0.031871),
            /*bone.offsetMatrix * */Vec4f(-0.072501,0.076722,0.06653),
            /*bone.offsetMatrix * */Vec4f(-0.119486,0.063466,0.055727),
            /*bone.offsetMatrix * */Vec4f(-0.106029,0.081808,0.020978)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("LeftFoot");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.066779,0.095111,0.031357),
            /*bone.offsetMatrix * */Vec4f(0.104367,0.084506,0.022714),
            /*bone.offsetMatrix * */Vec4f(0.116419,0.069204,0.047626),
            /*bone.offsetMatrix * */Vec4f(0.07032,0.078243,0.067453)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("Spine2");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(-0.04,1.445906,0.065629),
            /*bone.offsetMatrix * */Vec4f(-0.0,1.445906,0.065629),
            /*bone.offsetMatrix * */Vec4f(0.04,1.445906,0.065629),
            /*bone.offsetMatrix * */Vec4f(0.02,1.41617,0.092383)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("Head");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.01,1.726704,0.120329),
            /*bone.offsetMatrix * */Vec4f(0.05,1.73663,0.119114),
            /*bone.offsetMatrix * */Vec4f(-0.01,1.73663,0.119114),
            /*bone.offsetMatrix * */Vec4f(0.02,1.756482,0.116684)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("Head");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.029397,1.789364,-0.09593),
            /*bone.offsetMatrix * */Vec4f(-0.000032,1.794758,-0.075809),
            /*bone.offsetMatrix * */Vec4f(-0.038218,1.773996,-0.086335),
            /*bone.offsetMatrix * */Vec4f(0.010529,1.772994,-0.109197)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("Hips");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(-0.17,1.04404,-0.084448),
            /*bone.offsetMatrix * */Vec4f(-0.17,1.003527,-0.044969),
            /*bone.offsetMatrix * */Vec4f(-0.17,1.043653,-0.054451),
            /*bone.offsetMatrix * */Vec4f(-0.17,1.043136,-0.014454)
        })));
    }
    {
        int boneID = skeleton.findBoneIdByName("Hips");
        Bone& bone = skeleton.bones[boneID];
        skeleton.rigidBodies.push_back(SkeletalRigidBody(boneID, RigidBody(std::vector<Vec4f>{
            /*bone.offsetMatrix * */Vec4f(0.17,1.04404,-0.044449),
            /*bone.offsetMatrix * */Vec4f(0.17,1.033395,0.005418),
            /*bone.offsetMatrix * */Vec4f(0.17,1.043007,0.035545),
            /*bone.offsetMatrix * */Vec4f(0.17,1.003527,-0.004969)
        })));
    }

    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftShoulder"), Vec4f(0.166945,1.519953,-0.057879)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftArm"), Vec4f(0.401628,1.440613,-0.105485)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightShoulder"), Vec4f(-0.166942,1.519952,-0.057878)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightArm"), Vec4f(-0.401628,1.440613,-0.105485)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightUpLeg"), Vec4f(-0.162078,0.595244,0.00251)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftUpLeg"), Vec4f(0.162078,0.595243,0.002519)));

    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightUpLeg"), Vec4f(-0.072078,0.595945,0.092507)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftUpLeg"), Vec4f(0.072078,0.595947,0.092516)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightHandIndex4"), Vec4f(-0.887637,1.445434,-0.032898)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightHandPinky4"), Vec4f(-0.862256,1.445745,-0.093694)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightHandThumb4"), Vec4f(-0.803998,1.384276,0.019953)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightArm"), Vec4f(-0.401628,1.490613,-0.065485)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftArm"), Vec4f(0.401628,1.490613,-0.055485)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftToeBase"), Vec4f(0.060547,0.052876,0.148288)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("RightToeBase"), Vec4f(-0.070957,0.044371,0.151758)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftHandPinky4"), Vec4f(0.862364,1.445744,-0.093293)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftHandIndex4"), Vec4f(0.887673,1.445432,-0.032893)));
    skeleton.markers.push_back(SkeletalMarker(skeleton.findBoneIdByName("LeftHandThumb4"), Vec4f(0.804055,1.384098,0.020108)));

    return skeleton;
}

