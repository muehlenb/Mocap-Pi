#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "src/model/DataModel.h"
#include "src/util/gl/StaticMesh.h"
#include "src/util/gl/Shader.h"

#include "src/util/gl/TextRenderer.h"

struct SkeletonRenderer {
    std::shared_ptr<StaticMesh> boneMesh;
    std::shared_ptr<StaticMesh> sphereMesh;
    std::shared_ptr<Shader> unlitColorShader;

    SkeletonRenderer(){
        boneMesh = std::make_shared<StaticMesh>(CMAKE_SOURCE_DIR "/data/model/bone.obj");
        sphereMesh = std::make_shared<StaticMesh>(CMAKE_SOURCE_DIR "/data/model/sphere_low.obj");
        unlitColorShader = std::make_shared<Shader>(CMAKE_SOURCE_DIR "/shader/unlitColor.vert", CMAKE_SOURCE_DIR "/shader/unlitColor.frag");
    }

    void render(Mat4f projection, Mat4f view, DataModel& dataModel){
        for(Skeleton& skeleton : dataModel.skeletons){

            for(Bone& bone : skeleton.bones){
                int parentBoneID = bone.parentID;

                if(parentBoneID != -1 && skeleton.bones[parentBoneID].display){
                    Bone& parentBone = skeleton.bones[parentBoneID];

                    Vec4f parentPos = parentBone.defaultPos;
                    Vec4f bonePos = bone.defaultPos;

                    Mat4f relativeWorldTransform = Mat4f::getRotationAndScaleMatrix(Vec4f(0,-1,0), bonePos - parentPos);
                    Mat4f worldTranslation = Mat4f::translation(parentPos.x, parentPos.y, parentPos.z);

                    Mat4f model = parentBone.finalTransform * (worldTranslation * relativeWorldTransform);

                    unlitColorShader->bind();
                    unlitColorShader->setUniform("color", Vec4f(0.75f,0.75f,0.75f));
                    unlitColorShader->setUniform("projection", projection);
                    unlitColorShader->setUniform("view", view);
                    unlitColorShader->setUniform("model", model);

                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    boneMesh->render(GL_TRIANGLES);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }

            for(SkeletalMarker& marker : skeleton.markers){
                Bone* bone = &skeleton.bones[marker.boneID];

                //if(bone->parentID != -1)
                //    bone = &skeleton.bones[bone->parentID];

                Vec4f animPos = bone->finalTransform * marker.position;

                unlitColorShader->bind();
                unlitColorShader->setUniform("color", Vec4f(0.5f,0.5f,1.0f));
                unlitColorShader->setUniform("projection", projection);
                unlitColorShader->setUniform("view", view);
                unlitColorShader->setUniform("model", Mat4f::translation(animPos.x, animPos.y, animPos.z) * Mat4f::scale(0.008f));

                //std::cout << animPos << std::endl;

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                sphereMesh->render(GL_TRIANGLES);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            for(SkeletalRigidBody& skeletalBody : skeleton.rigidBodies){
                Bone& bone = skeleton.bones[skeletalBody.boneID];
                RigidBody& body = skeletalBody.rigidBody;

                if(!bone.display)
                    continue;

                if(!body.bodyFound)
                    continue;

                for(RigidBodyMarker& marker : body.markers){
                    Vec4f animPos = bone.finalTransform * marker.position;

                    unlitColorShader->bind();
                    unlitColorShader->setUniform("color", Vec4f(0.25f,0.65f,0.65f));
                    unlitColorShader->setUniform("projection", projection);
                    unlitColorShader->setUniform("view", view);
                    unlitColorShader->setUniform("model", Mat4f::translation(animPos.x, animPos.y, animPos.z) * Mat4f::scale(0.008f));

                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    sphereMesh->render(GL_TRIANGLES);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                }
            }
        }

    }
};
