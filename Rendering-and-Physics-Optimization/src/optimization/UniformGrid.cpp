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

void UniformGrid::clear(int)
{
    for (int lid : touched) lut[lid] = -1;
    touched.clear();

    activeLinear.clear();
    nearWallList.clear();

    usedBuckets = 0;
}

void UniformGrid::insert(int id, const glm::vec3& pos, float radius)
{
    const glm::vec3 rel = (pos - bmin) / cellSize;
    int ix = static_cast<int>(glm::floor(rel.x));
    int iy = static_cast<int>(glm::floor(rel.y));
    int iz = static_cast<int>(glm::floor(rel.z));

    ix = clampToRange(ix, 0, dims.x - 1);
    iy = clampToRange(iy, 0, dims.y - 1);
    iz = clampToRange(iz, 0, dims.z - 1);

    const int lid = index(ix, iy, iz);

    int bi = lut[lid];
    if (bi < 0)
    {
        bi = usedBuckets++;

        if (bi >= (int)buckets.size())
        {
            buckets.emplace_back();
        }

        buckets[bi].clear();
        buckets[bi].reserve(8);

        lut[lid] = bi;
        touched.push_back(lid);
        activeLinear.push_back(lid);
    }

    buckets[bi].push_back(id);

    const bool nearX = (pos.x - radius <= bmin.x) || (pos.x + radius >= bmax.x);
    const bool nearY = (pos.y - radius <= bmin.y) || (pos.y + radius >= bmax.y);
    const bool nearZ = (pos.z - radius <= bmin.z) || (pos.z + radius >= bmax.z);

    if (nearX || nearY || nearZ)
    {
        nearWallList.push_back(id);
    }
}