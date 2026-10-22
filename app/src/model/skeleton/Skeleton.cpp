#include "src/model/skeleton/Skeleton.h"


int Skeleton::findBoneIdByName(std::string name){
    int i=0;
    for(Bone& bone : bones){
        if(bone.name == name){
            return i;
        }
        ++i;
    }

    return -1;
}
