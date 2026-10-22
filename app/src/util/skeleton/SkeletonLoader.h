#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "src/util/animation/Animation.h"
#include "src/model/skeleton/Skeleton.h"
#include "src/model/skeleton/SkeletalMarker.h"

struct SkeletonLoader {
    static aiNode* FindBoneNode(aiNode* rootNode, const std::string& boneName) {
        if (rootNode->mName.C_Str() == boneName) {
            return rootNode;
        }

        for (unsigned int i = 0; i < rootNode->mNumChildren; i++) {
            aiNode* found = FindBoneNode(rootNode->mChildren[i], boneName);
            if (found) {
                return found;
            }
        }
        return nullptr;
    }

    static int FindParentBoneIdx(const aiMesh* mesh, const aiNode* boneNode) {
        if (!boneNode) {
            return -1;
        }

        const aiNode* parentNode = boneNode->mParent;
        while (parentNode) {
            for (unsigned int j = 0; j < mesh->mNumBones; j++) {
                if (std::string(mesh->mBones[j]->mName.C_Str()) == parentNode->mName.C_Str()) {
                    return j;
                }
            }
            parentNode = parentNode->mParent;
        }
        return -1;
    }

    static std::string extractAfterColon(const aiString& ai_str) {
        std::string str = ai_str.C_Str();  // Konvertiert aiString in std::string
        size_t colonPos = str.find(':');

        if (colonPos != std::string::npos) {
            // ":" gefunden, extrahieren Sie den Teil nach dem ":"
            return str.substr(colonPos + 1);
        } else {
            // Kein ":" gefunden, verwenden Sie den gesamten String
            return str;
        }
    }

    static std::pair<Skeleton, Animation> load(std::string path){
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        }

        // Load the first skeleton:
        Skeleton skeleton;
        for(int mID = 0; mID < scene->mNumMeshes; ++mID){
            aiMesh* mesh = scene->mMeshes[mID];

            if (mesh->mNumBones > 0) {
                for (unsigned int i = 0; i < mesh->mNumBones; i++) {
                    aiBone* bone = mesh->mBones[i];

                    const aiNode* boneNode = FindBoneNode(scene->mRootNode, bone->mName.C_Str());
                    int parentBoneIdx = FindParentBoneIdx(mesh, boneNode);

                    Bone myBone;
                    myBone.name = extractAfterColon(bone->mName);
                    myBone.offsetMatrix = Mat4f::fromAssimp(bone->mOffsetMatrix).scaleTranslation(0.01f);
                    myBone.defaultPos = myBone.offsetMatrix.inverse().getPosition();
                    myBone.parentID = parentBoneIdx;

                    skeleton.bones.push_back(myBone);
                }

                break;
            }
        }

        // Load the first animation:
        Animation animation;
        for(int aID = 0; aID < scene->mNumAnimations; ++aID){
            aiAnimation* aiAnimation = scene->mAnimations[aID];

            if (aiAnimation->mNumChannels > 0) {
                for(int cID = 0; cID < aiAnimation->mNumChannels; ++cID){
                    aiNodeAnim* nodeAnim = aiAnimation->mChannels[cID];
                    AnimChannel animChannel;
                    animChannel.name = extractAfterColon(nodeAnim->mNodeName);

                    for(int kID = 0; kID < nodeAnim->mNumPositionKeys; ++kID){
                        aiVector3D v = nodeAnim->mPositionKeys[kID].mValue;
                        float time = nodeAnim->mPositionKeys[kID].mTime / aiAnimation->mTicksPerSecond;
                        animChannel.positions.push_back(Keyframe<Vec4f>(Vec4f(v.x, v.y, v.z) / 100.f, time));
                        animation.duration = std::max(animation.duration, time);
                    }

                    for(int kID = 0; kID < nodeAnim->mNumRotationKeys; ++kID){
                        aiQuaternion q = nodeAnim->mRotationKeys[kID].mValue;
                        float time = nodeAnim->mRotationKeys[kID].mTime / aiAnimation->mTicksPerSecond;
                        animChannel.rotations.push_back(Keyframe<Quatf>(Quatf(q.x, q.y, q.z, q.w), time));
                        animation.duration = std::max(animation.duration, time);
                    }

                    for(int kID = 0; kID < nodeAnim->mNumScalingKeys; ++kID){
                        aiVector3D v = nodeAnim->mScalingKeys[kID].mValue;
                        float time = nodeAnim->mScalingKeys[kID].mTime / aiAnimation->mTicksPerSecond;
                        animChannel.scales.push_back(Keyframe<Vec4f>(Vec4f(v.x, v.y, v.z), time));
                        animation.duration = std::max(animation.duration, time);
                    }

                    animation.channels.push_back(animChannel);
                }
                break;
            }
        }

        return std::pair<Skeleton, Animation>(skeleton, animation);
    }
};
