#pragma once

#include <Eigen/Dense>
#include <Eigen/SVD>
#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "src/model/DataModel.h"
#include "src/processors/Processor.h"

// Assumes DataModel defines RigidBodyDefinitionMode::{ToolTipOrigin, CentroidPCA}.
// Every input frame must contain exactly the selected body's marker count.
// Coordinates and the tolerances below are in meters.
class RigidBodyDefiner : public Processor {
    using Vec3 = Eigen::Vector3d;
    using Mat3 = Eigen::Matrix3d;
    using Points = std::vector<Vec3>;
    using Assignment = std::vector<int>;

    struct Transform {
        Mat3 R = Mat3::Identity();
        Vec3 t = Vec3::Zero();
    };

    static constexpr size_t MaxSamples = 800;
    static constexpr size_t BootstrapSamples = 10;
    static constexpr size_t UpdateInterval = 10;
    static constexpr double MaxPairError = 0.008; // 8 mm, compatible with detector's finalPairTolerance
    static constexpr double MaxDistanceRms = 0.004; // 4 mm

    struct Sample { size_t frame; Points points; };

    // Samples from the latest 800 incoming ticks; noisy ticks do not enter the history.
    // Marker identities are fixed after bootstrap.
    std::deque<Sample> history;
    std::deque<Sample> bootstrap;
    Points latest;
    Points reference;
    RigidBody* activeBody = nullptr;
    RigidBodyDefinitionMode activeMode = RigidBodyDefinitionMode::ToolTipOrigin;
    size_t acceptedSinceUpdate = 0;
    size_t rejectedStreak = 0;
    size_t frameSerial = 0;
    size_t activeMarkerCount = 0;

public:
    struct Feedback {
        size_t accepted = 0;
        size_t rejected = 0;
        size_t stored = 0;
        size_t definitions = 0;
        double lastMatchingRms = 0.0;
        double lastDefinitionRms = 0.0;
        std::string status = "Inactive";
    };

    Feedback feedback;

    explicit RigidBodyDefiner(DataModel& model) : Processor(model) {}

private:
    static Vec3 xyz(const Marker& marker) {
        return Vec3(marker.position.x, marker.position.y, marker.position.z);
    }

    static Points extract(const std::vector<Marker>& frame) {
        Points points;
        points.reserve(frame.size());
        for (const Marker& marker : frame) {
            Vec3 p = xyz(marker);
            if (!p.allFinite()) return {};
            points.push_back(p);
        }
        return points;
    }

