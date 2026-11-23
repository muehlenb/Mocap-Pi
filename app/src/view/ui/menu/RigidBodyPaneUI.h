
#pragma once

#include <imgui_internal.h>
#include <imgui.h>

#include "src/processors/ProcessorCollection.h"

#include <string>
#include <cstdio>
#include <cstring>
#include <algorithm>

struct RigidBodyPaneUI {

    static void render(DataModel& model, ProcessorCollection& processors) {
        static int editingBody = -1;
        static bool focusNameInput = false;
        static char nameBuffer[256] = {};

        if (!ImGui::CollapsingHeader("Rigid Bodies", ImGuiTreeNodeFlags_DefaultOpen)) return;

        // Find the currently selected rigid body by pointer.
        // Store the index so vector reallocations can be handled safely.
        int selectedBody = -1;

        for (int i = 0; i < static_cast<int>(model.rigidBodies.size()); ++i) {
            if (model.redefineRigidBody == &model.rigidBodies[i]) {
                selectedBody = i;
                break;
            }
        }

        if (ImGui::Button("+ Add Rigid Body", ImVec2(-1, 0))) {
            RigidBody body;
            body.label = "RigidBody " + std::to_string(model.rigidBodies.size() + 1);
            body.isTracked = false;
            model.rigidBodies.push_back(body);

            // Restore pointer after possible vector reallocation.
            model.redefineRigidBody = selectedBody >= 0 ? &model.rigidBodies[selectedBody] : nullptr;
        }

        ImGui::Spacing();

        int bodyToDelete = -1;

        for (int i = 0; i < static_cast<int>(model.rigidBodies.size()); ++i) {
            RigidBody& body = model.rigidBodies[i];
            ImGui::PushID(i);

            float frameHeight = ImGui::GetFrameHeight();
            float deleteWidth = frameHeight;
            float spacing = ImGui::GetStyle().ItemSpacing.x;

            // Tracking
            ImGui::Checkbox("##tracked", &body.isTracked);
            ImGui::SameLine();

            // Header area
            float headerStartX = ImGui::GetCursorPosX();
            float availableWidth = ImGui::GetContentRegionAvail().x;
            float deleteX = headerStartX + availableWidth - deleteWidth;

            bool open = false;

            if (editingBody == i) {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                open = ImGui::TreeNodeEx("##body");

                ImGui::SameLine();
                float inputWidth = std::max(1.0f, deleteX - ImGui::GetCursorPosX() - spacing);
                ImGui::SetNextItemWidth(inputWidth);

                if (focusNameInput) {
                    ImGui::SetKeyboardFocusHere();
                    focusNameInput = false;
                }

                bool confirmed = ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

                if (confirmed) {
                    if (std::strlen(nameBuffer) > 0) body.label = nameBuffer;
                    editingBody = -1;
                } else if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                    editingBody = -1;
                }

            } else {
                open = ImGui::TreeNodeEx("##body", ImGuiTreeNodeFlags_DefaultOpen, "%s", body.label.c_str());

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    editingBody = i;
                    focusNameInput = true;
                    std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", body.label.c_str());
                }
            }

            // Delete button
            ImGui::SameLine();
            ImGui::SetCursorPosX(deleteX);

            if (ImGui::Button("X", ImVec2(deleteWidth, frameHeight)))
                bodyToDelete = i;

