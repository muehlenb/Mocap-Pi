#pragma once

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <imgui_internal.h>
#include <imgui.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "src/view/ViewData.h"
#include "src/model/DataModel.h"
#include "src/processors/ProcessorCollection.h"
#include "src/util/settings/Settings.h"

class CameraImageUI {
    struct CameraTexture {
        GLuint id = 0;
        int width = 0;
        int height = 0;
    };

    inline static std::map<int, CameraTexture> textures;
    inline static std::map<int, int> lastRenderedFrame;

    static std::string toString(double d) {
        std::ostringstream oss;
        oss << std::setprecision(2) << d;
        return oss.str();
    }

    static GLuint updateTexture(int camID, const std::vector<uint8_t>& image_data, int width, int height) {
        if (width <= 0 || height <= 0 || image_data.size() < static_cast<size_t>(width) * static_cast<size_t>(height))
            return 0;

        CameraTexture& texture = textures[camID];

        if (texture.id == 0) {
            glGenTextures(1, &texture.id);

            if (texture.id == 0) {
                std::fprintf(stderr, "[CameraImageUI] glGenTextures failed for camera %d\n", camID);
                return 0;
            }

            glBindTexture(GL_TEXTURE_2D, texture.id);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        } else {
            glBindTexture(GL_TEXTURE_2D, texture.id);
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        if (texture.width != width || texture.height != height) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data.data());
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, image_data.data());
        }

        GLenum error = glGetError();

        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glBindTexture(GL_TEXTURE_2D, 0);

        if (error != GL_NO_ERROR) {
            std::fprintf(stderr, "[CameraImageUI] OpenGL error 0x%X for camera %d\n", error, camID);
            glDeleteTextures(1, &texture.id);
            texture = {};
            return 0;
        }

        texture.width = width;
        texture.height = height;

        return texture.id;
    }

