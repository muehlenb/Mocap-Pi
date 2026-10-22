#include "src/processors/marker/RawMarkerTriangulator.h"

using namespace std::chrono;

std::vector<RawMarkerTriangulator::Ray> RawMarkerTriangulator::getAllRays(std::map<int, Camera>& cameras, std::vector<CameraFrame>& frames){
    if(cameras.empty() || frames.empty())
        return std::vector<Ray>();

    // Vector of all rays:
    std::vector<Ray> allRays;

    size_t markerNum = 0;
    for(CameraFrame& frame : frames){
        if(frame.valid)
            markerNum += frame.markers.size();
    }
    allRays.reserve(markerNum);

    for(unsigned int i = 0; i < frames.size(); ++i){
        if(frames[i].valid){
            auto camera = cameras.find(frames[i].uniqueCamID);

            if(camera != cameras.end()){
                CameraCalibration& calibration = camera->second.calibration;
                CameraFrame& frame = frames[i];

                Mat4f matrix = calibration.getModelMatrix();
                Mat4d matrixD = matrix.toMat4d();
                Vec4d pos = matrix.getPosition().toVec4d();

                for(unsigned int mId = 0; mId < frame.markers.size(); ++mId){
                    ImageMarker& marker = frame.markers[mId];

                    Vec4d ray = matrixD * calibration.getRay(marker.x, marker.y).toVec4d();

                    allRays.emplace_back(pos, ray, ImageMarkerId(i, mId));
                }
            }
        }
    }

    return allRays;
}

std::vector<RawMarkerTriangulator::Intersection> RawMarkerTriangulator::getIntersections(std::vector<Ray>& allRays){
    size_t rayNum = allRays.size();

    std::vector<Intersection> intersections;
    intersections.reserve(rayNum * 4);

    const double maxIntersectionDistanceSquared = MAX_INTERSECTION_DISTANCE * MAX_INTERSECTION_DISTANCE;

    for(size_t i=0; i < rayNum; ++i){
        Ray& rayI = allRays[i];
        Vec4d& posI = rayI.origin;
        Vec4d& dirI = rayI.direction;

        for(size_t j=0; j < i; ++j){
            Ray& rayJ = allRays[j];
            Vec4d& posJ = rayJ.origin;
            Vec4d& dirJ = rayJ.direction;

            // Rays from the same camera should be ignored:
            if(rayI.imageMarker.camId == rayJ.imageMarker.camId)
                continue;

            const Vec4d n = dirI.cross(dirJ);

            if(n.dot(n) < 0.000000000001)
                continue;

            const Vec4d n1 = dirI.cross(n);
            const Vec4d n2 = dirJ.cross(n);

            const double dirIDotN2 = dirI.dot(n2);
            const double dirJDotN1 = dirJ.dot(n1);

            if(std::abs(dirIDotN2) < 0.1 || std::abs(dirJDotN1) < 0.1)
                continue;

            const Vec4d posDifferenceJI = posJ-posI;
            const Vec4d posDifferenceIJ = posI-posJ;

            const Vec4d c1 = posI + dirI * (posDifferenceJI.dot(n2) / dirIDotN2);
            const Vec4d c2 = posJ + dirJ * (posDifferenceIJ.dot(n1) / dirJDotN1);

            const double dX = c1.x - c2.x;
            const double dY = c1.y - c2.y;
            const double dZ = c1.z - c2.z;

            const double distance = dX*dX + dY*dY + dZ*dZ;

            if(distance < maxIntersectionDistanceSquared){
                Vec4d p = (c1 + c2) / 2;
                intersections.emplace_back(rayI.imageMarker, rayJ.imageMarker, p.toVec4f());
            }
        }
    }

    return intersections;
}

std::vector<RawMarkerTriangulator::Cluster> RawMarkerTriangulator::generateClusters(std::vector<Intersection>& intersections){
    std::vector<Cluster> clusters;
    clusters.reserve(intersections.size() / 2 + 1);

    const float maxClusterDistanceSquared = MAX_CLUSTER_DISTANCE * MAX_CLUSTER_DISTANCE;

    // Insert intersections into clusters:
    for(Intersection& intersection : intersections){
        // Inserted into cluster:
        bool insertedInExistingCluster = false;

        // Insert into existing clusters:
        for(Cluster& cluster : clusters){
            bool pointFitsCluster = false;

            for(Vec4f& clusterPoint : cluster.points){
                const float dX = clusterPoint.x - intersection.point.x;
                const float dY = clusterPoint.y - intersection.point.y;
                const float dZ = clusterPoint.z - intersection.point.z;

                if(dX*dX + dY*dY + dZ*dZ < maxClusterDistanceSquared){
                    pointFitsCluster = true;
                    break;
                }
            }

            if(pointFitsCluster){
                cluster.points.push_back(intersection.point);
                cluster.markers.insert(intersection.i);
                cluster.markers.insert(intersection.j);
                insertedInExistingCluster = true;
                break;
            }
        }

        // If it couldn't inserted into existing clusters, create new:
        if(!insertedInExistingCluster){
            Cluster cluster;
            cluster.points.reserve(8);
            cluster.points.push_back(intersection.point);
            cluster.markers.insert(intersection.i);
            cluster.markers.insert(intersection.j);
            clusters.push_back(std::move(cluster));
        }
    }


    // Evaluate clusters:
    for(Cluster& cluster : clusters){
        cluster.score = cluster.points.size();
    }

    std::sort(clusters.begin(), clusters.end(),[](Cluster& a, Cluster& b){
        return a.score > b.score;
    });

    return clusters;
}

