#pragma once

#define MARKERFINDER_THREAD_NUM 1
#define MAX_REGION_SIZE 1000
#define MAX_THREAD_REGION_SIZE \
    (MAX_REGION_SIZE / MARKERFINDER_THREAD_NUM)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Vec4f.h"

/**
 * Finds bright connected regions in an 8-bit grayscale/IR image.
 *
 * Pixels are connected when their horizontal scan regions overlap in
 * directly adjacent image rows.
 */
class MarkerFinder
{
private:
    /**
     * One continuous bright region inside a single image row.
     *
     * The parent and treeSize fields implement an index-based
     * disjoint-set/union-find data structure.
     *
     * Weighted marker values are calculated directly while scanning,
     * so the image does not need to be traversed again later.
     */
    struct ScanRegion
    {
        int startX = 0;
        int endX = 0;
        int y = 0;

        int parent = -1;
        int treeSize = 1;

        float weightedX = 0.0f;
        float weightedY = 0.0f;
        float weight = 0.0f;

        int pixelCount = 0;
    };

    /**
     * Brightness threshold above which a pixel belongs to a marker.
     */
    uint8_t brightnessThreshold = 20;

    /**
     * Finds all horizontal bright regions in a range of image rows.
     *
     * This function can safely be called from multiple threads as long
     * as brightnessThreshold is not changed while findInImage() runs.
     */
    std::vector<ScanRegion> findScanRegions(
        const uint8_t* image,
        int startRow,
        int endRow,
        int width) const
    {
        std::vector<ScanRegion> regions;
        regions.reserve(MAX_THREAD_REGION_SIZE);

        const float inverseThreshold =
            1.0f / static_cast<float>(brightnessThreshold);

        for (int y = startRow; y < endRow; ++y) {
            const int rowStart = y * width;

            bool regionActive = false;
            ScanRegion currentRegion;

            for (int x = 0; x < width; ++x) {
                const int imageIndex = rowStart + x;
                const uint8_t pixel = image[imageIndex];

                if (pixel > brightnessThreshold) {
                    if (!regionActive) {
                        currentRegion = ScanRegion {};
                        currentRegion.startX = x;
                        currentRegion.endX = x;
                        currentRegion.y = y;

                        regionActive = true;
                    }

                    currentRegion.endX = x;

                    const float ir =
                        static_cast<float>(pixel) * inverseThreshold;

                    currentRegion.weightedX +=
                        static_cast<float>(x) * ir;

                    currentRegion.weightedY +=
                        static_cast<float>(y) * ir;

                    currentRegion.weight += ir;
                    ++currentRegion.pixelCount;
                }
                else if (regionActive) {
                    regions.push_back(currentRegion);
                    regionActive = false;

                    if (regions.size() >= MAX_THREAD_REGION_SIZE) {
                        return regions;
                    }
                }
            }

            // Close a region that reaches the right image border.
            if (regionActive) {
                regions.push_back(currentRegion);

                if (regions.size() >= MAX_THREAD_REGION_SIZE) {
                    return regions;
                }
            }
        }

        return regions;
    }

    /**
     * Returns the root of the union-find tree and applies path compression.
     */
    static int getRoot(
        std::vector<ScanRegion>& regions,
        int regionIndex)
    {
        int root = regionIndex;

        while (regions[root].parent != root) {
            root = regions[root].parent;
        }

        // Path compression.
        while (regions[regionIndex].parent != regionIndex) {
            const int next = regions[regionIndex].parent;
            regions[regionIndex].parent = root;
            regionIndex = next;
        }

        return root;
    }

    /**
     * Joins two connected scan regions.
     */
    static void unite(
        std::vector<ScanRegion>& regions,
        int firstIndex,
        int secondIndex)
    {
        int firstRoot = getRoot(regions, firstIndex);
        int secondRoot = getRoot(regions, secondIndex);

        if (firstRoot == secondRoot) {
            return;
        }

        // Attach the smaller tree to the larger tree.
        if (regions[firstRoot].treeSize <
            regions[secondRoot].treeSize) {
            std::swap(firstRoot, secondRoot);
        }

        ScanRegion& destination = regions[firstRoot];
        ScanRegion& source = regions[secondRoot];

        source.parent = firstRoot;
        destination.treeSize += source.treeSize;

        // Aggregate marker values immediately.
        destination.weightedX += source.weightedX;
        destination.weightedY += source.weightedY;
        destination.weight += source.weight;
        destination.pixelCount += source.pixelCount;
    }

