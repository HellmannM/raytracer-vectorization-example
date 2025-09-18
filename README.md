# Raytracer (MPI + Visionaray)

This project is a parallel raytracer built on top of the [Visionaray](https://github.com/szellmann/visionaray) framework.  
It supports both **CPU (MPI + thread pool)** and **GPU acceleration (via CUDA + MPI)**.  

Each MPI rank renders a portion of the work, and results are reduced and combined into a final image.  
When compiled with CUDA support, each MPI rank can be mapped to a dedicated GPU.


## Features
- **CPU backend**: MPI ranks with Visionaray thread pool.  
- **GPU backend**: CUDA device selection per MPI rank (`cudaSetDevice`).  
- **MPI parallelism**: Image is rendered in parallel and combined on rank 0.  
- **PNG output**: Final image is written as PNG.  
- **Performance metrics**: Reports min/max/avg compute time across ranks.  


## Dependencies
- CMake ≥ 3.22  
- C++17 compiler (GCC)  
- [Boost](https://www.boost.org/) (filesystem, iostreams, system)  
- [libpng](http://www.libpng.org/pub/png/libpng.html)  
- [MPI](https://www.mpi-forum.org/)  
- [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads) *(optional, for GPU backend)*  
- Visionaray headers (included in `3rdparty/visionaray`)  

All required Visionaray header files are included and modified to reduce dependencies on GLEW/OpenGL.


## Build Instructions

### Clone repository
```bash
git clone --recursive git@github.com:HellmannM/raytracer-vectorization-example.git
cd raytracer-vectorization-example.git
git checkout CUDA_snowman
cd ..
mkdir build && cd build
module load GCC/13.2.0 OpenMPI/4.1.6-GCC-13.2.0 CMake/3.27.6-GCCcore-13.2.0 Boost/1.83.0-GCC-13.2.0 libpng/1.6.40-GCCcore-13.2.0 CUDA/12.6.0
```

### CPU build
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=OFF ..
make -j
```

### GPU build
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_CUDA=ON ..
make -j
```


## Command-line Options

The renderer supports the following command-line parameters:

| Option         | Description                          | Default                |
| -------------- | ------------------------------------ | ---------------------- |
| `-png`         | Output PNG filename                  | `rendered_snowman.png` |
| `-width`       | Image width                          | 512                    |
| `-height`      | Image height                         | 512                    |
| `-threads`     | Number of CPU threads per MPI rank   | 1                      |
| `-spp`         | Samples per pixel                    | 128                    |
| `-alloc_mode`  |Different allocation modes            | 3                      |

### Example usage
```bash
mpirun -n 4 ./build/raytracer -width=512 -height=512 -spp=128 -threads=1 -alloc_mod= 3 -png=snowman.png
```


## Scene Description
- The **scene objects** (snowmen, materials) are defined in:`common/model.h`
- The **rendering pipeline** (ray generation, camera setup,lighting and  sampling) is implemented in:`renderer.inl`

This separation allows you to **modify the scene geometry independently** of the rendering logic.

### Default Scene

By default, the scene includes:

- **Three snowmen**  
  - Each snowman is built from **stacked spheres** (representing the body, head, and features).  
- **Materials**
  - Default material is plastic
  - White color for snow.  
  - Black color for eyes and buttons.  
  - Orange color for noses.  
- **Camera**  
  - Positioned to view the snowmen from the front.  
  - Generates primary rays for each pixel.  
- **Lighting**  
  - A point light source illuminates the snowmen and casts soft shading.





