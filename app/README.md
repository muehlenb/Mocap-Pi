# Mocap-Pi: Main Application
This directory contains the main application of the **Mocap-Pi** framework.

For an overview of the project, including the **camera software/firmware**, **additional utilities**, and related publications, please refer to the [repository root](https://github.com/muehlenb/Mocap-Pi).

## Prebuild Binaries

To allow a fast start, we provide prebuild binaries of the software for Windows:

- [Download Mocap-Pi Binaries (Windows, 64-Bit)](https://github.com/muehlenb/Mocap-Pi/releases)

For Linux, please follow the build information below.

## Dependencies
To build the C++ project, you require a **C++ compiler**, at least **CMake 3.12**, and the following dependencies:

- **Ceres:** Currently used for Bundle Adjustment (Intrinsic & extrinsic calibration of cameras). Requires to be compiled with SuiteSparse, via vcpkg use `vcpkg install ceres[suitesparse]`.
- **PCL:** Used for SVD based Transformation estimation (very rough extrinsic calibration guess).
- **Boost:** Network communication
- **OpenMP:** Parallel execution in for loops. 
- **nlohmann-json:** Used to save camera calibration.
- **Assimp:** Load Skeleton.
- **LibSSH:** For connection to Raspberry Pi Cameras (update, status, etc.).

The required dependencies can be installed using **vcpkg**: `vcpkg install opencv pcl ceres[suitesparse] nlohmann-json assimp boost-asio`

This assumes that you configured vcpkg correctly for your platform (e.g., under Windows, set `VCPKG_DEFAULT_TRIPLET` to `x64-windows` in the enviroment variables).

## 3. Architecture
**[TODO]: This part is not fully up-to-date anymore. Explain more precisely the Difference between DataModel (persistent over frames) and TickData (just used during processing a single host-frame)**

[TODO] Also explain how the initial TICK is triggered, which modules exists and how to add new modules.

The classes of this software project are generally structured in:
- **Model:** Classes that store information, e.g. the Camera calibration, 2D-Markers, 3D-Markers, Rigid Bodies and Skeletons.
- **Processors:** These are classes that uses some data from the model as input and writes new data to the model, e.g. the MarkerTriangulator reads 2D-Markers of multiple cameras and writes 3D-Markers in Worldspace as output.
- **View:** GUI & Rendering classes
- **Util:** Math classes & OpenGL helpers
