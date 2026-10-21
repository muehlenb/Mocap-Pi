# Mocap-Pi: Main Application
This directory contains the main application of the **Mocap-Pi** framework.

For an overview of the project, including the **camera software/firmware**, **additional utilities**, and related publications, please refer to the [repository root](https://github.com/muehlenb/Mocap-Pi).

## Prebuild Binaries

To allow a fast start, we provide prebuild binaries of the software for Windows:

- [Download Mocap-Pi Binaries (Windows, 64-Bit)](https://github.com/muehlenb/Mocap-Pi/releases)

For Linux, please follow the build information below.

## Usage

### Calibration

Camera calibration is required for accurate **3D marker triangulation** and **rigid body tracking**. This also applies to synthetic recordings generated using the **UnrealDataCapturer**.

#### Calibration Wand

Calibration requires a wand equipped with three retroreflective markers at precisely known distances. By default, two calibration wand configurations are supported, but additional calibration wand definitions can be added at the top of the `CameraCalibrator.cpp`:

- **30 cm wand:** Marker distances of 10 cm and 20 cm.
- **25.1 cm wand:** Marker distances of 8.9 cm and 16.2 cm. *(This configuration was used for simultaneous calibration with OptiTrack in the evaluation publication)*

The calibration wand can be 3D-printed or assembled manually. However, the distances between the markers should be as accurate as possible, ideally within millimeter precision, as inaccuracies directly affect the calibration results.

#### Calibration Procedure

![Calibration Area](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/main-calibration-menu.png)

**1. Mask Unwanted Markers**

Before starting calibration, ensure that no unwanted markers or reflective objects are visible to the cameras. Reflective surfaces and rigid bodies should be covered or removed from the capture volume.

If some reflections cannot be eliminated (e.g., LEDs from other cameras appearing in the camera images), they can be excluded using the **Mask Areas** tab on the right-hand side:

1. Open the **Mask Areas** tab.
2. Click **Start Recording** while the calibration wand is outside the capture volume.
3. Wait a few seconds to capture unwanted reflections.
4. Click **Stop Recording** to apply the masks.

The recorded areas will be ignored during calibration.

**Important:** The calibration algorithm expects a maximum of three visible markers per camera, corresponding to the calibration wand. Frames containing additional detected markers are discarded.

**2. Collect Calibration Observations**

Select the appropriate **Calibration Wand** configuration and start the calibration recording.

Slowly move the wand throughout the entire capture volume, ensuring that its markers are visible to at least two cameras simultaneously. Try to cover as much of the capture volume as possible, including different positions, orientations, and depths.

Slower and smoother movements generally improve calibration accuracy by reducing motion-related measurement errors.

Valid calibration paths, consisting of marker observations that can be matched between at least two cameras, are visualized directly in the corresponding camera windows.

**3. Finalize Calibration**

![Calibration Paths Good](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/main-calibration-paths.jpg)

Once a sufficient number of corresponding marker observations has been collected across the cameras, click **Finalize!** to start the calibration process.

The system performs **Bundle Adjustment** to optimize the camera calibration parameters based on the collected observations.

Depending on the number of cameras and observations, this process may take anywhere from a few seconds to several minutes.

After completion:

- The calibration results and accuracy metrics are displayed.
- The optimized calibration parameters are automatically saved in the application's execution directory.
- Saved calibration parameters are automatically loaded the next time the application starts, eliminating the need to repeat calibration for every session.

**Note:** Recalibration is recommended whenever the physical position or orientation of any camera changes, as previously determined calibration parameters may no longer be accurate.




### Camera Settings

![Camera Window](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/camera-window.png)

The camera window provides several controls for adjusting marker detection and camera behavior.

- **Threshold:** Sets the brightness threshold used for marker segmentation. It should be high enough to prevent non-marker regions from being merged with actual markers. 

- **Exposure:** Controls the camera's exposure time. Keeping it low helps ensure that non-marker regions remain mostly black while minimizing motion blur. Short exposure times are generally preferred for reliable marker detection.

- **Lens Focus:** Adjusts the focus position of the Raspberry Pi Camera Module 3.

- **Raw Image (Checkbox):** Enables the transmission and display of raw camera images. This option should primarily be used for debugging, as transmitting images increases network traffic and may introduce additional latency. **Note:** Raw images are displayed immediately upon reception, whereas detected markers are processed with a delay. As a result, the displayed image and marker overlays may not be perfectly synchronized.


![Camera SSH Settings](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/camera-ssh-settings.png)

The Network Pane provides an overview of all camera nodes detected within the same subnet. To discover available cameras, the Mocap Framework scans SSH ports across the subnet and attempts to authenticate using the default credentials. Successfully authenticated camera nodes are listed in the Network Pane, where they can be remotely managed via SSH.

The following controls are available for each detected camera node:

- **Flash:** Updates the camera software on the selected node. The currently running application is stopped, the camera source code from the `/cam` directory in the current build folder is transferred to the node via SSH, and the application is rebuilt directly on the camera node. Once the build is complete, the updated application is started automatically.

- **Show Log:** Displays the error log of the camera application, allowing remote inspection of potential issues.

- **Start:** Starts the camera application on the selected camera node via SSH.

- **Stop:** Stops the running camera application on the selected node. SSH access and the Network Pane controls remain available, allowing the application to be restarted at any time.



## Dependencies
To build the C++ project on your own, you require a **C++ compiler**, at least **CMake 3.12**, and the following dependencies:

- **Ceres:** Currently used for Bundle Adjustment (Intrinsic & extrinsic calibration of cameras). Requires to be compiled with SuiteSparse, via vcpkg use `vcpkg install ceres[suitesparse]`.
- **PCL:** Used for SVD based Transformation estimation (very rough extrinsic calibration guess).
- **Boost:** Network communication
- **OpenMP:** Parallel execution in for loops. 
- **nlohmann-json:** Used to save camera calibration.
- **Assimp:** Load Skeleton.
- **LibSSH:** For connection to Raspberry Pi Cameras (update, status, etc.).

The required dependencies can be installed using **vcpkg**: `vcpkg install opencv pcl ceres[suitesparse] nlohmann-json assimp boost-asio libssh`

This assumes that you configured vcpkg correctly for your platform (e.g., under Windows, set `VCPKG_DEFAULT_TRIPLET` to `x64-windows` in the enviroment variables).

## Architecture

### How it works

`main.cpp` loads settings, initializes networking, creates the shared `DataModel`, starts the `ProcessorCollection`, and runs the `View` render loop. Processing runs in a separate thread from rendering.

![Architecture Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-architecture.png)

`DataModel` stores longer-lived objects, while `TickData` represents a single processing step. Processors typically implement the `Processor::process(TickData&)` interface.

The **processors** data flow can be visualized as follows:

![Processor Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-processors.png)


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

### Where to start reading

Read `main.cpp` → `model/DataModel.h` → `processors/ProcessorCollection.cpp`, then follow individual processors in pipeline order. For UI behavior, start with `view/View.h`.
