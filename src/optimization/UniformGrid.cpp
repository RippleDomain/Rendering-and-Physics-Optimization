#include "UniformGrid.h"

#include <algorithm>

UniformGrid::UniformGrid(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell)
{
    resize(boxMin, boxMax, cell);
}

void UniformGrid::resize(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell)
{
    bmin = boxMin;
    bmax = boxMax;
    cellSize = (cell > 1e-6f) ? cell : 1.0f;
    invCell = 1.0f / cellSize;

    const glm::vec3 extent = bmax - bmin;
    dims.x = std::max(1, static_cast<int>(glm::ceil(extent.x / cellSize)));
    dims.y = std::max(1, static_cast<int>(glm::ceil(extent.y / cellSize)));
    dims.z = std::max(1, static_cast<int>(glm::ceil(extent.z / cellSize)));

    const int nCells = dims.x * dims.y * dims.z;
    lut.assign(nCells, -1);
    touched.clear();

    activeLinear.clear();
    buckets.clear();
    usedBuckets = 0;

    nearWallList.clear();
}

void UniformGrid::clear(int expectedCount)
{
    //--SPARSE-LUT-RESET--
    for (int lid : touched) lut[lid] = -1;
    touched.clear();
    //--SPARSE-LUT-RESET-END--

    activeLinear.clear();
    nearWallList.clear();

    usedBuckets = 0;

    //--PER-FRAME-PREALLOC--
    if (expectedCount > 0)
    {
        const int cap = std::min(expectedCount, dims.x * dims.y * dims.z);
        if ((int)buckets.size() < cap) buckets.resize(cap);
        if ((int)activeLinear.capacity() < cap) activeLinear.reserve(cap);
        if ((int)touched.capacity() < cap) touched.reserve(cap);
        const int nearGuess = std::max(32, expectedCount / 8);
        if ((int)nearWallList.capacity() < nearGuess) nearWallList.reserve(nearGuess);
    }
    //--PER-FRAME-PREALLOC-END--
}

void UniformGrid::insert(int id, const glm::vec3& pos, float radius)
{
    //--INDEX-COMPUTE--
    const glm::vec3 rel = (pos - bmin) * invCell;
    int ix = static_cast<int>(glm::floor(rel.x));
    int iy = static_cast<int>(glm::floor(rel.y));
    int iz = static_cast<int>(glm::floor(rel.z));

    ix = clampToRange(ix, 0, dims.x - 1);
    iy = clampToRange(iy, 0, dims.y - 1);
    iz = clampToRange(iz, 0, dims.z - 1);

    const int lid = index(ix, iy, iz);
    //--INDEX-COMPUTE-END--

    int bi = lut[lid];
    if (bi < 0)
    {
        bi = usedBuckets++;

        //--ENSURE-BUCKET-EXISTS--
        if (bi >= (int)buckets.size())
        {
            buckets.resize(bi + 1);
        }
        auto& bucket = buckets[bi];
        bucket.clear();
        if ((int)bucket.capacity() < 8) bucket.reserve(8);
        //--ENSURE-BUCKET-EXISTS-END--

        lut[lid] = bi;
        touched.push_back(lid);
        activeLinear.push_back(lid);
    }

    buckets[bi].push_back(id);

    //--NEAR-WALL-TRACK--
    const bool nearX = (pos.x - radius <= bmin.x) || (pos.x + radius >= bmax.x);
    const bool nearY = (pos.y - radius <= bmin.y) || (pos.y + radius >= bmax.y);
    const bool nearZ = (pos.z - radius <= bmin.z) || (pos.z + radius >= bmax.z);
    if (nearX || nearY || nearZ) nearWallList.push_back(id);
    //--NEAR-WALL-TRACK-END--
}