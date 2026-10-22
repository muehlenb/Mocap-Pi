#pragma once

#include <nlohmann/json.hpp>

#include <ceres/ceres.h>
#include <ceres/rotation.h>

#include "src/util/math/Mat4.h"

/**
 * A calibration for a pinhole camera with position, rotation, focal length and a
 * principal point.
 *
 * The data is meant to be optimized by ceres (SO DON'T SWAP ATTRIBUTES, ONLY APPEND
 * NEW ONES!).
 */
struct CameraCalibration {
    double rotation[3] = {0.0, 0.0, 0.0};
    double translation[3] = {0.0, 0.0, 0.0};
    double focal_length[2] = {0, 0};
    double principal_point[2] = {0, 0};
    double k[3] = {0,0,0};
    double p[2] = {0,0};

    // Median reprojection error of calibration:
    double calib_reprojection_error = NAN;

    // Median distance error of calibration:
    double calib_distance_error = NAN;

    // Number of observations considered for optimization:
    int calib_observation_num = 0;

    // The unique camera id which this calibration is for:
    int unique_cam_id = -1;


    CameraCalibration(){}

    CameraCalibration(int uniqueCamID)
        : unique_cam_id(uniqueCamID){}

    /**
     * Returns the view matrix (world to camera space).
     */
    Mat4f getViewMatrix(){
        Mat4f matrix;

        double rotation_matrix[9];
        ceres::AngleAxisToRotationMatrix(rotation, rotation_matrix);

        matrix.data[0] = rotation_matrix[0];
        matrix.data[1] = rotation_matrix[1];
        matrix.data[2] = rotation_matrix[2];

        matrix.data[4] = rotation_matrix[3];
        matrix.data[5] = rotation_matrix[4];
        matrix.data[6] = rotation_matrix[5];

        matrix.data[8] = rotation_matrix[6];
        matrix.data[9] = rotation_matrix[7];
        matrix.data[10] = rotation_matrix[8];

        matrix.data[12] = translation[0];
        matrix.data[13] = translation[1];
        matrix.data[14] = translation[2];
        return matrix;
    }

    /**
     * Returns the transformation matrix of the camera (Cam to World Space).
     */
    Mat4f getModelMatrix(){
        return getViewMatrix().inverse();
    }

    /**
     * Returns the direction of the given pixel coordinate (in camera space).
     */
    Vec4f getRay(const double x, const double y) {
        double xn = (x - principal_point[0]) / focal_length[0];
        double yn = (y - principal_point[1]) / focal_length[1];

        // Apply distortion correction (iteratively)
        double x_corrected = xn;
        double y_corrected = yn;
        for(int i = 0; i < 20; ++i) {
            double r2 = x_corrected*x_corrected + y_corrected*y_corrected;
            double x_delta = 2*p[0]*x_corrected*y_corrected + p[1]*(r2 + 2*x_corrected*x_corrected);
            double y_delta = p[0]*(r2 + 2*y_corrected*y_corrected) + 2*p[1]*x_corrected*y_corrected;
            double factor = 1 + k[0]*r2 + k[1]*r2*r2 + k[2]*r2*r2*r2;

            x_corrected = (xn - x_delta) / factor;
            y_corrected = (yn - y_delta) / factor;
        }

        return Vec4f(-x_corrected, -y_corrected, 1, 0).normalized();
    }

    /**
     * Projects a 3D Point (world space) to the camera image and returns pixel coordinates.
     */
    Vec4f project(Vec4f point3DWS){
        // camera[0,1,2] are the angle-axis rotation.
        double projP[3];
        double input[3] = {point3DWS.x, point3DWS.y, point3DWS.z};
        ceres::AngleAxisRotatePoint(&rotation[0], &input[0], projP);

        // camera[3,4,5] are the translation.
        projP[0] += translation[0];
        projP[1] += translation[1];
        projP[2] += translation[2];

        double xn = -projP[0] / projP[2];
        double yn = -projP[1] / projP[2];

        double r2 = xn*xn + yn*yn;
        double x_distorted = xn * (1 + k[0]*r2 + k[1]*r2*r2 + k[2]*r2*r2*r2)
                             + 2*p[0]*xn*yn + p[1]*(r2 + 2*xn*xn);
        double y_distorted = yn * (1 + k[0]*r2 + k[1]*r2*r2 + k[2]*r2*r2*r2)
                             + p[0]*(r2 + 2*yn*yn) + 2*p[1]*xn*yn;

        // Applying distortion:
        double u = x_distorted * focal_length[0] + principal_point[0];
        double v = y_distorted * focal_length[1] + principal_point[1];

        return Vec4f(float(u), float(v), 0, 0);
    }
};

inline void to_json(nlohmann::json& j, const CameraCalibration& cam) {
    j = nlohmann::json{{"rotation", {cam.rotation[0], cam.rotation[1], cam.rotation[2]}},
                       {"translation", {cam.translation[0], cam.translation[1], cam.translation[2]}},
                       {"focal_length", {cam.focal_length[0], cam.focal_length[1]}},
                       {"principal_point", {cam.principal_point[0], cam.principal_point[1]}},
                       {"calib_reprojection_error", cam.calib_reprojection_error},
                       {"calib_distance_error", cam.calib_distance_error},
                       {"calib_observation_num", cam.calib_observation_num},
                       {"unique_cam_id", cam.unique_cam_id}};
}

inline void from_json(const nlohmann::json& j, CameraCalibration& cam) {
    try {
        j.at("rotation").get_to<std::array<double, 3>>(*reinterpret_cast<std::array<double, 3>*>(cam.rotation));
        j.at("translation").get_to<std::array<double, 3>>(*reinterpret_cast<std::array<double, 3>*>(cam.translation));
        j.at("focal_length").get_to<std::array<double, 2>>(*reinterpret_cast<std::array<double, 2>*>(cam.focal_length));
        j.at("principal_point").get_to<std::array<double, 2>>(*reinterpret_cast<std::array<double, 2>*>(cam.principal_point));
        j.at("calib_reprojection_error").get_to(cam.calib_reprojection_error);
        j.at("calib_distance_error").get_to(cam.calib_distance_error);
        j.at("calib_observation_num").get_to(cam.calib_observation_num);
        j.at("unique_cam_id").get_to(cam.unique_cam_id);
    } catch(std::exception e){
        std::cerr << e.what() << std::endl;
    }
}