    static double meanEdge(const Points& p) {
        double sum = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < p.size(); ++i)
            for (size_t j = i + 1; j < p.size(); ++j) { sum += (p[i] - p[j]).norm(); ++count; }
        return count ? sum / double(count) : 0.0;
    }

    static std::vector<double> sortedEdges(const Points& p) {
        std::vector<double> result;
        result.reserve(p.size() * (p.size() - 1) / 2);
        for (size_t i = 0; i < p.size(); ++i)
            for (size_t j = i + 1; j < p.size(); ++j) result.push_back((p[i] - p[j]).norm());
        std::sort(result.begin(), result.end());
        return result;
    }

    static double edgeRms(const std::vector<double>& a, const std::vector<double>& b) {
        if (a.size() != b.size() || a.empty()) return std::numeric_limits<double>::infinity();
        double sum = 0.0;
        for (size_t i = 0; i < a.size(); ++i) sum += (a[i] - b[i]) * (a[i] - b[i]);
        return std::sqrt(sum / double(a.size()));
    }

    static double distanceRms(const Points& a, const Points& b, double* maxError = nullptr) {
        double sum = 0.0, largest = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < a.size(); ++i)
            for (size_t j = i + 1; j < a.size(); ++j) {
                const double diff = std::abs((a[i] - a[j]).norm() - (b[i] - b[j]).norm());
                sum += diff * diff;
                largest = std::max(largest, diff);
                ++count;
            }
        if (maxError) *maxError = largest;
        return count ? std::sqrt(sum / double(count)) : std::numeric_limits<double>::infinity();
    }

    static Assignment hungarian(const Eigen::MatrixXd& cost) {
        const int n = int(cost.rows());
        const double inf = std::numeric_limits<double>::infinity();
        std::vector<double> u(n + 1), v(n + 1);
        std::vector<int> p(n + 1), way(n + 1);
        for (int i = 1; i <= n; ++i) {
            p[0] = i;
            int j0 = 0;
            std::vector<double> minv(n + 1, inf);
            std::vector<bool> used(n + 1, false);
            do {
                used[j0] = true;
                const int i0 = p[j0];
                int j1 = 0;
                double delta = inf;
                for (int j = 1; j <= n; ++j) {
                    if (used[j]) continue;
                    const double cur = cost(i0 - 1, j - 1) - u[i0] - v[j];
                    if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
                    if (minv[j] < delta) { delta = minv[j]; j1 = j; }
                }
                for (int j = 0; j <= n; ++j) {
                    if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
                    else minv[j] -= delta;
                }
                j0 = j1;
            } while (p[j0] != 0);
            do {
                const int j1 = way[j0];
                p[j0] = p[j1];
                j0 = j1;
            } while (j0 != 0);
        }
        Assignment assignment(n);
        for (int j = 1; j <= n; ++j) assignment[p[j] - 1] = j - 1;
        return assignment;
    }

    static Transform align(const Points& target, const Points& source) {
        const size_t n = target.size();
        Vec3 a = Vec3::Zero(), b = Vec3::Zero();
        for (size_t i = 0; i < n; ++i) { a += source[i]; b += target[i]; }
        a /= double(n);
        b /= double(n);
        Mat3 H = Mat3::Zero();
        for (size_t i = 0; i < n; ++i) H += (source[i] - a) * (target[i] - b).transpose();
        Eigen::JacobiSVD<Mat3> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
        Mat3 D = Mat3::Identity();
        D(2, 2) = (svd.matrixV() * svd.matrixU().transpose()).determinant() < 0.0 ? -1.0 : 1.0;
        Transform result;
        result.R = svd.matrixV() * D * svd.matrixU().transpose();
        result.t = b - result.R * a;
        return result;
    }

    static Points reorder(const Points& points, const Assignment& assignment) {
        Points result(assignment.size());
        for (size_t i = 0; i < assignment.size(); ++i) result[i] = points[assignment[i]];
        return result;
    }

    static Assignment match(const Points& target, const Points& observed) {
        const int n = int(target.size());
        std::vector<std::vector<double>> a(n), b(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (i == j) continue;
                a[i].push_back((target[i] - target[j]).norm());
                b[i].push_back((observed[i] - observed[j]).norm());
            }
            std::sort(a[i].begin(), a[i].end());
            std::sort(b[i].begin(), b[i].end());
        }
        Eigen::MatrixXd cost(n, n);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
                double sum = 0.0;
                for (int k = 0; k < n - 1; ++k) sum += (a[i][k] - b[j][k]) * (a[i][k] - b[j][k]);
                cost(i, j) = sum;
            }
        Assignment assignment = hungarian(cost);
        for (int iteration = 0; iteration < 4; ++iteration) {
            const Transform transform = align(target, reorder(observed, assignment));
            for (int i = 0; i < n; ++i)
                for (int j = 0; j < n; ++j) cost(i, j) = (target[i] - (transform.R * observed[j] + transform.t)).squaredNorm();
            Assignment next = hungarian(cost);
            if (next == assignment) break;
            assignment = std::move(next);
        }
        return assignment;
    }

    bool compatible(const Points& ordered, double* rms = nullptr) const {
        double maxError = 0.0;
        const double error = distanceRms(reference, ordered, &maxError);
        if (rms) *rms = error;
        return error <= MaxDistanceRms && maxError <= MaxPairError;
    }

    void reset(RigidBody* body, RigidBodyDefinitionMode mode) {
        activeBody = body;
        activeMode = mode;
        activeMarkerCount = body ? body->markers.size() : 0;
        history.clear();
        bootstrap.clear();
        latest.clear();
        reference.clear();
        acceptedSinceUpdate = 0;
        rejectedStreak = 0;
        feedback = Feedback{};
        feedback.status = body ? "Collecting bootstrap frames" : "Inactive";
    }

    // Choose a medoid, not the first frame: a single noisy frame cannot define our reference.
    bool bootstrapReference() {
        if (bootstrap.size() < BootstrapSamples) return false;
        size_t best = 0;
        double bestScore = std::numeric_limits<double>::infinity();
        std::vector<std::vector<double>> edges;
        edges.reserve(bootstrap.size());
        for (const Sample& sample : bootstrap) edges.push_back(sortedEdges(sample.points));
        for (size_t i = 0; i < bootstrap.size(); ++i) {
            double score = 0.0;
            for (size_t j = 0; j < bootstrap.size(); ++j) score += edgeRms(edges[i], edges[j]);
            if (score < bestScore) { bestScore = score; best = i; }
        }
        reference = bootstrap[best].points;
        if (meanEdge(reference) < 1e-9) return false;
        for (const Sample& raw : bootstrap) {
            const Points ordered = reorder(raw.points, match(reference, raw.points));
            if (!compatible(ordered)) { ++feedback.rejected; continue; }
            history.push_back({raw.frame, ordered});
            latest = ordered;
            ++feedback.accepted;
        }
        bootstrap.clear();
        feedback.stored = history.size();
        if (history.size() < 5) { history.clear(); latest.clear(); reference.clear(); return false; }
        feedback.status = "Tracking marker identities";
        acceptedSinceUpdate = history.size();
        return true;
    }

    // Robust mean of registered frames. Discard the 20% with the largest edge error.
    Points buildReference(double& rms) const {
        Points estimate = reference;
        std::vector<size_t> selected;
        for (int iteration = 0; iteration < 3; ++iteration) {
            std::vector<std::pair<double, size_t>> ranked;
            ranked.reserve(history.size());
            for (size_t t = 0; t < history.size(); ++t) ranked.emplace_back(distanceRms(estimate, history[t].points), t);
            std::sort(ranked.begin(), ranked.end());
            const size_t keep = std::max<size_t>(3, (ranked.size() * 8 + 9) / 10);
            selected.clear();
            for (size_t k = 0; k < std::min(keep, ranked.size()); ++k) selected.push_back(ranked[k].second);
            Points sum(estimate.size(), Vec3::Zero());
            for (size_t idx : selected) {
                const Transform transform = align(estimate, history[idx].points); // frame -> reference
                for (size_t i = 0; i < sum.size(); ++i) sum[i] += transform.R * history[idx].points[i] + transform.t;
            }
            for (size_t i = 0; i < sum.size(); ++i) sum[i] /= double(selected.size());
            estimate.swap(sum);
        }
        double squared = 0.0;
        for (size_t idx : selected) { const double e = distanceRms(estimate, history[idx].points); squared += e * e; }
        rms = std::sqrt(squared / double(selected.size()));
        return estimate;
    }

    static Vec3 estimatePivot(const Points& reference, const std::deque<Sample>& history) {
        // Reference coordinates are anchored to the medoid frame's WORLD coordinates.
        // For a stationary pivot c: (I-R_t)*c = t_t, where x_world = R_t*x_reference+t_t.
        std::vector<Transform> poses;
        poses.reserve(history.size());
        for (const Sample& frame : history) poses.push_back(align(frame.points, reference));
        Vec3 pivot = Vec3::Zero();
        std::vector<double> weights(poses.size(), 1.0);
        for (int iteration = 0; iteration < 5; ++iteration) {
            Mat3 H = Mat3::Zero();
            Vec3 g = Vec3::Zero();
            for (size_t t = 0; t < poses.size(); ++t) {
                const Mat3 A = Mat3::Identity() - poses[t].R;
                H += weights[t] * A.transpose() * A;
                g += weights[t] * A.transpose() * poses[t].t;
            }
            Eigen::SelfAdjointEigenSolver<Mat3> eig(H);
            if (eig.info() != Eigen::Success || eig.eigenvalues()[2] < 1e-10 || eig.eigenvalues()[0] / eig.eigenvalues()[2] < 1e-5)
                throw std::runtime_error("Pivot not observable: rotate tool around multiple axes");
            pivot = H.ldlt().solve(g);
            for (size_t t = 0; t < poses.size(); ++t) {
                const double error = ((Mat3::Identity() - poses[t].R) * pivot - poses[t].t).norm();
                weights[t] = std::min(1.0, 0.005 / std::max(error, 1e-12)); // Huber, 5 mm
            }
        }
        if (!pivot.allFinite()) throw std::runtime_error("Invalid pivot");
        double pivotSquared = 0.0;
        for (const Transform& pose : poses) {
            const Vec3 error = (Mat3::Identity() - pose.R) * pivot - pose.t;
            pivotSquared += error.squaredNorm();
        }
        const double pivotRms = std::sqrt(pivotSquared / double(poses.size()));
        if (pivotRms > 0.015) throw std::runtime_error("No sufficiently stationary pivot (RMS > 15 mm)");
        const double scale = meanEdge(reference);
        Vec3 centroid = Vec3::Zero();
        for (const Vec3& p : reference) centroid += p;
        centroid /= double(reference.size());
        if ((pivot - centroid).norm() > 20.0 * scale)
            throw std::runtime_error("Implausible pivot: insufficient rotation");
        return pivot;
    }

    static RigidBody define(const Points& points, const Vec3& pivot, RigidBodyDefinitionMode mode) {
        const size_t n = points.size();
        Vec3 centroid = Vec3::Zero();
        for (const Vec3& p : points) centroid += p;
        centroid /= double(n);
        Mat3 covariance = Mat3::Zero();
        for (const Vec3& p : points) covariance += (p - centroid) * (p - centroid).transpose();
        Eigen::SelfAdjointEigenSolver<Mat3> eig(covariance);
        if (eig.info() != Eigen::Success || eig.eigenvalues()[1] < 1e-10 * std::max(eig.eigenvalues()[2], 1e-12))
            throw std::runtime_error("Marker layout is (almost) collinear");
        Vec3 origin = centroid;
        Vec3 x, y;
        if (mode == RigidBodyDefinitionMode::ToolTipOrigin) {
            origin = pivot;
            size_t closest = 0;
            for (size_t i = 1; i < n; ++i)
                if ((points[i] - origin).squaredNorm() < (points[closest] - origin).squaredNorm()) closest = i;
            y = points[closest] - origin;
            if (y.norm() < 1e-8) throw std::runtime_error("Pivot coincides with closest marker");
            y.normalize();
            const Mat3 P = Mat3::Identity() - y * y.transpose();
            Eigen::SelfAdjointEigenSolver<Mat3> planar(P * covariance * P);
            if (planar.info() != Eigen::Success || planar.eigenvalues()[2] < 1e-12)
                throw std::runtime_error("No stable tool X axis");
            x = planar.eigenvectors().col(2).normalized();
        } else {
            x = eig.eigenvectors().col(2).normalized();
            y = eig.eigenvectors().col(1).normalized();
        }
        const Vec3 anchor = Vec3::UnitX() - y.dot(Vec3::UnitX()) * y;
        if (anchor.norm() > 1e-6 && x.dot(anchor) < 0.0) x = -x;
        Vec3 z = x.cross(y);
        if (z.norm() < 1e-9) throw std::runtime_error("Degenerate coordinate axes");
        z.normalize();
        x = y.cross(z).normalized();
        if (mode == RigidBodyDefinitionMode::CentroidPCA && y.dot(Vec3::UnitY()) < 0.0) { y = -y; z = -z; }
        Mat3 R;
        R.col(0) = x;
        R.col(1) = y;
        R.col(2) = z;
        std::vector<RigidBodyMarker> result;
        result.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            const Vec3 local = R.transpose() * (points[i] - origin);
            RigidBodyMarker marker(Vec4f(float(local.x()), float(local.y()), float(local.z()), 1.0f));
            marker.id = int(i);
            result.push_back(marker);
        }
        RigidBody body(result); // init() is called once here
        body.isTracked = false;
        return body;
    }

    void updateDefinition(RigidBody& target) {
        double rms = 0.0;
        Points newReference = buildReference(rms);
        if (rms > MaxDistanceRms) throw std::runtime_error("Reference geometry too noisy");
        const Vec3 pivot = activeMode == RigidBodyDefinitionMode::ToolTipOrigin ? estimatePivot(newReference, history) : Vec3::Zero();
        RigidBody result = define(newReference, pivot, activeMode);
        // Only commit a complete, validated definition. Keep label, tracking and other state.
        target.markers = std::move(result.markers);
        target.init();
        reference = std::move(newReference);
        feedback.lastDefinitionRms = rms;
        ++feedback.definitions;
        feedback.status = "Definition updated";
    }

