#pragma once

#include <string>

#include <imgui_internal.h>
#include <imgui.h>

#include <vector>

#define IMGUI_BUTTONCOL_SELECTED ImVec4(0.259f, 0.588f, 0.98f, 1.f)
#define IMGUI_BUTTONCOL_SELECTED_HOVER ImVec4(0.259f, 0.588f, 0.98f, 1.f)

class SwitchUI {
public:
    static void render(std::vector<const char*> entries, int& active){

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
        int regAvailableX = ImGui::GetContentRegionAvail().x;

        for(int i = 0; i < entries.size(); ++i){
            bool isActive = i == active;
            if(isActive){
                ImGui::PushStyleColor(ImGuiCol_Button, IMGUI_BUTTONCOL_SELECTED);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IMGUI_BUTTONCOL_SELECTED_HOVER);
            }
            if(ImGui::Button(entries[i], ImVec2(regAvailableX / entries.size(), 24))){
                active = i;
            }
            if(isActive){
                ImGui::PopStyleColor(2);
            }

            if(i + 1 < entries.size())
                ImGui::SameLine();
        }

        ImGui::PopStyleVar(1);
    }
};
