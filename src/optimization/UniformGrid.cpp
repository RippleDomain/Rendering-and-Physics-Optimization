/*
    Uniform grid implementation: sparse reset, insert, and active cell tracking.
*/

#include "UniformGrid.h"

#include <algorithm>

UniformGrid::UniformGrid(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell)
{
    resize(boxMin, boxMax, cell);
}

void UniformGrid::resize(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell)
{
    this->boxMin = boxMin;
    this->boxMax = boxMax;
    cellSize = (cell > 1e-6f) ? cell : 1.0f; //Defensive clamp to avoid div-by-zero.
    invCellSize = 1.0f / cellSize;

    const glm::vec3 extent = this->boxMax - this->boxMin;
    gridDims.x = std::max(1, static_cast<int>(glm::ceil(extent.x / cellSize)));
    gridDims.y = std::max(1, static_cast<int>(glm::ceil(extent.y / cellSize)));
    gridDims.z = std::max(1, static_cast<int>(glm::ceil(extent.z / cellSize)));

    const int totalCells = gridDims.x * gridDims.y * gridDims.z;
    cellBucketLUT.assign(totalCells, -1); //-1 marks empty.
    touchedCells.clear();                 //Touched list is empty after full resize.
    activeCellLinear.clear();             //Linear list of active cell IDs (for fast iteration).
    cellBuckets.clear();                  //Buckets will be created on demand.
    usedBucketCount = 0;
    nearWallIds.clear();                  //Reset the auxiliary near-wall list.
}

void UniformGrid::clear(int expectedCount)
{
    //--SPARSE-LUT-RESET--
    for (int linearCellId : touchedCells) cellBucketLUT[linearCellId] = -1; //Only clear cells we touched last frame.
    touchedCells.clear();
    //--SPARSE-LUT-RESET-END--

    activeCellLinear.clear();
    nearWallIds.clear();

    usedBucketCount = 0;

    //--PER-FRAME-PREALLOC--
    if (expectedCount > 0)
    {
        const int capacityEstimate = std::min(expectedCount, gridDims.x * gridDims.y * gridDims.z);

        if ((int)cellBuckets.size() < capacityEstimate) cellBuckets.resize(capacityEstimate); //Grow backing bucket store if needed.
        if ((int)activeCellLinear.capacity() < capacityEstimate) activeCellLinear.reserve(capacityEstimate);
        if ((int)touchedCells.capacity() < capacityEstimate) touchedCells.reserve(capacityEstimate);

        const int nearWallReserve = std::max(32, expectedCount / 8);

        if ((int)nearWallIds.capacity() < nearWallReserve) nearWallIds.reserve(nearWallReserve);
    }
    //--PER-FRAME-PREALLOC-END--
}

void UniformGrid::insert(int objectId, const glm::vec3& position, float radius)
{
    //--INDEX-COMPUTE--
    const glm::vec3 relative = (position - boxMin) * invCellSize; //Map to grid coordinates.
    int cellX = static_cast<int>(glm::floor(relative.x));
    int cellY = static_cast<int>(glm::floor(relative.y));
    int cellZ = static_cast<int>(glm::floor(relative.z));

    cellX = clampToRange(cellX, 0, gridDims.x - 1);
    cellY = clampToRange(cellY, 0, gridDims.y - 1);
    cellZ = clampToRange(cellZ, 0, gridDims.z - 1);

    const int linearCellId = index(cellX, cellY, cellZ); //Linear cell index.
    //--INDEX-COMPUTE-END--

    int bucketIndex = cellBucketLUT[linearCellId];
    if (bucketIndex < 0)
    {
        bucketIndex = usedBucketCount++;

        //--ENSURE-BUCKET-EXISTS--
        if (bucketIndex >= (int)cellBuckets.size())
        {
            cellBuckets.resize(bucketIndex + 1);
        }
        auto& bucket = cellBuckets[bucketIndex];
        bucket.clear();                             //We reuse bucket storage across frames.

        if ((int)bucket.capacity() < 8) bucket.reserve(8);
        //--ENSURE-BUCKET-EXISTS-END--

        cellBucketLUT[linearCellId] = bucketIndex;  //Map cell -> bucket index.
        touchedCells.push_back(linearCellId);       //Track to reset next frame.
        activeCellLinear.push_back(linearCellId);   //Add to active cell list.
    }

    cellBuckets[bucketIndex].push_back(objectId);   //Store object index in this cell.

    //--NEAR-WALL-TRACK--
    const bool nearX = (position.x - radius <= boxMin.x) || (position.x + radius >= boxMax.x);
    const bool nearY = (position.y - radius <= boxMin.y) || (position.y + radius >= boxMax.y);
    const bool nearZ = (position.z - radius <= boxMin.z) || (position.z + radius >= boxMax.z);
    if (nearX || nearY || nearZ) nearWallIds.push_back(objectId); //Optional list for wall-optimized passes.
    //--NEAR-WALL-TRACK-END--
}