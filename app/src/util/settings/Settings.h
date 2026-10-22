
#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <imgui_internal.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include "src/processors/calibration/CalibrationSticks.h"
#include "src/model/primitive/RigidBody.h"

struct Settings {
    bool showMarkerRays = false;
    bool show3DMarkers = true;
    bool show3DIntersections = true;
    bool showGroundTruthMarkers = true;
    bool showRigidBodies = true;

    bool showCameraInfos = false;
    bool showCameraControls = false;
    bool drawImageFlow = false;
    bool drawImageMarkerId = false;

    int leftTabOpenIdx = 0;
    int rightTabOpenIdx = 0;

    std::string networkInterface;

    std::vector<RigidBody> rigidBodies;

    static Settings instance;

    static void saveSettings();
    static bool loadSettings();
};


inline void to_json(nlohmann::json& j, const RigidBodyMarker& marker) {
    j = nlohmann::json{
        {"id", marker.id},
        {"position", {
            marker.position.x,
            marker.position.y,
            marker.position.z
        }}
    };
}

inline void from_json(const nlohmann::json& j, RigidBodyMarker& marker) {
    marker.id = j.value("id", -1);

    const auto& pos = j.at("position");
    marker.position.x = pos.at(0).get<float>();
    marker.position.y = pos.at(1).get<float>();
    marker.position.z = pos.at(2).get<float>();
    marker.position.w = 1.0f;
}


inline void to_json(nlohmann::json& j, const RigidBody& body) {
    j = nlohmann::json{
        {"label", body.label},
        {"isTracked", body.isTracked},
        {"markers", body.markers}
    };
}

inline void from_json(const nlohmann::json& j, RigidBody& body) {
    body.label = j.value("label", std::string("RigidBody"));
    body.isTracked = j.value("isTracked", false);

    body.markers.clear();

    for (const auto& markerJson : j.at("markers")) {
        const auto& pos = markerJson.at("position");

        Vec4f position;
        position.x = pos.at(0).get<float>();
        position.y = pos.at(1).get<float>();
        position.z = pos.at(2).get<float>();
        position.w = 1.0f;

        RigidBodyMarker marker(position);
        marker.id = markerJson.value("id", -1);

        body.markers.push_back(marker);
    }

    if (!body.markers.empty()) {
        body.init();
    }
}

inline void to_json(nlohmann::json& j, const Settings& settings) {
    j = nlohmann::json{
        {"showMarkerRays", settings.showMarkerRays},
        {"show3DMarkers", settings.show3DMarkers},
        {"show3DIntersections", settings.show3DIntersections},
        {"showGroundTruthMarkers", settings.showGroundTruthMarkers},
        {"showRigidBodies", settings.showRigidBodies},

        {"showCameraInfos", settings.showCameraInfos},
        {"showCameraControls", settings.showCameraControls},
        {"drawImageFlow", settings.drawImageFlow},
        {"drawImageMarkerId", settings.drawImageMarkerId},

        {"calibrationStickId", CalibrationSticks::selectedCalibStick},

        {"leftTabOpenIdx", settings.leftTabOpenIdx},
        {"rightTabOpenIdx", settings.rightTabOpenIdx},
        {"networkInterface", settings.networkInterface},

        {"rigidBodies", settings.rigidBodies}
    };
}

inline void from_json(const nlohmann::json& j, Settings& settings) {
    try {
        settings.showMarkerRays = j.value("showMarkerRays", settings.showMarkerRays);
        settings.show3DMarkers = j.value("show3DMarkers", settings.show3DMarkers);
        settings.show3DIntersections = j.value("show3DIntersections", settings.show3DIntersections);
        settings.showGroundTruthMarkers = j.value("showGroundTruthMarkers", settings.showGroundTruthMarkers);
        settings.showRigidBodies = j.value("showRigidBodies", settings.showRigidBodies);

        settings.showCameraInfos = j.value("showCameraInfos", settings.showCameraInfos);
        settings.showCameraControls = j.value("showCameraControls", settings.showCameraControls);
        settings.drawImageFlow = j.value("drawImageFlow", settings.drawImageFlow);
        settings.drawImageMarkerId = j.value("drawImageMarkerId", settings.drawImageMarkerId);

        CalibrationSticks::selectedCalibStick = j.value("calibrationStickId", CalibrationSticks::selectedCalibStick);

        settings.leftTabOpenIdx = j.value("leftTabOpenIdx", settings.leftTabOpenIdx);
        settings.rightTabOpenIdx = j.value("rightTabOpenIdx", settings.rightTabOpenIdx);
        settings.networkInterface = j.value("networkInterface", settings.networkInterface);

        if (j.contains("rigidBodies")) {
            settings.rigidBodies = j.at("rigidBodies").get<std::vector<RigidBody>>();
        }

    } catch (const std::exception& e) {
        std::cout << "Couldn't parse settings: " << e.what() << std::endl;
    }
}
