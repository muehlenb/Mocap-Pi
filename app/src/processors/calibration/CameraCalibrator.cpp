#include "src/processors/calibration/CameraCalibrator.h"

#include <pcl/point_types.h>
#include <pcl/registration/transformation_estimation.h>
#include <pcl/registration/transformation_estimation_svd.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>

#include <imgui_internal.h>
#include <imgui.h>

#include <ceres/ceres.h>
#include <ceres/rotation.h>

#include <nlohmann/json.hpp>
#include <fstream>

#include "CeresCostFunctions.h"

int CalibrationSticks::selectedCalibStick = 0;
unsigned int CalibrationSticks::calibrationStickNum = 2;
const char* CalibrationSticks::calibrationStickNames[] = {"30cm Simulation-Stick", "25cm Optitrack-Stick"};
const std::tuple<float, float> CalibrationSticks::calibrationStickLengths[] = {
    {0.1f, 0.2f},
    {0.089f, 0.162f},
};

void CameraCalibrator::calculateStatistics(){
    for(int camId = 0; camId < observations.size(); ++camId){
        CameraCalibration& calibration = calibrations[camId];

        std::vector<std::pair<int, ImageMarker>>& observationPerCam = observations[calibration.unique_cam_id];

        std::vector<float> camReprojectionErrors;
        std::vector<float> camDistanceErrors;
        int num = 0;

        for(std::pair<int, ImageMarker>& observation : observationPerCam){
            int point3DId = observation.first;
            Vec4f predicted = calibration.project(points3D[point3DId].toVec4f());

            camReprojectionErrors.push_back(predicted.distanceTo(observation.second.getCoordinates()));

            float distance3D = 0;
            if(point3DId % 3 == 0){
                distance3D = points3D[point3DId].distanceTo(points3D[point3DId+2]);
            } else if(point3DId % 3 == 1){
                distance3D = points3D[point3DId].distanceTo(points3D[point3DId-1]) + points3D[point3DId].distanceTo(points3D[point3DId+1]);
            } else {
                distance3D = points3D[point3DId].distanceTo(points3D[point3DId-2]);
            }

            camDistanceErrors.push_back(abs(CalibrationSticks::getSelectedLength() - distance3D) / CalibrationSticks::getSelectedLength());

            ++num;
        }

        std::sort(camReprojectionErrors.begin(), camReprojectionErrors.end(), [](float x, float y){
            return x > y;
        });
        std::sort(camDistanceErrors.begin(), camDistanceErrors.end(), [](float x, float y){
            return x > y;
        });

        calibration.calib_observation_num = num;
        if(camReprojectionErrors.size() > 0)
            calibration.calib_reprojection_error = camReprojectionErrors[camReprojectionErrors.size()/2];

        if(camDistanceErrors.size() > 0)
            calibration.calib_distance_error = camDistanceErrors[camDistanceErrors.size()/2];
    }
}

void CameraCalibrator::saveCalibration(){
    nlohmann::json json_to_save = calibrations;
    std::ofstream output_file("calibration.json");
    output_file << json_to_save.dump(4);
    output_file.close();
}

void CameraCalibrator::loadCalibration(){
    try {
        std::ifstream input_file("calibration.json");
        nlohmann::json json_to_load;
        input_file >> json_to_load;
        std::vector<CameraCalibration> calibs = json_to_load.get<std::vector<CameraCalibration>>();

        for(CameraCalibration& calibration : calibs){
            if(model.cameras.count(calibration.unique_cam_id) == 0){
                model.semaphore.acquire();
                if(model.cameras.count(calibration.unique_cam_id) == 0)
                    model.cameras.try_emplace(calibration.unique_cam_id);
                model.semaphore.release();
            }

            model.cameras[calibration.unique_cam_id].calibration = calibration;
        }

        input_file.close();
    } catch(nlohmann::json::parse_error& e){
        std::cout << e.what() << std::endl;
    } catch (std::runtime_error& e) {
        std::cout << e.what() << std::endl;
    }
}