    /**
     * Merges overlapping scan regions in consecutive image rows.
     */
    static void mergeScanRegions(
        std::vector<ScanRegion>& regions,
        int height)
    {
        if (regions.empty() || height <= 1) {
            return;
        }

        /*
         * rowOffsets[y] is the first region belonging to row y.
         * The regions are already ordered by row because they were
         * generated and combined in image order.
         */
        std::vector<std::size_t> rowOffsets(
            static_cast<std::size_t>(height) + 1,
            regions.size());

        std::size_t position = 0;

        for (int y = 0; y < height; ++y) {
            rowOffsets[static_cast<std::size_t>(y)] = position;

            while (position < regions.size() &&
                   regions[position].y == y) {
                ++position;
            }
        }

        rowOffsets[static_cast<std::size_t>(height)] = position;

        /*
         * Compare the horizontal intervals of each row with those of
         * the row directly below it.
         */
        for (int y = 0; y < height - 1; ++y) {
            std::size_t upper =
                rowOffsets[static_cast<std::size_t>(y)];

            const std::size_t upperEnd =
                rowOffsets[static_cast<std::size_t>(y + 1)];

            std::size_t lower =
                rowOffsets[static_cast<std::size_t>(y + 1)];

            const std::size_t lowerEnd =
                rowOffsets[static_cast<std::size_t>(y + 2)];

            while (upper < upperEnd && lower < lowerEnd) {
                const int upperStart = regions[upper].startX;
                const int upperFinish = regions[upper].endX;

                const int lowerStart = regions[lower].startX;
                const int lowerFinish = regions[lower].endX;

                if (upperFinish < lowerStart) {
                    ++upper;
                    continue;
                }

                if (lowerFinish < upperStart) {
                    ++lower;
                    continue;
                }

                /*
                 * The regions overlap horizontally and are located in
                 * consecutive rows, so they belong to the same marker.
                 */
                unite(
                    regions,
                    static_cast<int>(upper),
                    static_cast<int>(lower));

                /*
                 * Advance the interval which ends first. It cannot
                 * overlap any further interval in the other row.
                 */
                if (upperFinish < lowerFinish) {
                    ++upper;
                }
                else if (lowerFinish < upperFinish) {
                    ++lower;
                }
                else {
                    ++upper;
                    ++lower;
                }
            }
        }
    }

    /**
     * Converts root regions into marker results.
     *
     * This preserves the output format of the original implementation:
     *
     * Vec4f(
     *     weighted X sum,
     *     weighted Y sum,
     *     pixel count,
     *     total weight
     * )
     */
    static std::vector<Vec4f> createMarkerResults(
        std::vector<ScanRegion>& regions)
    {
        std::vector<Vec4f> result;
        result.reserve(regions.size());

        for (int i = 0;
             i < static_cast<int>(regions.size());
             ++i) {
            if (getRoot(regions, i) != i) {
                continue;
            }

            const ScanRegion& region = regions[i];

            result.emplace_back(
                region.weightedX,
                region.weightedY,
                static_cast<float>(region.pixelCount),
                region.weight);
        }

        return result;
    }

public:
    /**
     * Finds markers in an 8-bit grayscale/IR image.
     *
     * image must contain at least width * height bytes.
     */
    std::vector<Vec4f> findInImage(
        const uint8_t* image,
        int width,
        int height) const
    {
        if (image == nullptr ||
            width <= 0 ||
            height <= 0 ||
            brightnessThreshold == 0) {
            return {};
        }

        static_assert(
            MARKERFINDER_THREAD_NUM > 0,
            "MARKERFINDER_THREAD_NUM must be greater than zero");

        static_assert(
            MARKERFINDER_THREAD_NUM <= MAX_REGION_SIZE,
            "MARKERFINDER_THREAD_NUM must not exceed MAX_REGION_SIZE");

        std::array<
            std::vector<ScanRegion>,
            MARKERFINDER_THREAD_NUM
        > splitRegions;

#if MARKERFINDER_THREAD_NUM > 1
        #pragma omp parallel for
#endif
        for (int threadIndex = 0;
             threadIndex < MARKERFINDER_THREAD_NUM;
             ++threadIndex) {
            /*
             * This calculation distributes remainder rows correctly.
             * For example, 361 rows are not truncated when divided
             * between multiple threads.
             */
            const int startRow =
                threadIndex * height /
                MARKERFINDER_THREAD_NUM;

            const int endRow =
                (threadIndex + 1) * height /
                MARKERFINDER_THREAD_NUM;

            splitRegions[threadIndex] = findScanRegions(
                image,
                startRow,
                endRow,
                width);
        }

        std::size_t totalRegionCount = 0;

        for (const auto& threadRegions : splitRegions) {
            /*
             * Reaching the limit means that the result may have been
             * truncated, so return no markers, matching the behavior
             * of the original implementation.
             */
            if (threadRegions.size() >=
                MAX_THREAD_REGION_SIZE) {
                return {};
            }

            totalRegionCount += threadRegions.size();
        }

        if (totalRegionCount > MAX_REGION_SIZE) {
            return {};
        }

        std::vector<ScanRegion> regions;
        regions.reserve(totalRegionCount);

        for (auto& threadRegions : splitRegions) {
            regions.insert(
                regions.end(),
                threadRegions.begin(),
                threadRegions.end());
        }

        /*
         * Initialize global union-find indices after combining the
         * per-thread vectors.
         */
        for (int i = 0;
             i < static_cast<int>(regions.size());
             ++i) {
            regions[i].parent = i;
            regions[i].treeSize = 1;
        }

        mergeScanRegions(regions, height);

        return createMarkerResults(regions);
    }

    /**
     * Returns the current brightness threshold.
     */
    uint8_t getBrightnessThreshold() const
    {
        return brightnessThreshold;
    }

    /**
     * Sets the brightness threshold.
     *
     * Zero is converted to one to prevent division by zero.
     */
    void setBrightnessThreshold(uint8_t threshold)
    {
        brightnessThreshold =
            std::max<uint8_t>(threshold, 1);
    }
};