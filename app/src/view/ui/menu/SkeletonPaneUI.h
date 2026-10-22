#pragma once

#include <imgui_internal.h>
#include <imgui.h>

#include "src/processors/ProcessorCollection.h"
#include "src/util/skeleton/SkeletonTemplates.h"

struct SkeletonPaneUI {
    static int selectedItem;

    static void render(DataModel& model, ProcessorCollection& processors){

        ImGui::TextWrapped("WARNING: Skeletal tracking is WORK IN PROGRESS and does not work yet.");
        ImGui::Separator();

        ImGui::Text("Add Skeleton:");
        ImGui::Separator();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.81f);
        ImGui::Combo("##1", &SkeletonTemplates::selectedSkeletonTemplate, SkeletonTemplates::skeletonTemplateNames, SkeletonTemplates::skeletonTemplateNum);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.18f);
        if(ImGui::Button("Add")){
            model.semaphore.acquire();
            int uniqueId = 0;
            while(true){
                bool foundSameName = false;
                for(Skeleton& skeleton : model.skeletons){
                    if(skeleton.name == "Skeleton " + std::to_string(uniqueId)){
                        foundSameName = true;
                        break;
                    }
                }
                if(!foundSameName)
                    break;
                ++uniqueId;
            }
            Skeleton skeleton = SkeletonTemplates::buildSelectedSkeletonInstance("Skeleton " + std::to_string(uniqueId));
            model.skeletons.push_back(skeleton);
            model.semaphore.release();
        }
        ImGui::Text("");
        ImGui::Separator();

        if(ImGui::CollapsingHeader("Skeletons", ImGuiTreeNodeFlags_DefaultOpen)){
            int removeSkeleton = -1;
            for (int i = 0; i < model.skeletons.size(); i++) {
                // The selectable will return true if this item is clicked
                ImGui::Checkbox(("##skeleton_"+std::to_string(i)).c_str(), &model.skeletons[i].isTracked);
                ImGui::SameLine();
                if(ImGui::Button(("x##skeleton_"+std::to_string(i)).c_str())){
                    removeSkeleton = i;
                }
                ImGui::SameLine();

                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.8f);
                if (ImGui::Selectable(model.skeletons[i].name.c_str(), selectedItem == i)) {
                    selectedItem = i;  // Set the current selection to this item
                }

                // Add a separator after each item except the last one
                if (i < model.rigidBodies.size() - 1) {
                    ImGui::Separator();
                }
            }

            if(removeSkeleton != -1){
                model.semaphore.acquire();
                model.skeletons.erase(model.skeletons.begin() + removeSkeleton);
                model.semaphore.release();
            }
        }
    }
};

int SkeletonPaneUI::selectedItem = 0;
