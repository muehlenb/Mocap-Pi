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

The required dependencies can be installed using **vcpkg**: `vcpkg install opencv pcl ceres[suitesparse] nlohmann-json assimp boost-asio libssh`

This assumes that you configured vcpkg correctly for your platform (e.g., under Windows, set `VCPKG_DEFAULT_TRIPLET` to `x64-windows` in the enviroment variables).

## 3. Architecture

### How it works

`main.cpp` loads settings, initializes networking, creates the shared `DataModel`, starts the `ProcessorCollection`, and runs the `View` render loop. Processing runs in a separate thread from rendering.

![Architecture Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-architecture.png)

`DataModel` stores longer-lived objects, while `TickData` represents a single processing step. Processors typically implement the `Processor::process(TickData&)` interface.

### Project structure

```text
src/
├── main.cpp             # Application startup and main render loop
├── model/               # Shared data: cameras, markers, rigid bodies, skeletons
├── processors/          # Frame-by-frame tracking and calibration pipeline
│   ├── input/           # Camera data and frame timestamps
│   ├── calibration/     # Camera calibration
│   ├── marker/          # 3D triangulation and marker persistence
│   ├── rigid/           # Rigid-body detection
│   └── skeleton/        # Skeleton fitting
├── view/                # GUI and rendering
└── util/                # Networking, math, OpenGL, settings, helpers
```

The processors are structured as follows:

![Processor Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-processors.png)

### Where to start reading

Read `main.cpp` → `model/DataModel.h` → `processors/ProcessorCollection.cpp`, then follow individual processors in pipeline order. For UI behavior, start with `view/View.h`.
