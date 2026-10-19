# Mocap-Pi: Utils

## UnrealDataCapturer

An **Unreal Engine 5.3** project for simulating and exporting motion capture data using configurable virtual cameras and marker trajectories. Cameras can be freely positioned and oriented, while markers can be attached to moving actors or skeletal bones. After capturing the markers in the virtual cameras, the exported data can be used by the **DummyMarkerSender** to play it in the Mocap-Pi main application. This enables rapid, low-cost prototyping and evaluation of different motion capture scenarios.

![Unreal Data Capturer](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/ue5_mocapdatacapturer.jpg)

### Camera Configuration

The main component is **`MarkerDetectorCamera.h`**, with 10 camera instances already placed in the default startup level. Each camera requires a **unique ID** in its settings and should be positioned and oriented appropriately for the desired recording scenario.

### Scene Configuration

The startup level includes three animated elements:

- **Calibration Wand:** An animated wand for calibration scenarios.
- **Rigid Bodies:** Four animated rigid bodies.
- **Mixamo X-Bot:** An animated humanoid character from [Mixamo](https://www.mixamo.com/).

To select which elements are included in the simulation, **disable** `Actor Hidden in Game` for the desired objects and **enable** it for all others.

### Recording and Export

Recording starts automatically when the level is launched. Each active camera exports its marker data to a separate `[ID].txt` file in the `recordings` directory, where `[ID]` corresponds to the camera's unique identifier.

The generated `recordings` directory can be saved and used as the source folder for **`DummyMarkerSender`** to stream the recorded marker data into the Mocap-Pi main application.