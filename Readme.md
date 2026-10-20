**Development Status:** Mocap-Pi is currently being prepared for its initial public release. Source code, documentation, and setup instructions are expected to become fully available in the first half of November 2026.

# Mocap-Pi

**Open-Source Real-Time Motion Capture Using Wireless Raspberry Pi Cameras**

Mocap-Pi is an open-source optical motion capture system designed for **real-time 6DoF rigid-body tracking** using passive infrared (IR) markers and low-cost, wireless Raspberry Pi cameras.

The system is designed for applications such as **extended reality (XR), robotics, and precise real-time object tracking**, providing millimeter-level positional accuracy with affordable hardware.

## Key Features

| Feature | Specification |
|---|---|
| **Tracking** | Real-time 6DoF rigid-body tracking |
| **Position Error** | Approx. 1–5 mm¹ |
| **Orientation Error** | Approx. 0.3–2.0°¹ |
| **End-to-End Latency** | Approx. 50–100 ms² (photon-to-application) |
| **Camera Hardware** | Raspberry Pi Camera Module 3 NoIR |
| **Processing Hardware** | Raspberry Pi Zero 2 W |
| **Camera Frame Rate** | 60 FPS (up to 85 FPS with cooling) |
| **License** | MIT |

**¹ Tracking accuracy:** Based on a 12-camera setup with a capture area of approximately 1–4 m². For more details, see our publication (below). Tracking error depends on motion speed, capture area, and application conditions. Slower motion and smaller capture areas generally yield better results. At higher motion speeds and at the border of the capture volume, rigid body detection may become less reliable.

**² Latency:** Configurable depending on system settings. Typical processing stages include (1) ~21 ms: Camera capture and image buffer availability (2) ~12 ms: On-device IR marker segmentation (3) A few milliseconds: Wireless network transmission (4) ~16 ms: Temporal synchronization buffer at 60 Hz camera frequency. Actual end-to-end latency depends on network conditions, synchronization settings, and application processing.

## Repository Overview

![Overview over the software packages](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-general.png)

### `/app` - Main Application [(Readme.md)](/app)
The main application, running on a host computer with a graphical user interface (GUI). It provides network communication, temporal marker synchronization, 3D triangulation, and rigid-body matching and tracking.

The framework also includes a user-friendly camera calibration workflow with visual feedback, combining approximate initialization with a two-stage bundle adjustment procedure.

### `/cam` - Camera Application [(Readme.md)](/cam)
Raspberry Pi camera software responsible for image acquisition, on-device IR marker segmentation, and wireless transmission of detected markers to the main application.

### `/pub` -  Publication [(Readme.md)](/pub)
Supplementary materials and scripts for reproducing the measurements and evaluations presented in our publications, including MIG '26.

### `/utils` - Utility [(Readme.md)](/utils)
Additional utilities:

- **DummyMarkerSender:** Sends recorded or synthetic marker data from multiple (possibly virtual) cameras to the main application via the network interface.
- **UnrealDataCapturer:** An **Unreal Engine 5.3** project for simulating and exporting motion capture scenes with configurable camera setups and marker trajectories. Virtual cameras can be placed freely in the scene, while marker components can be attached to moving actors or skeletal bones. This enables rapid, low-cost prototyping and evaluation of different motion capture scenarios.

## Setup Instructions

### Camera Node

To build your own camera node, the following hardware components are required:

- Raspberry Pi Zero 2 W
- Raspberry Pi Camera Module 3 NoIR
- Raspberry Pi Zero Camera Flex Cable
- HIR 204/H0 850 nm infrared (IR) LEDs (e.g., 6 LEDs)
- 850 nm IR bandpass filter (e.g., available on Alibaba)
- Micro-USB power supply and microSDHC card
- Mounting screws and nuts (4x M2 × 10 mm, 4x M1.6 × 10 mm)
- Jumper wires (for IR LEDs)

#### 3D-Printed Enclosure

The enclosure used in our setup is available as STL files for 3D printing (**Back cover:** [bottom.stl](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/utils/stl-files/bottom.stl), **Front cover:** [top.stl](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/utils/stl-files/top.stl), **IR filter mount:** [filter.stl](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/utils/stl-files/filter.stl)):

![Camera Case STL Files](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/stl-files2.png)


**Note:** The IR filter mount is currently attached to the enclosure using adhesive. The Raspberry Pi is secured using four M2 × 10 mm screws and nuts, while the camera is mounted using four M1.6 × 10 mm screws and nuts. To prevent the nuts from loosening over time, self-locking nuts or a suitable threadlocker can be used. Although the outer enclosure includes threaded screw holes, additional screws were not required, as the housing components fit snugly together. Depending on the dimensions of your IR filter, the mount may need to be adjusted. Our setup uses an IR filter measuring approximately 8 × 8 mm.

Standard jumper wires can be used to connect the IR LEDs to the Raspberry Pi's 5V power supply (soldered). However, you should consider the usage of appropriate resistors depending on the IR LED specifications. 
For instructions on installing and configuring the camera node software, please refer to the [Camera Application README](/cam).


### Main Application
For instructions on setting up and running the system, please refer to the [Main Application README](/app).


## Current Limitations

Mocap-Pi currently focuses on **rigid-body tracking** and does not provide built-in skeletal or full-body tracking.
However, the framework can be extended with custom skeletal tracking, matching, and fitting algorithms. Native skeletal tracking support is planned for future development.

## Credits
The original project was privately initiated by Janis Rosskamp and Andre Muehlenbrock, but was recently integrated into the research context.


## Publications
- Andre Muehlenbrock, Janis Rosskamp, and Gabriel Zachmann. 2026. Mocap-Pi: An Open-Source Real-Time Motion Capture System Using Wireless Optical Cameras. In The 19th ACM SIGGRAPH Conference on Motion, Interaction, and Games (MIG ’26), December 11–13, 2026, Charleston, SC, USA. ACM, New
York, NY, USA, 7 pages. https://doi.org/10.1145/3828647.3857294
  - [Paper](https://cgvr.cs.uni-bremen.de/papers/mig2026/Mocap-Pi-Paper.pdf) | [Supplemental Material](https://cgvr.cs.uni-bremen.de/papers/mig2026/Mocap-Pi-Supplemental.pdf)
  - [Video](https://cgvr.cs.uni-bremen.de/papers/mig2026/Mocap-Pi-Video.mp4)

Please cite:

```
@inproceedings{Mocap-Pi-2026,
  author    = {Andre Muehlenbrock and Janis Rosskamp and Gabriel Zachmann},
  title     = {Mocap-Pi: An Open-Source Real-Time Motion Capture System Using Wireless Optical Cameras},
  booktitle = {Proceedings of the 19th ACM SIGGRAPH Conference on Motion, Interaction, and Games (MIG '26)},
  year      = {2026},
  month     = dec,
  address   = {Charleston, SC, USA},
  publisher = {Association for Computing Machinery},
  doi       = {10.1145/3828647.3857294},
  isbn      = {979-8-4007-2824-2},
  url       = {https://doi.org/10.1145/3828647.3857294}
}
```