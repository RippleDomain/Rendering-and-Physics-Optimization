/*
    Uniform grid header: API and pair enumeration (serial/parallel/pruned).
*/

#pragma once

#include "ThreadSystem.h"

#include <glm.hpp>
#include <vector>

//Uniform grid broadphase. Sparse reset via "touched" keeps per-frame clear O(active).
class UniformGrid
{
public:
    UniformGrid() = default;
    UniformGrid(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell);

    void resize(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell); //Rebuild grid dims and storage.
    void clear(int expectedCount);                                             //Sparse clear + opportunistic reserve.

    void insert(int objectId, const glm::vec3& position, float radius);        //Insert one element at position.

    //Enumerate potential pairs inside a cell and with its forward neighbors (no duplicates).
    template<typename Fn>
    void forEachPotentialPair(Fn&& fn) const
    {
        if (gridDims.x <= 0 || gridDims.y <= 0 || gridDims.z <= 0) return;

        const int activeCount = static_cast<int>(activeCellLinear.size());

        for (int idx = 0; idx < activeCount; ++idx)
        {
            const int linearCellId = activeCellLinear[idx];
            int cellX, cellY, cellZ;
            unpack(linearCellId, cellX, cellY, cellZ);

            //--LOOKUP-ACTIVE-BUCKET--
            const int activeBucketIndex = cellBucketLUT[linearCellId];
            if (activeBucketIndex < 0) continue;
            const auto& bucketA = cellBuckets[activeBucketIndex];
            //--LOOKUP-ACTIVE-BUCKET-END--

            const int countA = static_cast<int>(bucketA.size());

            for (int i = 0; i < countA; ++i)
            {
                for (int j = i + 1; j < countA; ++j)
                {
                    fn(bucketA[i], bucketA[j]); //Intra-cell pairs.
                }
            }

            for (int dx = 0; dx <= 1; ++dx)
            {
                for (int dy = -1; dy <= 1; ++dy)
                {
                    for (int dz = -1; dz <= 1; ++dz)
                    {
                        if (dx == 0)
                        {
                            if (dy < 0) continue;
                            if (dy == 0 && dz <= 0) continue; //Forward-only neighborhood to avoid duplicates.
                        }

                        const int neighborX = cellX + dx;
                        const int neighborY = cellY + dy;
                        const int neighborZ = cellZ + dz;

                        if (neighborX < 0 || neighborY < 0 || neighborZ < 0 ||
                            neighborX >= gridDims.x || neighborY >= gridDims.y || neighborZ >= gridDims.z)
                        {
                            continue;
                        }

                        const int neighborLinearCellId = index(neighborX, neighborY, neighborZ);
                        const int bucketIndex = cellBucketLUT[neighborLinearCellId];

                        if (bucketIndex < 0) continue;

                        const auto& bucketB = cellBuckets[bucketIndex];

                        for (int objectA : bucketA)
                        {
                            for (int objectB : bucketB)
                            {
                                fn(objectA, objectB); //Cross-cell pairs.
                            }
                        }
                    }
                }
            }
        }
    }