void CameraCalibrator::addObservation(std::vector<CameraFrame>& frames, std::vector<Vec4f> gtPositions){
    if(isOptimizing)
        return;

    // Check if all have the same timestamp:
    if(frames.size() > 0 && frames[0].valid){
        float timestamp = frames[0].timestamp;

        for(CameraFrame& frame : frames){
            if(frame.valid && abs(timestamp - frame.timestamp) > 0.001){
                std::cout << "Timestamp mismatch! Reject sample." << std::endl;
                return;
            }
        }
    }

    int numPkgsWithExactly3Markers = 0;

    // Count pakets with exactly 3 markers:
    for(CameraFrame& frame : frames){
        if(frame.valid && frame.markers.size() == 3){
            ++numPkgsWithExactly3Markers;
        }
    }

    // If there are not at least 2 pkgs with exactly 3 markers, we cannot calibrate:
    if(numPkgsWithExactly3Markers < 2)
        return;

    bool hasObservation = false;
    for(unsigned int i = 0; i < frames.size(); ++i){
        CameraFrame& frame = frames[i];

        int calibIndex = -1;
        for(int h = 0; h < calibrations.size(); ++h){
            if(calibrations[h].unique_cam_id == frame.uniqueCamID){
                calibIndex = h;
                break;
            }
        }

        if(calibIndex == -1){
            std::cout << "Add Calibration with UniqueID: " << frame.uniqueCamID << std::endl;
            calibrations.push_back(CameraCalibration(frame.uniqueCamID));
        }

        if(observations.count(frame.uniqueCamID) == 0)
            observations[frame.uniqueCamID] = std::vector<std::pair<int, ImageMarker>>();

        if(frame.valid && frame.markers.size() == 3){
            Vec4f v1 = (frame.markers[0]).getCoordinates();
            Vec4f v2 = (frame.markers[1]).getCoordinates();
            Vec4f v3 = (frame.markers[2]).getCoordinates();

            // Distance between points:
            float d12 = v1.distanceTo(v2);
            float d13 = v1.distanceTo(v3);
            float d23 = v2.distanceTo(v3);

            // Length of relevant distances:
            float l1 = d12 + d13;
            float l2 = d12 + d23;
            float l3 = d23 + d13;

            ImageMarker rawV1 = frame.markers[0];
            ImageMarker rawV2 = frame.markers[1];
            ImageMarker rawV3 = frame.markers[2];

            ImageMarker orderedV1 = l1 < l2 != l1 <= l3 ? rawV1 : (l2 < l3 != l2 <= l3 ? rawV2 : rawV3);   // V1: Middle distance to others.
            ImageMarker orderedV2 = l1 < l2 && l1 < l3  ? rawV1 : (l2 < l3 && l2 < l1 ? rawV2 : rawV3);    // V2: Smallest distance to others.
            ImageMarker orderedV3 = l1 > l2 && l1 > l3  ? rawV1 : (l2 > l3 && l2 > l1 ? rawV2 : rawV3);    // V3: Largest distance to others.

            float abs = std::abs((v1 - v2).normalized().dot((v1 - v3).normalized()));
            if(abs < 0.98){
                std::cout << "Rejected Marker: Not aligned enough! " << abs << std::endl;
                continue;
            }

            if(d12 < 5 || d13 < 5 || d23 < 5){
                std::cout << "Rejected Marker: Distance between markers not big enough! " << d12 << " " << d13 << " " << d23 << std::endl;
                continue;
            }

            if(orderedV1.pixels < 8 || orderedV2.pixels < 8 || orderedV3.pixels < 8){
                std::cout << "Rejected Marker: Too far away / to small in image! " << orderedV1.pixels << " " << orderedV2.pixels << " " << orderedV3.pixels << std::endl;
                continue;
            }

            float smallerDist = orderedV1.getCoordinates().distanceTo(orderedV2.getCoordinates());
            float largerDist = orderedV2.getCoordinates().distanceTo(orderedV3.getCoordinates());

            float distAspect = smallerDist * 2 / largerDist;

            if(distAspect < 0.75 || distAspect > 1.3){
                std::cout << "Distance Aspect doesn't fit: " << smallerDist << " * 2 != " << largerDist << std::endl;
                continue;
            } else {
                std::cout << "Distance Aspect fits!" << std::endl;
            }

            if(frame.markers[0].pixels > 1000 || frame.markers[1].pixels > 1000 || frame.markers[2].pixels > 1000){
                std::cout << "Rejected Marker: Markers to close to camera! " << frame.markers[0].pixels << " " << frame.markers[1].pixels << " " << frame.markers[2].pixels << std::endl;
                continue;
            }

            observations[frame.uniqueCamID].push_back(std::pair<int, ImageMarker>(currentPointId, orderedV1));
            observations[frame.uniqueCamID].push_back(std::pair<int, ImageMarker>(currentPointId + 1, orderedV2));
            observations[frame.uniqueCamID].push_back(std::pair<int, ImageMarker>(currentPointId + 2, orderedV3));
            hasObservation = true;
        }
    }

    if(hasObservation){
        if(gtPositions.size() == 3){
            groundTruthObservations.push_back(std::pair<int, Vec4f>(currentPointId, gtPositions[0]));
            groundTruthObservations.push_back(std::pair<int, Vec4f>(currentPointId + 1, gtPositions[1]));
            groundTruthObservations.push_back(std::pair<int, Vec4f>(currentPointId + 2, gtPositions[2]));
        }

        Vec4d randV1 = Vec4d(double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, 1.0);
        Vec4d randV2 = Vec4d(double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, 1.0);
        Vec4d randV3 = Vec4d(double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, double(rand()) / (RAND_MAX) * 2 - 1, 1.0);

        points3D.push_back(randV1);
        points3D.push_back(randV2);
        points3D.push_back(randV3);

        currentPointId += 3;

        createInitialGuess();
        align3DPointsToGuess();
    }
}

