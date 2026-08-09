# Anland: Termux User Guide

**English** | [中文](user-guide_zh.md)

---

This guide walks you through downloading, installing, and using [Anland: Termux](https://github.com/lfdevs/anland-termux).

## Prerequisites

Anland: Termux provides two display APKs. Choose the one that matches the installed Termux app:

| Termux source | Display APK | Transport |
| --- | --- | --- |
| [Official GitHub releases](https://github.com/termux/termux-app/releases) | `AnlandTermux-<version>.apk` | Shared UID and direct Unix socket connection |
| [F-Droid](https://f-droid.org/packages/com.termux/) or variants such as ZeroTermux | `AnlandTermux-<version>-compatible.apk` | Termux-side socket plus Binder fd transfer |

The two APKs use the same application ID and versionCode, so they cannot be installed side by side. Uninstall the previous Anland Termux APK before switching transport variants.

```sh
# Run this command in Termux. Use the compatible APK when it reports F_DROID.
echo $TERMUX_APP__APK_RELEASE
```

## Download

In the [latest release notes](https://github.com/lfdevs/anland-termux/releases/latest), focus on the “File List” section. For example:

| Item | Filename |
| :---: | --- |
| Android Display App (Standard) | `AnlandTermux-5.13.3.apk` |
| Android Display App (Compatible) | `AnlandTermux-5.13.3-compatible.apk` |
| Termux Daemon | `anland_5.13.3_aarch64.deb` |

| | XWayland | KWin | Weston |
| :---: | --- | --- | --- |
| Termux Native | `xwayland_24.1.12-2_aarch64.deb` | `kwin-anland_6.7.2_aarch64.deb` | `weston_14.0.2-3_aarch64.deb` |
| Ubuntu 26.04 | `xwayland_24.1.10-91_arm64.deb` | `kwin_anland-5.8-4_6.6.4-0ubuntu92.zip` | `weston_anland-5.13-ubuntu-14.0.2-92.zip` |
| Debian 13 | `xwayland_24.1.6-91_arm64.deb` | `kwin_anland-5.8-debian-4_6.3.6-92.zip` | `weston_anland-5.13-debian-14.0.2-92.zip` |

The Android Display App and Termux Daemon are required. Choose the display APK according to the table above, then choose the XWayland, Weston, and KWin versions that match your runtime environment.

For example, to run Anland: Termux with KDE Plasma in a Debian 13 PRoot container using F-Droid Termux, download these four files: `AnlandTermux-5.13.3-compatible.apk`, `anland_5.13.3_aarch64.deb`, `xwayland_24.1.6-91_arm64.deb`, and `kwin_anland-5.8-debian-4_6.3.6-92.zip`.

Or, to run Anland: Termux with Weston in an Ubuntu 26.04 Chroot container using the GitHub Termux release, download these four files: `AnlandTermux-5.13.3.apk`, `anland_5.13.3_aarch64.deb`, `xwayland_24.1.10-91_arm64.deb`, and `weston_anland-5.13-ubuntu-14.0.2-92.zip`.

## Installation

1. Install the display app on Android: use `AnlandTermux-5.13.3.apk` with GitHub Termux, or `AnlandTermux-5.13.3-compatible.apk` with F-Droid Termux. After installation, **long-press the app icon** to open its settings interface.

2. Install the daemon in Termux, such as `anland_5.13.3_aarch64.deb`.

   ```sh
   pkg reinstall ./anland_5.13.3_aarch64.deb
   ```

> [!TIP]
> If you plan to use Anland in a [PRoot-Distro](https://github.com/termux/proot-distro) container, you can use the system images built by this project directly. Install one as follows:
>
> ```sh
> pkg install proot-distro
> # Debian 13 with KDE Plasma:
> proot-distro install ghcr.io/lfdevs/debian:trixie-anland-plasma --name debian-anland
> # Ubuntu 26.04 with KDE Plasma:
> proot-distro install ghcr.io/lfdevs/ubuntu:resolute-anland-plasma --name ubuntu-anland
> # Debian 13 with Weston:
> proot-distro install ghcr.io/lfdevs/debian:trixie-anland-weston --name debian-anland-weston
> # Ubuntu 26.04 with Weston:
> proot-distro install ghcr.io/lfdevs/ubuntu:resolute-anland-weston --name ubuntu-anland-weston
> ```
>
> To use it, run the following commands. This starts KDE Plasma or Weston. Then switch to the “Anland Termux” app on Android. The `ANLAND_WESTON_SCALE` environment variable in the commands sets Weston’s scaling factor; set it to an integer that suits your needs.
>
> ```sh
> # Start the daemon:
> killall anland > /dev/null 2>&1; anland > /dev/null 2>&1 &
> # The compatible APK also requires the Binder bridge:
> pkill -TERM -x anland-compatible; anland-compatible &
> # Debian 13 with KDE Plasma:
> proot-distro login debian-anland --shared-tmp -- bash -c "startplasma-anland"
> # Ubuntu 26.04 with KDE Plasma:
> proot-distro login ubuntu-anland --shared-tmp -- bash -c "startplasma-anland"
> # Debian 13 with Weston:
> proot-distro login debian-anland-weston --shared-tmp -- bash -c "ANLAND_WESTON_SCALE=2 startweston-anland"
> # Ubuntu 26.04 with Weston:
> proot-distro login ubuntu-anland-weston --shared-tmp -- bash -c "ANLAND_WESTON_SCALE=2 startweston-anland"
> ```

3. After installing KDE Plasma or Weston in your runtime environment, install this project’s XWayland through its package manager, then install this project’s KWin or Weston as needed. **If a file is a `.zip` archive, extract it first to obtain the actual installation packages.**

   For example, to install XWayland and KWin in Termux Native:

   ```sh
   pkg reinstall ./kwin-anland_6.7.2_aarch64.deb ./xwayland_24.1.12-2_aarch64.deb
   ```

   Or, to install XWayland and KWin in a Debian 13 container:

   ```sh
   sudo apt reinstall ./xwayland_24.1.6-91_arm64.deb
   unzip kwin_anland-5.8-debian-4_6.3.6-92.zip -d kwin-debs-install/
   sudo apt reinstall ./kwin-debs-install/*.deb
   rm -rf kwin-debs-install/
   ```

   Or, to install XWayland and Weston in an Ubuntu 26.04 container:

   ```sh
   sudo apt reinstall ./xwayland_24.1.10-91_arm64.deb
   unzip weston_anland-5.13-ubuntu-14.0.2-92.zip -d weston-debs-install/
   sudo apt reinstall ./weston-debs-install/*.deb
   rm -rf weston-debs-install/
   ```

> [!NOTE]
> When using KDE Plasma Wayland in Termux Native, you must also install the modified LayerShellQt package: <https://github.com/lfdevs/termux-packages/releases/tag/layer-shell-qt_6.7.3-1>
>
> Installing [Termux:API](https://github.com/termux/termux-api) is also recommended, as it improves the stability of KDE Plasma Wayland in Termux Native.

4. Install the Freedreno (KGSL) driver in your runtime environment.

   For Termux Native, follow the instructions on this page: <https://github.com/lfdevs/termux-packages/releases/tag/freedreno-26.2.0-devel-20260709>. If you use Weston, use this version instead: <https://github.com/lfdevs/termux-packages/releases/tag/freedreno-26.2.0-devel-20260709-weston>.

   For Linux containers, follow the instructions on this page: <https://github.com/lfdevs/mesa-for-android-container/releases/latest>

5. Hold the XWayland, KWin, Weston, and Mesa packages to prevent them from being affected by updates.

   For example, in Termux Native:

   ```sh
   apt-mark hold xwayland weston mesa mesa-vulkan-icd-freedreno
   ```

   Or in Debian 13 or Ubuntu 26.04 containers:

   ```sh
   sudo apt-mark hold xwayland kwin-common kwin-data kwin-wayland libkwin6 weston libweston-14-0 libegl-mesa0 libgbm1 libgl1-mesa-dri libglx-mesa0 mesa-libgallium mesa-vulkan-drivers
   ```

> [!TIP]
> The method above automatically removes the version lock whenever you manually install or update a related package. To make it permanent, add a package-manager configuration file.
>
> For example, in Termux Native, create `$PREFIX/etc/apt/preferences.d/hold-anland-package`:
>
> ```text
> Package: xwayland mesa mesa-vulkan-icd-freedreno weston layer-shell-qt
> Pin: release *
> Pin-Priority: -1
> ```
>
> Or, in a Debian 13 or Ubuntu 26.04 container, create `/etc/apt/preferences.d/hold-anland-package`:
>
> ```text
> Package: xwayland kwin-common kwin-data kwin-wayland libkwin6 libegl-mesa0 libgbm1 libgl1-mesa-dri libglx-mesa0 mesa-libgallium mesa-vulkan-drivers weston libweston-*
> Pin: release *
> Pin-Priority: -1
> ```

## Usage

1. Start the daemon in Termux:

   ```sh
   killall anland > /dev/null 2>&1; anland > /dev/null 2>&1 &
   # The compatible APK also requires the Binder bridge:
   pkill -TERM -x anland-compatible; anland-compatible &
   ```

   For the compatible APK, keep `anland-compatible` running in Termux while the display app and daemon are in use.

2. If your runtime environment is a Linux container, bind-mount Termux’s `$TMPDIR` to `/tmp` inside the container.

   For example, add the `--shared-tmp` option when logging into a PRoot-Distro container:

   ```sh
   proot-distro login debian --shared-tmp
   ```

3. After entering the runtime environment, download and run the following helper scripts.

> [!TIP]
> To ensure that audio services work correctly, make sure PipeWire is installed before running either script. For example, install the `pipewire` package in Termux or the `pipewire-audio` and `pipewire-libcamera` packages in Debian/Ubuntu.

   KDE Plasma: [startplasma-anland.sh](../scripts/startplasma-anland.sh)

   ```sh
   curl -LO https://github.com/lfdevs/anland-termux/raw/refs/heads/termux/scripts/startplasma-anland.sh
   chmod +x ./startplasma-anland.sh
   ./startplasma-anland.sh
   ```

   Weston: [startweston-anland.sh](../scripts/startweston-anland.sh)

   ```sh
   curl -LO https://github.com/lfdevs/anland-termux/raw/refs/heads/termux/scripts/startweston-anland.sh
   chmod +x ./startweston-anland.sh
   ./startweston-anland.sh
   ```

4. Switch to the “Anland Termux” app on Android and enjoy your Wayland desktop.

> [!TIP]
> If the helper script cannot enter the KDE Plasma desktop or the desktop session is unstable, you can manually start the desktop session with the following commands for your runtime environment. **Pay close attention to the comments in the commands and choose the appropriate options.** These commands currently cannot start the PipeWire audio service; you must also **disable microphone and camera forwarding** in the “Anland Termux” app’s settings.
>
> * In a PRoot / Chroot / LXC container:
>
>   ```sh
>   #!/bin/bash
>   sudo chmod -R 777 /tmp/anland
>   killall plasmashell > /dev/null 2>&1; killall kwin_wayland > /dev/null 2>&1; killall startplasma > /dev/null 2>&1;
>   unset DISPLAY
>   export QT_QPA_PLATFORM=wayland XDG_CURRENT_DESKTOP=KDE XDG_SESSION_DESKTOP=KDE
>   export ANLAND_SOCKET=/tmp/anland/display_daemon.sock ANLAND=1
>
>   # For PRoot container:
>   export ANLAND_NO_DRM_DEVICE=1 EGL_PLATFORM=surfaceless
>
>   # For Chroot/LXC container:
>   export ANLAND_DRM_DEVICE=/dev/dri/renderD128
>
>   # Enable Freedreno (KGSL) driver for devices with Adreno GPU
>   export MESA_LOADER_DRIVER_OVERRIDE=kgsl TURNIP_KMD=kgsl GALLIUM_DRIVER=freedreno FD_FORCE_KGSL=1 XWAYLAND_FORCE_KGSL_SURFACELESS=1
>
>   export XDG_RUNTIME_DIR=/run/user/$(id -u)
>   sudo mkdir -p /run/user/$(id -u)
>   sudo chown $(id -un):$(id -gn) /run/user/$(id -u)
>   chmod 700 /run/user/$(id -u)
>   rm -f $XDG_RUNTIME_DIR/wayland-* > /dev/null 2>&1
>   sudo mkdir -p /tmp/.X11-unix
>   sudo chmod 1777 /tmp/.X11-unix
>   dbus-run-session startplasma-wayland > /dev/null 2>&1
>
>   # If startplasma-wayland cannot enter the desktop normally (especially on devices without Adreno GPU), you can use plasmashell
>   dbus-run-session -- bash -lc '
>       kwin_wayland plasmashell > /dev/null 2>&1 &
>       sleep 2
>       konsole > /dev/null 2>&1
>       wait
>   '
>   ```
>
> * In the Termux native environment:
>
>   ```sh
>   #!/data/data/com.termux/files/usr/bin/bash
>   mkdir -p $TMPDIR/run
>   chown -R $(id -un):$(id -gn) $TMPDIR/run
>   chmod -R 700 $TMPDIR/run
>   mkdir -p $TMPDIR/.X11-unix
>   chmod 1777 $TMPDIR/.X11-unix
>   killall anland > /dev/null 2>&1
>   anland > /dev/null 2>&1 &
>   killall plasmashell > /dev/null 2>&1; killall kwin_wayland > /dev/null 2>&1; killall startplasma > /dev/null 2>&1;
>   unset DISPLAY
>   unset PULSE_SERVER
>   export XDG_RUNTIME_DIR=$TMPDIR/run
>   export QT_QPA_PLATFORM=wayland XDG_CURRENT_DESKTOP=KDE XDG_SESSION_DESKTOP=KDE
>   export ANLAND_SOCKET=$TMPDIR/anland/display_daemon.sock ANLAND=1 ANLAND_NO_DRM_DEVICE=1 EGL_PLATFORM=surfaceless
>
>   # Enable Freedreno (KGSL) driver for devices with Adreno GPU
>   export MESA_LOADER_DRIVER_OVERRIDE=kgsl TURNIP_KMD=kgsl GALLIUM_DRIVER=freedreno FD_FORCE_KGSL=1 XWAYLAND_FORCE_KGSL_SURFACELESS=1
>
>   rm -f $XDG_RUNTIME_DIR/wayland-* > /dev/null 2>&1
>   dbus-run-session startplasma-wayland > /dev/null 2>&1
>   ```
