#pragma once

#include <imgui_internal.h>
#include <imgui.h>
#include <imGuiAnsiColor.hpp>

#include "src/processors/ProcessorCollection.h"
#include "src/util/network/CameraNetworkUtils.h"

#include "src/view/ViewData.h"

#include "src/util/GuiUtils.h"

#include <sstream>


struct RemoteCamTask {
    std::string name;
    std::thread thread;
    std::stringstream log;

    bool showLog = true;
    bool isFinished = false;

    ~RemoteCamTask(){
        if(thread.joinable()){
            thread.join();
        }
    }
};

struct NetworkPaneUI {
    static int selectedInterfaceId;
    static std::vector<std::shared_ptr<RemoteCamTask>> tasks;

    static void render(DataModel& model, ProcessorCollection& processors){
        if(ImGui::CollapsingHeader("Network Interfaces", ImGuiTreeNodeFlags_DefaultOpen)){

            bool disabled = false;
            if(!CameraNetworkUtils::scanThreadFinished){
                ImGui::BeginDisabled(true);
                disabled = true;
            }

            // Network interface selector:
            {
                std::string itemsCombined = GuiUtils::VectorToString(CameraNetworkUtils::availableNetworkInterfaces);

                int activeInterfaceId = CameraNetworkUtils::findNetworkInterfaceId(Settings::instance.networkInterface);

                if(selectedInterfaceId == -1)
                    selectedInterfaceId = activeInterfaceId;

                ImGui::Combo("##NetworkInterface", &selectedInterfaceId, itemsCombined.c_str());

                // Change network interface
                if(selectedInterfaceId != activeInterfaceId){
                    // Change the interface
                    std::string interfaceAddr = CameraNetworkUtils::availableNetworkInterfaces[selectedInterfaceId];
                    Settings::instance.networkInterface = interfaceAddr;

                    std::cout << "Change to network interface " << interfaceAddr << std::endl;

                    // Reinit relevant processors:
                    processors.reinitNetworkNextFrame = true;
                }
            }

            if(disabled)
                ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::Text("");
            ImGui::Separator();
        }

        if(ImGui::CollapsingHeader("Camera Devices", ImGuiTreeNodeFlags_DefaultOpen)){
            for(RemoteCam& cam : CameraNetworkUtils::remoteCams){
                if(cam.isValid){
                    ImGui::Image(ViewData::getInstance().camIconTexture.texture, ImVec2(42,42));
                } else {
                    ImGui::Image(ViewData::getInstance().sshIconTexture.texture, ImVec2(42,42));
                }

                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("%s", cam.address.c_str());

                bool sendsFrames = cam.lastReceivedImageTime + processors.processingDelay > processors.cameraDataProvider->getHostTimestamp();
                bool sendsStatus = cam.lastReceivedStatusTime + 1.0f > processors.cameraDataProvider->getHostTimestamp();

                // Red color status:
                ImU32 col = IM_COL32(255,0,0,255);
                if(sendsFrames && sendsStatus){
                    col = IM_COL32(0,255,0,255);
                } else if(sendsStatus){
                    col = IM_COL32(255,255,0,255);
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::BulletText("");
                ImGui::PopStyleColor();

                if(cam.isValid){
                    // Flash Button:
                    if(ImGui::Button("Flash")){
                        std::shared_ptr<RemoteCamTask> task = std::make_shared<RemoteCamTask>();
                        task->name = "Flash " + cam.address;
                        task->thread = std::thread([&cam, task](){
                            cam.install(task->log);
                            task->isFinished = true;
                        });
                        tasks.push_back(task);
                    }
                    ImGui::SameLine();

                    // Log Button:
                    if(ImGui::Button("Show Log")){
                        std::shared_ptr<RemoteCamTask> task = std::make_shared<RemoteCamTask>();
                        task->name = "Log " + cam.address;
                        task->thread = std::thread([&cam, task](){
                            cam.getLog(task->log);
                            task->isFinished = true;
                        });
                        tasks.push_back(task);
                    }
                    ImGui::SameLine();

                    // Start Service:
                    if(ImGui::Button("Start")){
                        std::shared_ptr<RemoteCamTask> task = std::make_shared<RemoteCamTask>();
                        task->name = "Start " + cam.address;
                        task->showLog = false;
                        task->thread = std::thread([&cam, task](){
                            cam.startService(task->log);
                            task->isFinished = true;
                        });
                        tasks.push_back(task);
                    }
                    ImGui::SameLine();

                    // Stop Service:
                    if(ImGui::Button("Stop")){
                        std::shared_ptr<RemoteCamTask> task = std::make_shared<RemoteCamTask>();
                        task->name = "Stop " + cam.address;
                        task->showLog = false;
                        task->thread = std::thread([&cam, task](){
                            cam.stopService(task->log);
                            task->isFinished = true;
                        });
                        tasks.push_back(task);
                    }
                }

                ImGui::EndGroup();
            }
            if(CameraNetworkUtils::scanThreadFinished){
                ImGui::Separator();
                if(ImGui::Button("Refresh")){
                    CameraNetworkUtils::scanRemoteCams();
                }
            }
        }
        for (int i = 0; i < tasks.size(); ++i) {
            std::shared_ptr<RemoteCamTask>& task = tasks[i];

            bool erase = false;

            if (task->showLog) {
                bool shouldBeOpen = true;

                // Minimum and maximum window size
                ImGui::SetNextWindowSizeConstraints(
                    ImVec2(400, 250),
                    ImVec2(FLT_MAX, FLT_MAX)
                    );

                // Initial window size
                ImGui::SetNextWindowSize(
                    ImVec2(650, 400),
                    ImGuiCond_FirstUseEver
                    );

                bool visible;
                if (task->isFinished)
                    visible = ImGui::Begin(task->name.c_str(), &shouldBeOpen);
                else
                    visible = ImGui::Begin(task->name.c_str());

                if (visible) {
                    // Scrollable log area
                    ImGui::BeginChild(
                        "LogContent",
                        ImVec2(0, 0),
                        false,
                        ImGuiWindowFlags_HorizontalScrollbar
                        );

                    // Check whether user is already at the bottom
                    bool autoScroll =
                        ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

                    ImGui::PushFont(ViewData::getInstance().fontSSH);

                    std::string log = task->log.str();
                    ImGui::TextAnsiUnformatted(log.c_str(), nullptr);

                    ImGui::PopFont();

                    // Scroll down automatically
                    if (autoScroll)
                        ImGui::SetScrollHereY(1.0f);

                    ImGui::EndChild();
                }

                ImGui::End();

                erase = !shouldBeOpen;
            }
            else {
                if (task->isFinished)
                    erase = true;
            }

            if (erase) {
                tasks.erase(tasks.begin() + i);
                --i;
            }
        }
    }
};

std::vector<std::shared_ptr<RemoteCamTask>> NetworkPaneUI::tasks;

int NetworkPaneUI::selectedInterfaceId = -1;
