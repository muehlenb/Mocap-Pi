#pragma once

#include <tuple>

/**
 * Stores data of calibrations sticks in a way that is easily accessable
 * by ImGui.
 */
struct CalibrationSticks{
    /**
     * The current selected calibration stick type (id).
     */
    static int selectedCalibStick;

    /**
     * Names of the calibration sticks by id.
     */
    static const char* calibrationStickNames[];

    /**
     * Number of different calibration sticks.
     */
    static unsigned int calibrationStickNum;

    /**
     * Lengths of calibration sticks
     */
    static const std::tuple<float, float> calibrationStickLengths[];

    /**
     * Returns the total (lId = 0) length or the length of the first (lId = 1)
     * or the second (lId = 2) part of the current selected calibration stick.
     */
    static float getSelectedLength(int lId = 0){
        int calibId = CalibrationSticks::selectedCalibStick;
        std::tuple<float, float> lengths = CalibrationSticks::calibrationStickLengths[calibId];

        if(lId == 1)
            return std::get<0>(lengths);
        else if(lId == 2)
            return std::get<1>(lengths);

        return std::get<0>(lengths) + std::get<1>(lengths);
    }

};