public:
    static void releaseTextures() {
        for (auto& [camID, texture] : textures) {
            if (texture.id != 0)
                glDeleteTextures(1, &texture.id);
        }

        textures.clear();
        lastRenderedFrame.clear();
    }

    static void renderCameraUI(int x, int y, int width, int height, DataModel& model, ProcessorCollection& processors, CameraFrame& frame) {
        const int camID = frame.uniqueCamID;
        const int currentFrame = ImGui::GetFrameCount();

        auto lastIt = lastRenderedFrame.find(camID);

        if (lastIt != lastRenderedFrame.end() && lastIt->second == currentFrame) {
            static int lastReportedFrame = -1;

            if (lastReportedFrame != currentFrame) {
                std::fprintf(stderr, "[CameraImageUI] Duplicate rendering for camera %d in frame %d\n", camID, currentFrame);
                lastReportedFrame = currentFrame;
            }

            return;
        }

        auto camIt = model.cameras.find(camID);
        if (camIt == model.cameras.end())
            return;

        lastRenderedFrame[camID] = currentFrame;

        constexpr float camWidth = 1280.0f;
        constexpr float camHeight = 720.0f;
        constexpr int rawWidth = 320;
        constexpr int rawHeight = 180;
        constexpr size_t expectedImageSize = static_cast<size_t>(rawWidth) * rawHeight;

        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(std::max(1, width)), static_cast<float>(std::max(1, height))));

        const ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        const std::string windowName = "Camera " + std::to_string(camID);

        if (!ImGui::Begin(windowName.c_str(), nullptr, flags)) {
            ImGui::End();
            return;
        }

        auto& camera = camIt->second;
        CameraDeviceState& camInfo = camera.deviceState;
        ImGuiStyle& style = ImGui::GetStyle();

        const bool showControls = Settings::instance.showCameraControls;
        const ImVec2 available = ImGui::GetContentRegionAvail();

        const float controlsHeight = showControls ? ImGui::GetFrameHeight() + style.ItemSpacing.y : 0.0f;
        const float imageWidth = ImMax(1.0f, available.x);
        const float imageHeight = ImMax(1.0f, available.y - controlsHeight - style.ItemSpacing.y);

        const ImVec2 imageSize(imageWidth, imageHeight);
        const ImVec2 imagePos = ImGui::GetCursorScreenPos();

        const std::vector<uint8_t>& rawImage = camera.rawImage;

        if (camInfo.requestRawImage && rawImage.size() >= expectedImageSize) {
            const GLuint textureID = updateTexture(camID, rawImage, rawWidth, rawHeight);

            if (textureID != 0)
                ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(textureID)), imageSize);
            else
                ImGui::Dummy(imageSize);
        } else {
            ImGui::Dummy(imageSize);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        if (showControls) {
            const float spacing = style.ItemSpacing.x;
            const float checkboxWidth = ImGui::GetFrameHeight();
            const float sliderWidth = ImMax(1.0f, (available.x - checkboxWidth - 3.0f * spacing) / 3.0f);

            ImGui::PushItemWidth(sliderWidth);

            ImGui::SliderInt("##Threshold", &camInfo.requestedBrightnessThreshold, 0, 255, "Trh: %d");
            ImGui::SameLine();

            ImGui::SliderInt("##Exposure", &camInfo.requestedExposureTime, 100, 10000, "Exp: %d");
            ImGui::SameLine();

            ImGui::SliderFloat("##Lens", &camInfo.requestLensPosition, 0.0f, 1.0f, "Lens: %.2f");

            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::Checkbox("##R", &camInfo.requestRawImage);
        }

        if (Settings::instance.showCameraInfos) {
            std::string str = "ID: " + std::to_string(camInfo.uniqueCamID)
            + " | Marker: " + std::to_string(frame.markers.size())
                + " | Trh: " + std::to_string(camInfo.brightnessThreshold)
                + " | Exp: " + std::to_string(camInfo.exposureTime)
                + " | Err: " + toString(camInfo.timeSyncError * 1000) + " ms";

            draw_list->AddText(ImVec2(imagePos.x + 10, imagePos.y + 5), ImGui::GetColorU32(ImGuiCol_Text), str.c_str());

            std::string str2 = "FPS: " + toString(camInfo.measuredFramerate)
                               + " | Drops/s: " + toString(camInfo.frameDropsLastSecond)
                               + " | Delay: " + toString(camInfo.captureToAvailableDelay * 1000) + " ms"
                               + " | Ping: " + toString(camInfo.ping) + " ms";

            draw_list->AddText(ImVec2(imagePos.x + 10, imagePos.y + 23), ImGui::GetColorU32(ImGuiCol_Text), str2.c_str());
        }

        const float xScale = imageWidth / camWidth;
        const float yScale = imageHeight / camHeight;

        if (frame.valid) {
            ImFont* smallFont = ViewData::getInstance().fontSmall;
            bool fontPushed = false;

            if (smallFont != nullptr) {
                ImGui::PushFont(smallFont);
                fontPushed = true;
            }

            for (const ImageMarker& marker : frame.markers) {
                if (!std::isfinite(marker.x) || !std::isfinite(marker.y))
                    continue;

                const float markerX = imagePos.x + marker.x * xScale;
                const float markerY = imagePos.y + marker.y * yScale;

                std::string markerLabel;

                if (marker.marker3DIdx >= 0 && static_cast<size_t>(marker.marker3DIdx) < model.lastTick.markers.size())
                    markerLabel = std::to_string(model.lastTick.markers[marker.marker3DIdx].id % 100);

                const float pixels = std::isfinite(marker.pixels) ? std::max(0.0f, static_cast<float>(marker.pixels)) : 0.0f;
                const float radius = std::max(1.0f, std::sqrt(std::sqrt(pixels)) / 1.5f);

                draw_list->AddCircleFilled(ImVec2(markerX, markerY), radius, ImGui::GetColorU32(ImGuiCol_Text));

                if (Settings::instance.drawImageMarkerId)
                    draw_list->AddText(ImVec2(markerX, markerY), ImGui::GetColorU32(ImGuiCol_Text), markerLabel.c_str());

                if (Settings::instance.drawImageFlow && std::isfinite(marker.flowX) && std::isfinite(marker.flowY))
                    draw_list->AddLine(ImVec2(markerX, markerY), ImVec2(markerX + marker.flowX * 3, markerY + marker.flowY * 3), ImGui::GetColorU32(ImGuiCol_Separator), 1.0f);
            }

            if (fontPushed)
                ImGui::PopFont();
        }

        if (processors.cameraCalibrator && processors.cameraCalibrator->getIsCollectingObservations()) {
            auto& observations = processors.cameraCalibrator->getObservations();
            auto obsIt = observations.find(camID);

            if (obsIt != observations.end()) {
                for (const auto& observation : obsIt->second) {
                    const ImageMarker& marker = observation.second;

                    if (!std::isfinite(marker.x) || !std::isfinite(marker.y))
                        continue;

                    const float markerX = imagePos.x + marker.x * xScale;
                    const float markerY = imagePos.y + marker.y * yScale;
                    const float pixels = std::isfinite(marker.pixels) ? std::max(0.0f, static_cast<float>(marker.pixels)) : 0.0f;
                    const float radius = std::max(1.0f, std::sqrt(std::sqrt(pixels)) / 2.0f);

                    draw_list->AddCircleFilled(ImVec2(markerX, markerY), radius, ImGui::GetColorU32(ImGuiCol_Separator));
                }
            }
        }

        ImGui::End();
    }
};
