# Mocap-Pi: Utils
## DummyMarkerSender

Streams recorded or synthetic marker data from multiple cameras to the main application over the network.

To use recorded data, place all camera recordings in one folder, named either `[ID].txt` or `camera_[ID].txt` (consistent across all files). IDs do not need to be sequential and typically correspond to the last octet of the camera's IPv4 address.

Start streaming with:

```bash
DummyMarkerSender --folder [PathToFolder]
```

**Note:** This application is intended to run on the same computer as the main application. To view the streamed data, select the loopback interface (`127.0.0.1`) in the main application's **Network** tab.

## UnrealDataCapturer

An **Unreal Engine 5.3** project for simulating and exporting motion capture data using configurable virtual cameras and marker trajectories. Cameras can be freely positioned and oriented, while markers can be attached to moving actors or skeletal bones. After capturing the markers in the virtual cameras, the exported data can be used by the **DummyMarkerSender** to play it in the Mocap-Pi main application. This enables rapid, low-cost prototyping and evaluation of different motion capture scenarios.

![Unreal Data Capturer](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/ue5_mocapdatacapturer.jpg)

### Camera Configuration

The main actor/class is **`MarkerDetectorCamera.h`**, with 10 camera instances already placed in the default startup level. Each camera requires a **unique ID** in its settings and should be positioned and oriented appropriately for the desired recording scenario.

### Scene Configuration

The startup level includes three animated elements:

- **Calibration Wand:** An animated wand for calibration scenarios.
- **Rigid Bodies:** Four animated rigid bodies.
- **Mixamo X-Bot:** An animated humanoid character from [Mixamo](https://www.mixamo.com/).

To select which elements are included in the simulation, **disable** `Actor Hidden in Game` for the desired objects and **enable** it for all others.

### Recording and Export

Recording starts automatically when the level is launched. Each active camera exports its marker data to a separate `[ID].txt` file in the `recordings` directory, where `[ID]` corresponds to the camera's unique identifier.

The generated `recordings` directory can be saved and used as the source folder for **`DummyMarkerSender`** to stream the recorded marker data into the Mocap-Pi main application.

**Note:** Before using other recordings, you must first record the calibration wand with your camera setup and perform camera calibration in the Mocap-Pi main application using the recorded data. This ensures that subsequent recordings can be processed correctly.