    //Same as above, but split across the thread pool.
    template<typename Fn>
    void forEachPotentialPairParallel(ThreadSystem& tasks, Fn&& fn) const
    {
        if (gridDims.x <= 0 || gridDims.y <= 0 || gridDims.z <= 0) return;

        const int activeCount = static_cast<int>(activeCellLinear.size());

        tasks.parallelFor(0, activeCount, 64, [&](int begin, int end, int)
        {
            for (int idx = begin; idx < end; ++idx)
            {
                const int linearCellId = activeCellLinear[idx];
                int cellX, cellY, cellZ;
                unpack(linearCellId, cellX, cellY, cellZ);

                const int activeBucketIndex = cellBucketLUT[linearCellId];
                if (activeBucketIndex < 0) continue;
                const auto& bucketA = cellBuckets[activeBucketIndex];

                const int countA = static_cast<int>(bucketA.size());

                for (int i = 0; i < countA; ++i)
                {
                    for (int j = i + 1; j < countA; ++j)
                    {
                        fn(bucketA[i], bucketA[j]);
                    }
                }

                for (int dx = 0; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        for (int dz = -1; dz <= 1; ++dz)
                        {
                            if (dx == 0)
                            {
                                if (dy < 0) continue;
                                if (dy == 0 && dz <= 0) continue;
                            }

                            const int neighborX = cellX + dx;
                            const int neighborY = cellY + dy;
                            const int neighborZ = cellZ + dz;

                            if (neighborX < 0 || neighborY < 0 || neighborZ < 0 ||
                                neighborX >= gridDims.x || neighborY >= gridDims.y || neighborZ >= gridDims.z)
                                continue;

                            const int neighborLinearCellId = index(neighborX, neighborY, neighborZ);
                            const int bucketIndex =
                                (neighborLinearCellId >= 0 && neighborLinearCellId < (int)cellBucketLUT.size())
                                ? cellBucketLUT[neighborLinearCellId] : -1;

                            if (bucketIndex < 0) continue;

                            const auto& bucketB = cellBuckets[bucketIndex];

                            for (int objectA : bucketA)
                            {
                                for (int objectB : bucketB)
                                {
                                    fn(objectA, objectB);
                                }
                            }
                        }
                    }
                }
            }
        });
    }

    //--PRUNED-PAIR-ENUMERATION--
    //Parallel pair enumeration with lightweight axis sweep pruning to reduce narrow-phase calls in dense cells.
    template<typename GetPos, typename GetRad, typename Fn>
    void forEachPotentialPairPrunedParallel(ThreadSystem& tasks, GetPos getPos, GetRad getRad, Fn&& fn) const
    {
        if (gridDims.x <= 0 || gridDims.y <= 0 || gridDims.z <= 0) return;

        const int activeCount = static_cast<int>(activeCellLinear.size());
        const int MIN_GRAIN = 16;

        tasks.parallelFor(0, activeCount, MIN_GRAIN, [&](int begin, int end, int)
        {
            thread_local std::vector<int> bucketASorted;
            thread_local std::vector<int> bucketBSorted;

            auto sweepIntra = [&](const std::vector<int>& bucket)
            {
                const int countInCell = (int)bucket.size();

                if (countInCell <= 64)
                {
                    for (int i = 0; i < countInCell; ++i)
                        for (int j = i + 1; j < countInCell; ++j)
                            fn(bucket[i], bucket[j]); //Small cells: brute-force is cheaper.
                }
                else
                {
                    bucketASorted.assign(bucket.begin(), bucket.end());
                    std::sort(bucketASorted.begin(), bucketASorted.end(),
                        [&](int idA, int idB) { return getPos(idA).x < getPos(idB).x; }); //Sort along X for a quick sweep.

                    for (int i = 0; i < countInCell; ++i)
                    {
                        const int objectA = bucketASorted[i];
                        const auto posA = getPos(objectA);
                        const float radA = getRad(objectA);

                        for (int j = i + 1; j < countInCell; ++j)
                        {
                            const int objectB = bucketASorted[j];
                            if (getPos(objectB).x - posA.x > (radA + getRad(objectB))) break; //Stop when too far on X.
                            fn(objectA, objectB);
                        }
                    }
                }
            };

            auto sweepCross = [&](const std::vector<int>& bucketA, const std::vector<int>& bucketB)
            {
                const int aCount = (int)bucketA.size(), bCount = (int)bucketB.size();

                if (aCount == 0 || bCount == 0) return;

                if (aCount > 64 && bCount > 64)
                {
                    bucketASorted.assign(bucketA.begin(), bucketA.end());
                    bucketBSorted.assign(bucketB.begin(), bucketB.end());
                    std::sort(bucketASorted.begin(), bucketASorted.end(),
                        [&](int idA, int idB) { return getPos(idA).x < getPos(idB).x; });
                    std::sort(bucketBSorted.begin(), bucketBSorted.end(),
                        [&](int idA, int idB) { return getPos(idA).x < getPos(idB).x; });

                    int i = 0, j = 0;
                    while (i < (int)bucketASorted.size() && j < (int)bucketBSorted.size())
                    {
                        const int objectA = bucketASorted[i]; const auto posA = getPos(objectA); const float radA = getRad(objectA);
                        const int objectB = bucketBSorted[j]; const auto posB = getPos(objectB); const float radB = getRad(objectB);

                        if (posA.x + radA < posB.x - radB) { ++i; continue; }
                        if (posB.x + radB < posA.x - radA) { ++j; continue; }

                        int jj = j;
                        while (jj < (int)bucketBSorted.size())
                        {
                            const int candidateB = bucketBSorted[jj];
                            if (getPos(candidateB).x - posA.x > (radA + getRad(candidateB))) break;
                            fn(objectA, candidateB);
                            ++jj;
                        }
                        ++i;
                    }
                }
                else
                {
                    for (int objectA : bucketA)
                    {
                        const auto posA = getPos(objectA);
                        const float radA = getRad(objectA);
                        for (int objectB : bucketB)
                        {
                            if (std::abs(getPos(objectB).x - posA.x) <= (radA + getRad(objectB)))
                                fn(objectA, objectB); //Small buckets: simple check is enough.
                        }
                    }
                }
            };

            for (int idx = begin; idx < end; ++idx)
            {
                const int linearCellId = activeCellLinear[idx];
                const int activeBucketIndex = cellBucketLUT[linearCellId];
                if (activeBucketIndex < 0) continue;

                int cellX, cellY, cellZ;
                unpack(linearCellId, cellX, cellY, cellZ);

                const auto& bucketA = cellBuckets[activeBucketIndex];

                sweepIntra(bucketA); //Intra-cell.

                for (int dx = 0; dx <= 1; ++dx)
                {
                    for (int dy = -1; dy <= 1; ++dy)
                    {
                        for (int dz = -1; dz <= 1; ++dz)
                        {
                            if (dx == 0)
                            {
                                if (dy < 0) continue;
                                if (dy == 0 && dz <= 0) continue;
                            }

                            const int neighborX = cellX + dx, neighborY = cellY + dy, neighborZ = cellZ + dz;
                            if (neighborX < 0 || neighborY < 0 || neighborZ < 0 ||
                                neighborX >= gridDims.x || neighborY >= gridDims.y || neighborZ >= gridDims.z)
                            {
                                continue;
                            }

                            const int neighborLinearCellId = index(neighborX, neighborY, neighborZ);
                            const int neighborBucketIndex = cellBucketLUT[neighborLinearCellId];

                            if (neighborBucketIndex < 0) continue;

                            const auto& bucketB = cellBuckets[neighborBucketIndex];
                            sweepCross(bucketA, bucketB); //Cross-cell.
                        }
                    }
                }
            }
        });
    }
    //--PRUNED-PAIR-ENUMERATION-END--

    const std::vector<int>& getNearWallList() const { return nearWallIds; } //Optional accessor.

private:
    glm::vec3 boxMin{ 0.0f }, boxMax{ 0.0f };
    glm::ivec3 gridDims{ 0, 0, 0 };
    float cellSize = 1.0f;
    float invCellSize = 1.0f;

    std::vector<int> activeCellLinear;           //Linear list of active cells (lids).
    std::vector<std::vector<int>> cellBuckets;   //Bucket storage reused across frames.
    std::vector<int> cellBucketLUT;              //Cell -> bucket index (or -1).
    std::vector<int> touchedCells;               //Cells to reset next frame.
    int usedBucketCount = 0;

    std::vector<int> nearWallIds;                //IDs near walls for wall-focused passes.

    inline int clampToRange(int v, int lo, int hi) const { return v < lo ? lo : (v > hi ? hi : v); }
    inline int index(int cellX, int cellY, int cellZ) const { return (cellZ * gridDims.y + cellY) * gridDims.x + cellX; }

    inline void unpack(int linearCellId, int& cellX, int& cellY, int& cellZ) const
    {
        cellX = linearCellId % gridDims.x;
        const int tmp = linearCellId / gridDims.x;
        cellY = tmp % gridDims.y;
        cellZ = tmp / gridDims.y;
    }
};