            if (open) {
                ImGui::Indent();

                ImGui::TextDisabled("%d Markers", static_cast<int>(body.markers.size()));
                ImGui::Spacing();

                bool markersChanged = false;
                int markerToDelete = -1;

                // Marker positions
                for (int j = 0; j < static_cast<int>(body.markers.size()); ++j) {
                    RigidBodyMarker& marker = body.markers[j];
                    ImGui::PushID(j);

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextDisabled("%d", j);
                    ImGui::SameLine();

                    float position[3] = {marker.position.x, marker.position.y, marker.position.z};

                    float removeWidth = ImGui::GetFrameHeight();
                    float positionWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x - removeWidth - spacing);
                    ImGui::SetNextItemWidth(positionWidth);

                    if (ImGui::DragFloat3("##position", position, 0.1f, 0.0f, 0.0f, "%.3f")) {
                        marker.position.x = position[0];
                        marker.position.y = position[1];
                        marker.position.z = position[2];
                        marker.position.w = 1.0f;
                        markersChanged = true;
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("X", ImVec2(removeWidth, 0)))
                        markerToDelete = j;

                    ImGui::PopID();
                }

                // Remove marker
                if (markerToDelete >= 0) {
                    body.markers.erase(body.markers.begin() + markerToDelete);
                    markersChanged = true;
                }

                ImGui::Spacing();

                // Add marker
                if (ImGui::SmallButton("+ Add Marker")) {
                    body.markers.emplace_back(Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
                    markersChanged = true;
                }

                // Recalculate marker distances and centroid
                if (markersChanged) {
                    if (!body.markers.empty()) {
                        body.init();
                    } else {
                        body.averagePoint = Vec4f();
                        body.isTracked = false;
                    }
                }

                ImGui::Unindent();
                ImGui::TreePop();
            }

            ImGui::Separator();
            ImGui::PopID();
        }

        // Delete rigid body and repair selection.
        if (bodyToDelete >= 0) {
            if (selectedBody == bodyToDelete)
                selectedBody = -1;
            else if (selectedBody > bodyToDelete)
                --selectedBody;

            model.rigidBodies.erase(model.rigidBodies.begin() + bodyToDelete);
            model.redefineRigidBody = selectedBody >= 0 ? &model.rigidBodies[selectedBody] : nullptr;

            if (editingBody == bodyToDelete) {
                editingBody = -1;
                focusNameInput = false;
            } else if (editingBody > bodyToDelete) {
                --editingBody;
            }
        }

        // ---------------------------------------------------------
        // Rigid Body Redefinition
        // ---------------------------------------------------------

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextUnformatted("Rigid Body Redefinition");

        // Update selected index after potential deletion.
        selectedBody = -1;

        for (int i = 0; i < static_cast<int>(model.rigidBodies.size()); ++i) {
            if (model.redefineRigidBody == &model.rigidBodies[i]) {
                selectedBody = i;
                break;
            }
        }

        const char* selectedLabel = selectedBody >= 0 ? model.rigidBodies[selectedBody].label.c_str() : "None";

        ImGui::SetNextItemWidth(-1);

        if (ImGui::BeginCombo("##redefineRigidBody", selectedLabel)) {
            if (ImGui::Selectable("None", selectedBody == -1))
                model.redefineRigidBody = nullptr;

            for (int i = 0; i < static_cast<int>(model.rigidBodies.size()); ++i) {
                ImGui::PushID(i);

                bool selected = selectedBody == i;

                if (ImGui::Selectable(model.rigidBodies[i].label.c_str(), selected))
                    model.redefineRigidBody = &model.rigidBodies[i];

                if (selected)
                    ImGui::SetItemDefaultFocus();

                ImGui::PopID();
            }

            ImGui::EndCombo();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Select the rigid body to redefine.");

        // Definition mode
        ImGui::SetNextItemWidth(-1);

        const char* modeNames[] = {
            "Tool Tip Origin",
            "Centroid + PCA"
        };

        int mode = static_cast<int>(model.redefineRigidBodyMode);

        if (ImGui::Combo("##redefineMode", &mode, modeNames, IM_ARRAYSIZE(modeNames)))
            model.redefineRigidBodyMode = static_cast<RigidBodyDefinitionMode>(mode);

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip(
                "Tool Tip Origin: Estimate rotation pivot, align closest marker with Y.\n"
                "Centroid + PCA: Use marker centroid and principal component axes."
                );
    }
};