/**
 * Guesses a extremly rough 3D point with a z value based on the number of pixels
 * of the given marker.
 */
Vec4f CameraCalibrator::guess3DPoint(ImageMarker raw2D){
    // Sampled number of pixels for different distances and fitted a exponential
    // function into the samples, which was 9.575 * pixels ^(-0.5):
    float z = std::max(0.4, std::min(9.575 * std::pow(raw2D.pixels, -0.5), 3.0));

    // Umrechnung der FOV-Werte von Grad zu Radianten
    float hFOVRad = 62 * (M_PI / 180.0);
    float vFOVRad = 48 * (M_PI / 180.0);

    // Berechnung der Halbwinkel
    float halberHFOV = hFOVRad / 2.0f;
    float halberVFOV = vFOVRad / 2.0f;

    // Berechnung der Größe der Welt-Einheiten pro Pixel
    float weltProPixelX = 2.0f * tan(halberHFOV) / 1280.f;
    float weltProPixelY = 2.0f * tan(halberVFOV) / 720.f;

    // Umrechnung der Pixelkoordinaten in 3D-Weltkoordinaten
    float x = ((raw2D.x) - 1280.f / 2.0f) * weltProPixelX * z;
    float y = (720.f / 2.0f - (raw2D.y)) * weltProPixelY * z;

    return Vec4f(x, y, z);
}

