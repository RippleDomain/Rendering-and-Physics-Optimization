#pragma once

#include <glm.hpp>
#include <vector>

class UniformGrid
{
public:
    UniformGrid() = default;
    UniformGrid(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell);

    void resize(const glm::vec3& boxMin, const glm::vec3& boxMax, float cell);
    void clear(int expectedCount);

    void insert(int id, const glm::vec3& pos, float radius);

    template<typename Fn>
    void forEachPotentialPair(Fn&& fn) const
    {
        if (dims.x <= 0 || dims.y <= 0 || dims.z <= 0) return;

        const int nActive = static_cast<int>(activeLinear.size());
        for (int idx = 0; idx < nActive; ++idx)
        {
            const int lid = activeLinear[idx];
            int x, y, z;
            unpack(lid, x, y, z);

            //--LOOKUP-ACTIVE-BUCKET--
            const int biA = (lid >= 0 && lid < (int)lut.size()) ? lut[lid] : -1;
            if (biA < 0) continue;
            const auto& A = buckets[biA];
            //--LOOKUP-ACTIVE-BUCKET-END--

            const int asz = static_cast<int>(A.size());
            for (int i = 0; i < asz; ++i)
            {
                for (int j = i + 1; j < asz; ++j)
                {
                    fn(A[i], A[j]);
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

                        const int nx = x + dx;
                        const int ny = y + dy;
                        const int nz = z + dz;
                        if (nx < 0 || ny < 0 || nz < 0 || nx >= dims.x || ny >= dims.y || nz >= dims.z) continue;

                        const int nLid = index(nx, ny, nz);
                        const int bi = (nLid >= 0 && nLid < (int)lut.size()) ? lut[nLid] : -1;
                        if (bi < 0) continue;

                        const auto& B = buckets[bi];
                        for (int a : A)
                            for (int b : B)
                                fn(a, b);
                    }
                }
            }
        }
    }

    const std::vector<int>& nearWall() const { return nearWallList; }

private:
    glm::vec3 bmin{ 0.0f }, bmax{ 0.0f };
    glm::ivec3 dims{ 0, 0, 0 };
    float cellSize = 1.0f;

    std::vector<int> activeLinear;
    std::vector<std::vector<int>> buckets;
    std::vector<int> lut;
    std::vector<int> touched;
    int usedBuckets = 0;

    std::vector<int> nearWallList;

    inline int clampToRange(int v, int lo, int hi) const { return v < lo ? lo : (v > hi ? hi : v); }
    inline int index(int ix, int iy, int iz) const { return (iz * dims.y + iy) * dims.x + ix; }
    inline void unpack(int lid, int& ix, int& iy, int& iz) const
    {
        ix = lid % dims.x;
        const int tmp = lid / dims.x;
        iy = tmp % dims.y;
        iz = tmp / dims.y;
    }
};