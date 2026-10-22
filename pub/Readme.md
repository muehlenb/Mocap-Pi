# Mocap-Pi: ACM MIG '26 Recordings

This repository contains the recordings used for the Mocap-Pi evaluation at ACM MIG '26.

## Mocap-Pi Recordings

The recordings are located in `mig26-recordings/mocap-pi` and include two types of data for each of the 10 trials:

1. **`calib[ID]`**: Camera calibration recordings and a precomputed `calibration.json` file.
2. **`test[ID]`**: Motion recordings (slow movement, swimming, jumping, etc.) tracking three rigid bodies: left hand, right hand, and head.

All recordings can be replayed using **DummyDataSender**. For example:

```bash
DummyDataSender.exe --folder [root]/pub/mig26-recordings/mocap-pi/test1
```

### Calibration

Before replaying a test recording, either:
- Calibrate the cameras by streaming the corresponding `calib[ID]` recording.
- Copy the corresponding `calib[ID]/calibration.json` into the main application's executable directory (e.g., next to the `.exe` on Windows) to use our precomputed calibration.

**Important:** Calibration may vary slightly between trials. Always use the calibration matching the test recording's ID to avoid reduced tracking accuracy.

TODO: Add instructions for exporting rigid body poses.

## OptiTrack Recordings

The OptiTrack reference recordings contain the tracked rigid body data and are located in `mig26-recordings/optitrack`.
