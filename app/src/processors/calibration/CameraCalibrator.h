#pragma once

#include "src/processors/calibration/CalibrationSticks.h"

#include "src/model/camera/CameraCalibration.h"
#include "src/model/DataModel.h"

#include "src/processors/Processor.h"

#include "src/util/Semaphore.h"

/**
 * Calibrates all cameras extrinsically and intrinsically via Bundle Adjustment.
 */
class CameraCalibrator : public Processor {

    // When this semaphone is released, calibration optimization is performed:
    Semaphore triggerCalculatingSemaphore;
    bool isOptimizing = false;
    std::thread calculatingThread;
    bool shouldStopThread = false;

    // uniqueCamID -> Observations of correspondences (Point3D Index, Marker2D):
    std::map<int, std::vector<std::pair<int, ImageMarker>>> observations;

    std::set<int> filteredObservations;

    // Ground Truth Observations:
    std::vector<std::pair<int, Vec4f>> groundTruthObservations;

    // Current number of points3Ds:
    int currentPointId = 0;

    // Should collect observations when processing?
    bool isCollectingObservations = false;

    // How often should observations be made?
    int collectEveryNFrame = 2;

    // Every time the process method is called this is increased by 1:
    unsigned int processingCount = 0;

    // Save cameras while calibrating (to restore when calibration is aborted):
    std::vector<CameraCalibration> savedCamerasWhileCalib;

    /**
     * Add points for all cameras at one timestamp.
     */
    void addObservation(std::vector<CameraFrame>& frames, std::vector<Vec4f> gtPositions = std::vector<Vec4f>());

    /**
     * Start calibration.
     */
    void calibrate();

    Vec4f guess3DPoint(ImageMarker raw2D);

    void filterMeasurement();

    void resetCalibrations();
    void createInitialGuess();
    void refineExtrinsicGuess();
    void refineExtrinsicIntrinsics();

    void groundTruthRegistration();

    void align3DPointsToGuess();

    void calculateStatistics();

    void saveCalibration();

    void loadCalibration();

public:
    // Cameras with parameters:
    std::vector<CameraCalibration> calibrations;

    // Optimized 3D points:
    std::vector<Vec4d> points3D;

    bool registerWithGroundTruth = true;

    CameraCalibrator(DataModel& model)
        : Processor(model){
        loadCalibration();

        calculatingThread = std::thread([this](){
            while(!shouldStopThread){
                triggerCalculatingSemaphore.acquire();
                if(shouldStopThread)
                    break;

                isOptimizing = true;
                calibrate();
                isOptimizing = false;
            }
        });
    }

    ~CameraCalibrator(){
        shouldStopThread = true;
        triggerCalculatingSemaphore.release();
        calculatingThread.join();
    }

    void triggerCalibration(){
        isCollectingObservations = false;
        triggerCalculatingSemaphore.release();
    }

    void clearObservations(){
        observations = std::map<int, std::vector<std::pair<int, ImageMarker>>>();
        calibrations.clear();
        points3D = std::vector<Vec4d>();
        currentPointId = 0;
    }

    std::map<int, std::vector<std::pair<int, ImageMarker>>>& getObservations(){
         return observations;
    }

    void startObservationCollection(){
        savedCamerasWhileCalib = calibrations;
        clearObservations();
        isCollectingObservations = true;
    }

    bool isOptimizingCalibration(){
        return isOptimizing;
    }

    bool getIsCollectingObservations(){
        return isCollectingObservations;
    }

    void abortObservationCollection(){
        calibrations = savedCamerasWhileCalib;
        isCollectingObservations = false;
    }

    virtual void process(TickData& tick) override {
        // If observations should be collected:
        if(isCollectingObservations){
            if(processingCount % collectEveryNFrame == 0)
                addObservation(tick.frames, tick.markersGTPos);
        }

        // Update the current calibration in the model:
        for(CameraCalibration& calibration : calibrations){
            if(model.cameras.count(calibration.unique_cam_id) > 0){
                model.cameras[calibration.unique_cam_id].calibration = calibration;
            }
        }

        ++processingCount;
    };
};