void CameraCalibrator::createInitialGuess(){
    int pivotCamId = 0;
    int pivotCamObservationNum = -1;

    // Choose pivot cam with largest amount of observations:
    for(int camId = 0; camId < calibrations.size(); ++camId){
        int uniqueCamID = calibrations[camId].unique_cam_id;
        if(observations.size() > camId && observations[uniqueCamID].size() > pivotCamObservationNum){
            pivotCamId = camId;
            pivotCamObservationNum = observations[uniqueCamID].size();
        }
    }
    calibrations[pivotCamId].translation[0] = 0;
    calibrations[pivotCamId].translation[1] = 0;
    calibrations[pivotCamId].translation[2] = 0;
    calibrations[pivotCamId].rotation[0] = 0;
    calibrations[pivotCamId].rotation[1] = 0;
    calibrations[pivotCamId].rotation[2] = 0;

    int pivotUniqueID = calibrations[pivotCamId].unique_cam_id;

    //std::cout << "Initial Guess: " << std::endl;
    for(int camId = 0; camId < calibrations.size(); ++camId){
        if(camId == pivotCamId){
            continue;
        }

        int uniqueID = calibrations[camId].unique_cam_id;

        std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> sourcePC(new pcl::PointCloud<pcl::PointXYZ>());
        std::shared_ptr<pcl::PointCloud<pcl::PointXYZ>> targetPC(new pcl::PointCloud<pcl::PointXYZ>());

        int sourceId = 0;
        int targetId = 0;

        // Filter Observations:
        while(sourceId < observations[pivotUniqueID].size() && targetId < observations[uniqueID].size()){
            std::pair<int, ImageMarker> sourceObs = observations[pivotUniqueID][sourceId];
            std::pair<int, ImageMarker> targetObs = observations[uniqueID][targetId];

            if(sourceObs.first == targetObs.first){
                Vec4f sourceGuess3D = guess3DPoint(sourceObs.second);
                Vec4f targetGuess3D = guess3DPoint(targetObs.second);

                sourcePC->push_back(pcl::PointXYZ(sourceGuess3D.x, sourceGuess3D.y, sourceGuess3D.z));
                targetPC->push_back(pcl::PointXYZ(targetGuess3D.x, targetGuess3D.y, targetGuess3D.z));

                targetId++;
                sourceId++;
            } else if(sourceObs.first > targetObs.first){
                targetId++;
            } else {
                sourceId++;
            }
        }

        if(sourcePC->size() > 3){
            pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ> estimator;
            pcl::registration::TransformationEstimationSVD<pcl::PointXYZ, pcl::PointXYZ>::Matrix4 mat;
            estimator.estimateRigidTransformation(*sourcePC, *targetPC, mat);

            Mat4f myMat = Mat4f::fromEigen(mat).inverse();

            float* mData = myMat.data;

            double rotation_matrix[9] = {
                mData[0], mData[1], mData[2],
                mData[4], mData[5], mData[6],
                mData[8], mData[9], mData[10]
            };

            ceres::RotationMatrixToAngleAxis(rotation_matrix, calibrations[camId].rotation);
            calibrations[camId].translation[0] = mData[12];
            calibrations[camId].translation[1] = mData[13];
            calibrations[camId].translation[2] = mData[14];
        }
    }
}

void CameraCalibrator::groundTruthRegistration(){
    /*
    SVD Result (to align it correct with UE5 values)
    0.6868 0.236  0.6875 -1.806
    -0.01510.9503 -0.31111.906
    -0.72670.2033 0.6562 -1.494
    0      0      0      1
    */
    Mat4d mat = Mat4d({
        0.688734, -0.01585, -0.724841, 0.0f,
        0.233701, 0.951251, 0.201258, 0.0f,
        0.686316, -0.308009, 0.658863, 0.0f,
        -1.8, 1.9, -1.5, 1.0f
    });

    Mat4d estimated = calibrations[0].getViewMatrix().toMat4d();
    Mat4d transform = mat * estimated;

    for(int camId = 0; camId < calibrations.size(); ++camId){
        Mat4d camMat = (transform * calibrations[camId].getViewMatrix().toMat4d().inverse()).inverse();

        double rotation_matrix[9] = {
            camMat.data[0], camMat.data[1], camMat.data[2],
            camMat.data[4], camMat.data[5], camMat.data[6],
            camMat.data[8], camMat.data[9], camMat.data[10]
        };

        ceres::RotationMatrixToAngleAxis(rotation_matrix, calibrations[camId].rotation);

        calibrations[camId].translation[0] = camMat.data[12];
        calibrations[camId].translation[1] = camMat.data[13];
        calibrations[camId].translation[2] = camMat.data[14];
    }
}

void CameraCalibrator::align3DPointsToGuess(){
    std::vector<Vec4f> sumPos(points3D.size());
    std::vector<int> nums(points3D.size());

    for(int i=0; i < points3D.size(); ++i){
        sumPos[i] = Vec4f(0,0,0,0);
        nums[i] = 0;
    }

    for(int camId = 0; camId < calibrations.size(); ++camId){
        int uniqueCamID = calibrations[camId].unique_cam_id;
        for(int obsId = 0; obsId < observations[uniqueCamID].size();++obsId){
            std::pair<int, ImageMarker>& observation = observations[uniqueCamID][obsId];
            int pointId = observation.first;

            sumPos[pointId] = sumPos[pointId] + calibrations[camId].getModelMatrix() * guess3DPoint(observation.second);
        }
    }

    for(int i=0; i < points3D.size(); ++i){
        Vec4f p = sumPos[i] / sumPos[i].w;
        points3D[i] = Vec4d(p.x, p.y, p.z, p.w);
    }
}

