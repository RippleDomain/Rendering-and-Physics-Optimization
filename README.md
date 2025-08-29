# Real-Time CPU/GPU Optimization in C++
A high-performance C++ project focused on rendering and simulating **50,000+ dynamic objects** at interactive framerates using a stack of modern and practical optimization techniques. This project demonstrates intelligent CPU/GPU collaboration, pushing the limits of a crowded, fully dynamic scene using techniques that are simple, composable, and portable.

## 🎥 Demo Video

Click the image below to watch the simulation:

<a href="https://youtu.be/X44JvG213eA" target="_blank">
  <img width="1919" height="1079" alt="Ekran görüntüsü 2025-08-29 173024" src="https://github.com/user-attachments/assets/38bb15d6-ab1e-4922-b73a-eff6b9ee5479" />
</a>

---

## Overview

This project investigates how far efficiency-first design in C++ can take dynamic object simulation and rendering in real time. It highlights a blend of **data-oriented design**, **multithreading**, and **GPU-level optimizations**, all structured for determinism and clarity.

Key characteristics:
- **50K+ moving objects**, with positions and attributes updated every frame.
- **Efficient partitioning** for physics and rendering using uniform spatial grids.
- **Modern GPU pipelines** with instancing and compact draw submissions.
- **Multithreaded CPU-side simulation** with minimal synchronization overhead.
- **Frustum culling** and memory-locality optimizations.

---

## Techniques Used

### 1. GPU Instancing
- **One mesh, many instances.**
- Transforms and per-instance attributes are streamed using compact GPU buffers.
- This drastically reduces CPU overhead and draw call count.

### 2. Uniform Grid (Spatial Hashing)
- Lightweight 3D grid structure partitions the world.
- Used for broad-phase collision detection and efficient view-frustum culling.
- Avoids unnecessary work by focusing on relevant spatial regions.

### 3. Multithreading
- Simulation, grid updates, and culling are distributed across multiple threads.
- Designed for minimal contention and optimal parallel throughput.
- Uses job-based patterns with lock-free or low-contention synchronization.

### 4. View-Frustum Culling
- Fast AABB and bounding-sphere plane tests.
- Eliminates off-screen instances before draw calls.
- Reduces vertex shader workload and pixel overdraw.

### 5. Frame Pacing & Cache Optimization
- View/projection matrices are cached per frame.
- Transient memory allocations are avoided.
- Hot-path data structures use **Structure of Arrays (SoA)** for cache efficiency.
