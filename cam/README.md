# Mocap-Pi: Camera Application

This directory contains the **Mocap-Pi** camera application, designed for the Raspberry Pi Zero 2 W with the Raspberry Pi Camera Module 3 NoIR.

![Camera Software Class Overview](https://raw.githubusercontent.com/muehlenb/Mocap-Pi/main/img/software-camera.png)

For the **main application**, **additional utilities**, and related publications, see the [repository root](https://github.com/muehlenb/Mocap-Pi).

## Dependencies

Building the C++ application requires a **C++ compiler**, **CMake ≥ 3.12**, and the following dependencies:

- **Boost:** Network communication
- **OpenMP:** Parallel loop execution

On Raspberry Pi OS Lite, install Boost using:

```bash
sudo apt install libboost-dev
```

## Pre-Built Image

A ready-to-use image based on **Raspberry Pi OS 12 Lite** is available for quick setup.

### Installation

1. Download the [Mocap-Pi image](https://github.com/muehlenb/Mocap-Pi/releases).
2. Flash it to a microSD card using [Raspberry Pi Imager](https://www.raspberrypi.com/software/).
3. Insert the card into the Raspberry Pi Zero 2 W with the Camera Module 3 NoIR connected.
4. Power on the Raspberry Pi.

The camera application starts automatically on boot. The image uses the following default Wi-Fi credentials:

- **SSID:** `MocapPi`
- **Password:** `1100407111719631`

*The Raspberry Pi Zero 2 W supports only 2.4 GHz Wi-Fi. Ensure your router or hotspot uses this band if connection issues occur.*

**Warning:** The default SSH username and password are `mocappi` and `mocappi`. Always use a private network to prevent unauthorized access. Changing these credentials will disable camera control via the main application.

### Updating Wi-Fi Settings

For initial testing, you can use the default credentials with a router or a Windows Mobile Hotspot. To configure your own network, access the Raspberry Pi via SSH or a connected keyboard and run:

```bash
sudo raspi-config
```

Select the first menu option twice, then enter your Wi-Fi credentials.

To create a reusable image with the updated settings, connect a USB drive or use a network folder and run:

```bash
sudo image-backup
```

Specify the destination path for the image backup. The resulting image can be flashed to other microSD cards using **Raspberry Pi Imager**, preserving your custom Wi-Fi settings.