void CameraCalibrator::resetCalibrations(){
    for(CameraCalibration& calibration : calibrations){
        calibration.translation[0] = calibration.translation[1] = calibration.translation[2] = 0;
        calibration.rotation[0] = calibration.rotation[1] = calibration.rotation[2] = 0;

        calibration.focal_length[0] = 1400;
        calibration.focal_length[1] = 1400;

        calibration.principal_point[0] = 640;
        calibration.principal_point[1] = 360;

        calibration.k[0] = calibration.k[1] = calibration.k[2] = 0;
        calibration.p[0] = calibration.p[1] = 0;
    }
}

void CameraCalibrator::filterMeasurement(){

}


void CameraCalibrator::refineExtrinsicGuess(){
    ceres::Problem problem;

    for (unsigned int i = 0; i < calibrations.size(); ++i) {
        bool hasObservations = false;
        int uniqueCamID = calibrations[i].unique_cam_id;
        for (unsigned int j = 0; j < observations[uniqueCamID].size(); ++j) {
            const auto& observation = observations[uniqueCamID][j];

            ceres::CostFunction* cost_function = ReprojectionErrorExtOnly::Create(observation.second.x,observation.second.y);

            problem.AddResidualBlock(cost_function, new ceres::HuberLoss(0.10), &calibrations[i].rotation[0], &points3D[observation.first].x);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 0, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 0, 5);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 1, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 1, 5);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 2, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 2, 5);
            hasObservations = true;
        }

        if(hasObservations){
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 3, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 3, 5);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 4, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 4, 5);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 5, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 5, 5);
        }
    }

    for (size_t i = 0; i < points3D.size(); i += 3 * 5) {
        problem.AddResidualBlock(DistanceCostFunctor::Create(CalibrationSticks::getSelectedLength(1)), new ceres::HuberLoss(0.1), &points3D[i].x, &points3D[i+1].x);
        problem.AddResidualBlock(DistanceCostFunctor::Create(CalibrationSticks::getSelectedLength(2)), new ceres::HuberLoss(0.1), &points3D[i+1].x, &points3D[i+2].x);
    }

    // Konfigurieren und starten Sie den Solver.
    ceres::Solver::Options options;
    options.linear_solver_type = ceres::SPARSE_SCHUR;
    options.minimizer_progress_to_stdout = true;
    options.max_num_iterations = 100;
    options.update_state_every_iteration = true;
    options.sparse_linear_algebra_library_type = ceres::SUITE_SPARSE;
    options.function_tolerance = 1e-8;
    options.gradient_tolerance = 1e-10;
    options.parameter_tolerance = 1e-8;

    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);
}

