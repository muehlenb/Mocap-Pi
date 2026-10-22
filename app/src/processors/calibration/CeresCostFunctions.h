#pragma once

#include <ceres/ceres.h>
#include <ceres/rotation.h>

/**
 * Reprojection error for ceres optimization.
 */
class ReprojectionErrorExtOnly  {
public:
    ReprojectionErrorExtOnly(double observed_x, double observed_y) : observed_x(observed_x), observed_y(observed_y) {}

    template <typename T>
    bool operator()(const T* const camera, const T* const point, T* residuals) const {
        // camera[0,1,2] are the angle-axis rotation.
        T p[3];
        ceres::AngleAxisRotatePoint(camera, point, p);

        // camera[3,4,5] are the translation.
        p[0] += camera[3];
        p[1] += camera[4];
        p[2] += camera[5];

        T xp = -p[0] / p[2];
        T yp = -p[1] / p[2];

        T predicted_x = T(1400) * xp + T(640);
        T predicted_y = T(1400) * yp + T(360);

        // The error is the difference between the predicted and observed position.
        residuals[0] = predicted_x - T(observed_x);
        residuals[1] = predicted_y - T(observed_y);
        return true;
    }

    // Factory to hide the construction of the CostFunction object from
    // the client code.
    static ceres::CostFunction* Create(const double observed_x,
                                       const double observed_y) {
        return (new ceres::AutoDiffCostFunction<ReprojectionErrorExtOnly, 2, 6, 3>(
            new ReprojectionErrorExtOnly(observed_x, observed_y)));
    }

private:
    double observed_x;
    double observed_y;
};

class ReprojectionError  {
public:
    ReprojectionError(double observed_x, double observed_y) : observed_x(observed_x), observed_y(observed_y) {}

    template <typename T>
    bool operator()(const T* const camera, const T* const point, T* residuals) const {
        // camera[0,1,2] are the angle-axis rotation.
        T projPoint[3];
        ceres::AngleAxisRotatePoint(camera, point, projPoint);

        // camera[3,4,5] are the translation.
        projPoint[0] += camera[3];
        projPoint[1] += camera[4];
        projPoint[2] += camera[5];

        T xn = -projPoint[0] / projPoint[2];
        T yn = -projPoint[1] / projPoint[2];

        const T* focal_length = &camera[6];
        const T* principal_point = &camera[8];
        const T* k = &camera[10];
        const T* p = &camera[13];

        T r2 = xn*xn + yn*yn;
        T x_distorted = xn * (T(1.0) + k[0]*r2 + k[1]*r2*r2 + k[2]*r2*r2*r2)
                        + T(2.0)*p[0]*xn*yn + p[1]*(r2 + T(2.0)*xn*xn);
        T y_distorted = yn * (T(1.0) + k[0]*r2 + k[1]*r2*r2 + k[2]*r2*r2*r2)
                        + p[0]*(r2 + T(2.0)*yn*yn) + T(2.0)*p[1]*xn*yn;

        // Applying distortion:
        T u = xn * focal_length[0] + principal_point[0];
        T v = yn * focal_length[1] + principal_point[1];

        // The error is the difference between the predicted and observed position.
        residuals[0] = u - T(observed_x);
        residuals[1] = v - T(observed_y);
        return true;
    }

    // Factory to hide the construction of the CostFunction object from
    // the client code.
    static ceres::CostFunction* Create(const double observed_x,
                                       const double observed_y) {
        return (new ceres::AutoDiffCostFunction<ReprojectionError, 2, 15, 3>(
            new ReprojectionError(observed_x, observed_y)));
    }

private:
    double observed_x;
    double observed_y;
};

/**
 * 3D Distance error for ceres optimization.
 */

struct DistanceCostFunctor {
    DistanceCostFunctor(double observed_distance) : observed_distance_(observed_distance) {}

    template <typename T>
    bool operator()(const T* const point1, const T* const point2, T* residual) const {
        residual[0] = (sqrt((point1[0] - point2[0]) * (point1[0] - point2[0]) +
                            (point1[1] - point2[1]) * (point1[1] - point2[1]) +
                            (point1[2] - point2[2]) * (point1[2] - point2[2])) - T(observed_distance_)) * T(250.0);
        return true;
    }

    static ceres::CostFunction* Create(double observed_distance) {
        return new ceres::AutoDiffCostFunction<DistanceCostFunctor, 1, 3, 3>(
            new DistanceCostFunctor(observed_distance));
    }
private:
    double observed_distance_;
};
