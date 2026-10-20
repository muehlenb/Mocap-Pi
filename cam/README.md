# Mocap-Pi: Camera Application
This directory contains the camera application of the **Mocap-Pi** framework that is intended to run on the Raspberry Pi Zero 2 W with the Raspberry Camera Module 3 NoIR.

![Camera Software Class Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/initial_release/img/software-camera.png)

For an overview of the project, including the **camera software/firmware**, **additional utilities**, and related publications, please refer to the [repository root](https://github.com/muehlenb/Mocap-Pi).

## Dependencies
To build the C++ project, you require a **C++ compiler**, at least **CMake 3.12**, and the following dependencies:

- **Boost:** Network communication
- **OpenMP:** Parallel execution in for loops. 

On the Raspberry Pi OS Lite, the required dependencies can be installed using:
```bash
sudo apt install libboost-dev
```

## Pre-Build Image
To allow a fast start, we provide a ready-to-run image that can be put onto an MicroSDHC card, based on the Raspberry Pi OS Lite system.

- [Download Mocap-Pi Image](https://github.com/muehlenb/Mocap-Pi/releases)

This image is configured to start the camera application on startup.
Please note that in this image, the camera expects the following network configuration:
- **SSID:** `MocapPiWifi`
- **Password:** `1100407111719631`

**Note:** This configuration can be used for initial testing, but you should replace the default SSID and password with your own. To do so, flash the image onto a microSDHC card and boot a Raspberry Pi Zero 2 W connected to a monitor and keyboard. Adjust the Wi-Fi settings to match your network, then create a new image from the modified card and flash it onto the microSDHC cards of the remaining Raspberry Pi Zero 2 W devices. 

