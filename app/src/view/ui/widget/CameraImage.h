#pragma once

#include <memory>
#include <string>

#include <imgui_internal.h>
#include <imgui.h>

#include "src/model/MarkerPaket.h"

class CameraImageUI {
public:
    static void renderCameraUI(int x, int y, int width, int height, std::shared_ptr<MarkerPaket> paket){
		if(paket == nullptr)
			return;

		int camWidth = 1280;
		int camHeight = 720;

		float xScale = width / float(camWidth);
		float yScale = height / float(camHeight);

		ImGui::SetNextWindowPos(ImVec2(float(x),float(y)));
		ImGui::SetNextWindowSize(ImVec2(float(width),float(height)));
		ImGui::Begin((std::string("Camera ") + std::to_string(paket->camId)).c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		for(Marker2D marker : paket->markers){
			float markerX = marker.x * xScale;
			float markerY = marker.y * yScale;

			draw_list->AddCircleFilled(ImVec2(x + markerX, y + markerY), fmaxf(1.f, sqrt(sqrt(marker.pixels))/1.5f), ImGui::GetColorU32(ImGuiCol_Text));
		}

		draw_list->AddText(ImVec2(x + 6, y + height - 20), ImGui::GetColorU32(ImGuiCol_Text), ("Markers: " + std::to_string(paket->markers.size())).c_str());


		ImGui::End();
	}
};