void CameraCalibrator::refineExtrinsicIntrinsics(){
    //std::cout << "Start calibration... " << "\n";
    // Erstellen Sie das Problem.
    ceres::Problem problem;

    // Fügen Sie für jede Beobachtung in jeder Kamera die Kostenfunktion zum Problem hinzu.
    for (unsigned int i = 0; i < calibrations.size(); ++i) {
        bool hasObservations = false;
        int uniqueCamID = calibrations[i].unique_cam_id;

        for (unsigned int j = 0; j < observations[uniqueCamID].size(); ++j) {
            const auto& observation = observations[uniqueCamID][j];

            ceres::CostFunction* cost_function = ReprojectionError::Create(observation.second.x,observation.second.y);

            problem.AddResidualBlock(cost_function, new ceres::HuberLoss(0.1), &calibrations[i].rotation[0], &points3D[observation.first].x);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 0, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 0, 5);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 1, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 1, 5);
            problem.SetParameterLowerBound(&points3D[observation.first].x, 2, -5);
            problem.SetParameterUpperBound(&points3D[observation.first].x, 2, 5);
            hasObservations = true;
        }

        if(hasObservations){
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 3, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 3, 5);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 4, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 4, 5);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 5, -5);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 5, 5);

            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 6, 1000);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 6, 1600);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 7, 1000);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 7, 1600);

            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 8, 580);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 8, 700);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 9, 300);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 9, 420);

            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 10, -1);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 10, 1);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 11, -1);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 11, 1);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 12, -1);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 12, 1);

            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 13, -0.2);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 13, 0.2);
            problem.SetParameterLowerBound(&calibrations[i].rotation[0], 14, -0.2);
            problem.SetParameterUpperBound(&calibrations[i].rotation[0], 14, 0.2);

        }
    }

    for (size_t i = 0; i < points3D.size(); i += 3 * 5) {
        problem.AddResidualBlock(DistanceCostFunctor::Create(CalibrationSticks::getSelectedLength(1)), new ceres::HuberLoss(0.1), &points3D[i].x, &points3D[i+1].x);
        problem.AddResidualBlock(DistanceCostFunctor::Create(CalibrationSticks::getSelectedLength(2)), new ceres::HuberLoss(0.1), &points3D[i+1].x, &points3D[i+2].x);
    }

    // Konfigurieren und starten Sie den Solver.
    ceres::Solver::Options options;
    options.linear_solver_type = ceres::SPARSE_SCHUR;
    options.trust_region_strategy_type = ceres::LEVENBERG_MARQUARDT;
    options.sparse_linear_algebra_library_type = ceres::SUITE_SPARSE;
    options.minimizer_progress_to_stdout = true;
    options.max_num_iterations = 500;
    options.update_state_every_iteration = true;
    options.function_tolerance = 1e-8;
    options.gradient_tolerance = 1e-10;
    options.parameter_tolerance = 1e-8;
    options.initial_trust_region_radius = 1e4;
    options.max_trust_region_radius = 1e6;

    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);

    std::cout << summary.FullReport() << "\n";
}

void CameraCalibrator::calibrate(){
    if(observations.size() < 1)
        return;

    resetCalibrations();
    createInitialGuess();
    refineExtrinsicGuess();
    refineExtrinsicIntrinsics();

    for(int i=0; i < points3D.size() / 3; ++i){
        Vec4d p1 = points3D[i*3];
        Vec4d p2 = points3D[i*3+1];
        Vec4d p3 = points3D[i*3+2];

        std::cout << "D1: " << p1.distanceTo(p2) << " | D2: " << p2.distanceTo(p3) << std::endl;
    }

    for(CameraCalibration& calibration : calibrations){
        std::cout << "Camera: " << std::endl;
        std::cout << " \t Focal Length:" << calibration.focal_length[0] << " x " << calibration.focal_length[1] << std::endl;
        std::cout << " \t Principal Point:" << calibration.principal_point[0] << " x " << calibration.principal_point[1] << std::endl;
        std::cout << " \t Rotation:" << calibration.rotation[0] << " x " << calibration.rotation[1] << " x " << calibration.rotation[2] << std::endl;
        std::cout << " \t Translation:" << calibration.translation[0] << " x " << calibration.translation[1] << " x " << calibration.translation[2] << std::endl;
        std::cout << " \t K:" << calibration.k[0] << " x " << calibration.k[1] << " x " << calibration.k[2] << std::endl;
        std::cout << " \t P:" << calibration.p[0] << " x " << calibration.p[1] << std::endl;
    }

    for(int i=0; i < 3; ++i){
        for(int j=i; j < 3; ++j){
            if(i==j)
                continue;
            Vec4f a = calibrations[i].getModelMatrix().getPosition();
            Vec4f b = calibrations[j].getModelMatrix().getPosition();
            std::cout << "Camera " << i << " - " << j << ": " << a.distanceTo(b) << std::endl;
        }
    }

    double sumDist = 0;
    int sumCount = 0;
    for(int i=0; i < points3D.size(); i+=3){
        Vec4d p1 = points3D[i];
        Vec4d p3 = points3D[i+2];

        sumDist += p1.distanceTo(p3);
        ++sumCount;
    }

    std::cout << "Dist: " << (sumDist / sumCount) << std::endl;

    calculateStatistics();
    groundTruthRegistration();
    saveCalibration();
}