public:
    void process(TickData& tick) override {
        ++frameSerial;
        RigidBody* target = nullptr;
        for (RigidBody& rb : model.rigidBodies)
            if (&rb == model.redefineRigidBody) { target = &rb; break; }
        if (!target && model.redefineRigidBody) model.redefineRigidBody = nullptr;
        const auto mode = model.redefineRigidBodyMode;
        if (target != activeBody || mode != activeMode || (target && target->markers.size() != activeMarkerCount)) reset(target, mode);
        if (!target) return;

        // Expire samples based on their actual age, including rejected/missing ticks.
        while (!history.empty() && frameSerial - history.front().frame >= MaxSamples) history.pop_front();
        while (!bootstrap.empty() && frameSerial - bootstrap.front().frame >= MaxSamples) bootstrap.pop_front();
        feedback.stored = history.size();
        if (history.empty() && !reference.empty()) { latest.clear(); reference.clear(); acceptedSinceUpdate = 0; }

        const size_t n = target->markers.size();
        if (n < 3 || model.markers.size() != n) { ++feedback.rejected; feedback.status = "Wrong marker count"; return; }
        Points current = extract(model.markers);
        if (current.size() != n) { ++feedback.rejected; feedback.status = "Nonfinite marker position"; return; }

        if (reference.empty()) {
            bootstrap.push_back({frameSerial, std::move(current)});
            if (bootstrap.size() > BootstrapSamples) bootstrap.pop_front();
            if (!bootstrapReference()) { feedback.status = "Collecting valid bootstrap frames"; return; }
        } else {
            // Match once, at arrival. Never reassign the identities of old frames.
            const Points ordered = reorder(current, match(latest, current));
            double rms = 0.0;
            if (!compatible(ordered, &rms)) {
                ++feedback.rejected;
                ++rejectedStreak;
                feedback.lastMatchingRms = rms;
                feedback.status = "Rejected geometry outlier";
                // If the reference becomes unusable, restart bootstrap instead of freezing forever.
                if (rejectedStreak >= 100) reset(target, mode);
                return;
            }
            rejectedStreak = 0;
            latest = ordered;
            history.push_back({frameSerial, std::move(ordered)});
            if (history.size() > MaxSamples) history.pop_front();
            ++feedback.accepted;
            ++acceptedSinceUpdate;
            feedback.lastMatchingRms = rms;
            feedback.stored = history.size();
            feedback.status = "Collecting accepted frames";
        }

        if (history.size() < BootstrapSamples || acceptedSinceUpdate < UpdateInterval) return;
        acceptedSinceUpdate = 0;
        try { updateDefinition(*target); }
        catch (const std::exception& e) { feedback.status = e.what(); }
    }
};
