#pragma once

#define IMGUI_HEADCOL_ABORT ImVec4(0.5f, 0.2f, 0.2f, 1.f)
#define IMGUI_HEADCOL_ABORT_HOVER ImVec4(0.65f, 0.3f, 0.3f, 1.f)

#define IMGUI_HEADCOL_CALIB_UNUSABLE ImVec4(0.7f, 0.0f, 0.0f, 1.f)
#define IMGUI_HEADCOL_CALIB_VERY_BAD ImVec4(0.7f, 0.2f, 0.0f, 1.f)
#define IMGUI_HEADCOL_CALIB_BAD ImVec4(0.7f, 0.4f, 0.0f, 1.f)
#define IMGUI_HEADCOL_CALIB_SUFFICIENT ImVec4(0.7f, 0.7f, 0.0f, 1.f)
#define IMGUI_HEADCOL_CALIB_GOOD ImVec4(0.4f, 0.7f, 0.0f, 1.f)
#define IMGUI_HEADCOL_CALIB_VERY_GOOD ImVec4(0.0f, 0.7f, 0.0f, 1.f)

#include <imgui_internal.h>
#include <imgui.h>

#include "src/processors/ProcessorCollection.h"

struct CalibrationPaneUI {

    static ImVec4 mapObservationNumToColor(int num){
        if(num < 80)
            return IMGUI_HEADCOL_CALIB_UNUSABLE;
        else if(num < 150)
            return IMGUI_HEADCOL_CALIB_VERY_BAD;
        else if(num < 300)
            return IMGUI_HEADCOL_CALIB_BAD;
        else if(num < 500)
            return IMGUI_HEADCOL_CALIB_SUFFICIENT;
        else if(num < 8000)
            return IMGUI_HEADCOL_CALIB_GOOD;
        else
            return IMGUI_HEADCOL_CALIB_VERY_GOOD;

    }

    static void render(DataModel& model, ProcessorCollection& processors){
        if(ImGui::CollapsingHeader("Calibration", ImGuiTreeNodeFlags_DefaultOpen)){
            ImGui::Text("Cameras: %i ", int(processors.cameraCalibrator->calibrations.size()));
            ImGui::Separator();

            if(processors.cameraCalibrator->isOptimizingCalibration()){
                ImGui::Text("Optimizing...");
                ImGui::TextWrapped("TODO: Progress view and control.");

            } else if(processors.cameraCalibrator->getIsCollectingObservations()){
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0,255,0,255));
                ImGui::TextWrapped("Please move the calibration stick. Observations are recorded now!");
                ImGui::PopStyleColor();
                ImGui::Separator();
                std::map<int, std::vector<std::pair<int, ImageMarker>>>& observationCounts = processors.cameraCalibrator->getObservations();
                ImGui::Text("Observations:");
                for(auto& observationEntry : observationCounts){
                    int observationCount = int(observationEntry.second.size());

                    ImGui::Text("    Cam %i: ", observationEntry.first);
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, mapObservationNumToColor(observationCount));
                    ImGui::Text("%i", observationCount);
                    ImGui::PopStyleColor();
                }

                ImGui::Separator();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::Combo("##1", &CalibrationSticks::selectedCalibStick, CalibrationSticks::calibrationStickNames, CalibrationSticks::calibrationStickNum);

                ImGui::Separator();
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
                int regionAvailX = ImGui::GetContentRegionAvail().x / 2;
                if(ImGui::Button("Finalize!", ImVec2(regionAvailX, 24))){
                    processors.cameraCalibrator->triggerCalibration();
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, IMGUI_HEADCOL_ABORT);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IMGUI_HEADCOL_ABORT_HOVER);
                if(ImGui::Button("Abort", ImVec2(regionAvailX, 24))){
                    processors.cameraCalibrator->abortObservationCollection();
                }
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(1);
            } else {
                if(model.cameras.size() > 0){
                    if (ImGui::BeginTable("Calibration Precision", 3))
                    {
                        ImGui::Text("Calibration Error");
                        ImGui::Separator();
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Camera");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("2D Proj. (px)");
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("3D (mm/m)");

                        for (auto& cameraEntry : model.cameras)
                        {
                            Camera& camera = cameraEntry.second;
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("%i (n=%i)", cameraEntry.first, camera.calibration.calib_observation_num);
                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%.3f", camera.calibration.calib_reprojection_error);
                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%.3f", camera.calibration.calib_distance_error * 1000);

                        }
                        ImGui::EndTable();
                    }
                }
                ImGui::Separator();
                if(ImGui::Button("Start new calibration", ImVec2(ImGui::GetContentRegionAvail().x, 24))){
                    processors.cameraCalibrator->startObservationCollection();
                }
            }

            ImGui::Separator();
            ImGui::Text("");
            ImGui::Separator();
        }
    }
};