std::vector<RawMarker> RawMarkerTriangulator::generateUnlabeled3DMarkers(
    DataModel& model,
    TickData& tick,
    float* medianInnerClusterDistance,
    float* meanClusterNum,
    float* medianGroundTruthDistance
    ){


    // Get all rays (unprojected markers) of all cameras:
    std::vector<Ray> allRays = getAllRays(model.cameras, tick.frames);

    // Get all intersections between that rays (of different cameras):
    std::vector<Intersection> intersections = getIntersections(allRays);

    tick.triangulationIntersections.clear();
    tick.triangulationIntersections.reserve(intersections.size());

    for(Intersection& intersection : intersections){
        tick.triangulationIntersections.push_back(intersection.point);
    }

    std::vector<RawMarker> result;

    std::vector<Cluster> clusters = generateClusters(intersections);
    result.reserve(clusters.size());

    /*
    std::vector<float> distancesInClusters;
    float sumClusterNum = 0;

    for(Cluster& cluster : clusters) {
        auto end = cluster.points.end();
        for(auto it = cluster.points.begin(); it != end; ++it) {
            Vec4f pI = *it;
            auto it2 = it;
            for(++it2; it2 != end; ++it2) {
                Vec4f pH = *it2;
                float dX = pI.x - pH.x;
                float dY = pI.y - pH.y;
                float dZ = pI.z - pH.z;

                distancesInClusters.push_back(dX*dX + dY*dY + dZ*dZ);
            }
        }
        sumClusterNum += cluster.points.size();
    }

    if(distancesInClusters.size() > 0){
        std::sort(distancesInClusters.begin(), distancesInClusters.end());

        if(medianInnerClusterDistance != nullptr)
            *medianInnerClusterDistance = *medianInnerClusterDistance * 0.95f + sqrt(distancesInClusters[distancesInClusters.size()/2]) * 0.05f;

        if(meanClusterNum != nullptr && clusters.size() > 0)
            *meanClusterNum = *meanClusterNum * 0.95f + (sumClusterNum / clusters.size()) * 0.05f;

    }
    */

    std::unordered_set<ImageMarkerId> consideredMarkers;
    consideredMarkers.reserve(allRays.size());

    for(Cluster& cluster : clusters){
        int rayNum = 0;

        for(const ImageMarkerId& rayId : cluster.markers){
            if(consideredMarkers.count(rayId) == 0)
                ++rayNum;
        }

        if(rayNum >= MIN_RAY_INTERSECTIONS){
            for(const ImageMarkerId& rayId : cluster.markers)
                consideredMarkers.insert(rayId);

            // Find the middlest points:
            std::vector<float> x;
            std::vector<float> y;
            std::vector<float> z;

            x.reserve(cluster.points.size());
            y.reserve(cluster.points.size());
            z.reserve(cluster.points.size());

            for(Vec4f& p : cluster.points){
                x.push_back(p.x);
                y.push_back(p.y);
                z.push_back(p.z);
            }

            for(auto& item : cluster.markers){
                tick.frames[item.camId].markers[item.markerId].marker3DIdx = result.size();
            }

            size_t num = x.size() / 2;

            std::nth_element(x.begin(), x.begin() + num, x.end());
            std::nth_element(y.begin(), y.begin() + num, y.end());
            std::nth_element(z.begin(), z.begin() + num, z.end());

            Vec4f med(x[num], y[num], z[num]);
            result.push_back(med);
        }
    }

    if(medianGroundTruthDistance != nullptr){
        if(tick.markersGTPos.size() > 0){
            std::vector<float> groundTruthDistances;
            groundTruthDistances.reserve(result.size());

            for(RawMarker& marker : result){
                Vec4f& markerPos = marker.position;

                float minDistance = 999999.f;
                for(Vec4f& gt : tick.markersGTPos){
                    float distance = gt.distanceTo(markerPos);

                    if(distance < minDistance){
                        minDistance = distance;
                    }
                }

                groundTruthDistances.push_back(minDistance);
            }

            if(groundTruthDistances.size()){
                size_t num = groundTruthDistances.size() / 2;

                std::nth_element(
                    groundTruthDistances.begin(),
                    groundTruthDistances.begin() + num,
                    groundTruthDistances.end()
                    );

                *medianGroundTruthDistance = groundTruthDistances[num];
            } else {
                *medianGroundTruthDistance = -1.f;
            }
        } else {
            *medianGroundTruthDistance = -1.f;
        }
    }

    return result;
}

void RawMarkerTriangulator::process(TickData& tick){
    tick.markers = generateUnlabeled3DMarkers(model, tick, &medianInnerClusterDistance, &meanClusterNum, &medianGroundTruthDistance);

    // if(tick.markers.size() == 3){
    //     std::cout << tick.markers[0].position.distanceTo(tick.markers[1].position) << " " << tick.markers[1].position.distanceTo(tick.markers[2].position) << std::endl;
    // }
}
