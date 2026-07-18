# Anland：Termux 用户指南

[English](user-guide.md) | **中文**

---

本文将逐步介绍 [Anland: Termux](https://github.com/lfdevs/anland-termux) 的下载、安装和使用方法。

## 前提

在安装 Anland: Termux 前，请先检查你现在安装的 Termux App 是否来自 [GitHub 的官方 Releases](https://github.com/termux/termux-app/releases)**（而非 F-Droid 或 Google Play）**。本项目仅支持与来自 GitHub 官方 Releases 的 Termux App 一同工作。如果你想将 Termux 迁移到 GitHub 版本，可以参考官方的备份与恢复指南：https://wiki.termux.com/wiki/Backing_up_Termux

```sh
# 在 Termux 运行这条命令，输出应为 'GITHUB'
echo $TERMUX_APP__APK_RELEASE
```

## 下载

在[最新的 Release 说明](https://github.com/lfdevs/anland-termux/releases/latest) 中，我们重点关注“文件列表”一节。以下是一个示例：

| Item | Filename |
| :---: | --- |
| Android Display App | `AnlandTermux-5.13.0.apk` |
| Termux Daemon | `anland_5.11.0-1_aarch64.deb` |

| | KWin | XWayland |
| :---: | --- | --- |
| Termux Native | `kwin-anland_6.7.2_aarch64.deb` | `xwayland_24.1.12-2_aarch64.deb` |
| Ubuntu 26.04 LTS | `kwin_anland-5.8-4_6.6.4-0ubuntu92.zip` | `xwayland_24.1.10-91_arm64.deb` |
| Debian 13 | `kwin_anland-5.8-debian-4_6.3.6-92.zip` | `xwayland_24.1.6-91_arm64.deb` |

“Android Display App”和“Termux Daemon”是必需的。“KWin”和“XWayland”的版本请根据你的实际运行环境来进行选择。

例如，在 Debian 13 的 PRoot 容器中运行 Anland: Termux，需要下载 `AnlandTermux-5.13.0.apk`、`anland_5.11.0-1_aarch64.deb`、`kwin_anland-5.8-debian-4_6.3.6-92.zip` 和 `xwayland_24.1.6-91_arm64.deb` 四个文件。

## 安装

1. 在 Android 安装显示应用，如 `AnlandTermux-5.13.0.apk`。

   安装完成后，可以**长按应用图标**，进入设置界面。

2. 在 Termux 安装守护程序，如 `anland_5.11.0-1_aarch64.deb`。

   ```sh
   pkg reinstall ./anland_5.11.0-1_aarch64.deb
   ```

> [!TIP]
> 如果你接下来打算在 [PRoot-Distro](https://github.com/termux/proot-distro) 容器中使用 Anland，则可以直接使用本项目构建的系统镜像。安装方法如下：
>
> ```sh
> pkg install proot-distro
> # For Debian 13:
> proot-distro install ghcr.io/lfdevs/debian:trixie-anland-plasma --name debian-anland
> # For Ubuntu 26.04 LTS:
> proot-distro install ghcr.io/lfdevs/ubuntu:resolute-anland-plasma --name ubuntu-anland
> ```
>
> 使用方法如下。它将会启动 KDE Plasma 桌面，然后请切换到 Android 的“Anland Termux”应用。
>
> ```sh
> killall anland > /dev/null 2>&1
> anland > /dev/null 2>&1 &
> # For Debian 13:
> proot-distro login debian-anland --shared-tmp -- bash -c "startplasma-anland"
> # For Ubuntu 26.04 LTS:
> proot-distro login ubuntu-anland --shared-tmp -- bash -c "startplasma-anland"
> ```

3. 在实际运行环境中完成 KDE Plasma 桌面的安装后，使用软件包管理器安装 KWin 和 XWayland。**如果是 `.zip` 格式的压缩包，则需要先解压才能得到实际的安装包。**

   比如在 Termux Native 中：

   ```sh
   pkg reinstall ./kwin-anland_6.7.2_aarch64.deb ./xwayland_24.1.12-2_aarch64.deb
   ```

   又如在 Debian 13 容器中：

   ```sh
   sudo apt reinstall ./xwayland_24.1.6-91_arm64.deb
   unzip kwin_anland-5.8-debian-4_6.3.6-92.zip -d kwin-debs-install/
   sudo apt reinstall kwin-debs-install/*.deb
   rm -rf kwin-debs-install/
   ```

4. 在实际运行环境中安装 Freedreno (KGSL) 驱动。

   对于 Termux Native，请按照该页面的说明进行安装：https://github.com/lfdevs/termux-packages/releases/tag/freedreno-26.2.0-devel-20260709

   对于 Linux 容器，请按照该页面的说明进行安装：https://github.com/lfdevs/mesa-for-android-container/releases/latest
  
5. 锁定 KWin、XWayland 和 Mesa 软件包的版本，避免其受到更新的影响。

   比如在 Termux Native 中：

   ```sh
   apt-mark hold xwayland mesa mesa-vulkan-icd-freedreno
   ```

   又如在 Debian 13 或 Ubuntu 26.04 LTS 容器中：

   ```sh
   sudo apt-mark hold xwayland kwin-common kwin-data kwin-wayland libkwin6 libegl-mesa0 libgbm1 libgl1-mesa-dri libglx-mesa0 mesa-libgallium mesa-vulkan-drivers
   ```

## 使用

1. 在 Termux 启动守护程序：

   ```sh
   killall anland > /dev/null 2>&1
   anland > /dev/null 2>&1 &
   ```

2. 如果实际运行环境是 Linux 容器的话，需要将 Termux 的 `$TMPDIR` 绑定挂载到容器内部的 `/tmp`。

   比如 PRoot-Distro 容器在登录时需添加 `--shared-tmp` 选项：

   ```sh
   proot-distro login debian --shared-tmp
   ```

3. 进入实际运行环境后，下载并运行该一键脚本：[startplasma-anland.sh](../scripts/startplasma-anland.sh)

> [!TIP]
> 为了使音频服务正常运行，执行该脚本前请先确认 PipeWire 已安装。如 Termux 中的 `pipewire` 包，Debian/Ubuntu 中的 `pipewire-audio` 包。

   ```sh
   curl -LO https://github.com/lfdevs/anland-termux/raw/refs/heads/termux/scripts/startplasma-anland.sh
   chmod +x ./startplasma-anland.sh
   ./startplasma-anland.sh
   ```

4. 切换到 Android 的“Anland Termux”应用，开始享受 Wayland 桌面。

> [!TIP]
> 如果使用一键脚本无法进入桌面或者桌面会话容易崩溃的话，则可以根据实际运行环境使用以下的命令手动启动桌面会话。**请留意命令中的注释，根据实际情况进行选择。** 目前以下命令的限制是无法启动 PipeWire 音频服务，还需要在“Anland Termux”应用的设置里关闭麦克风和摄像头的转发。
>
> * 在 PRoot / Chroot / LXC 容器内：
>
>   ```sh
>   #!/bin/bash
>   sudo chmod -R 777 /tmp/anland
>   killall plasmashell > /dev/null 2>&1; killall kwin_wayland > /dev/null 2>&1; killall startplasma > /dev/null 2>&1;
>   unset DISPLAY
>   export QT_QPA_PLATFORM=wayland XDG_CURRENT_DESKTOP=KDE XDG_SESSION_DESKTOP=KDE
>   export ANLAND_SOCKET=/tmp/anland/display_daemon.sock ANLAND=1
>   
>   # 对于 PRoot 容器：
>   export ANLAND_NO_DRM_DEVICE=1 EGL_PLATFORM=surfaceless
>   
>   # 对于 Chroot/LXC 容器：
>   export ANLAND_DRM_DEVICE=/dev/dri/renderD128
>   
>   # 为搭载 Adreno GPU 的设备启用 Freedreno (KGSL) 驱动
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
>   # 如果 startplasma-wayland 不能正常进入桌面（尤其在不是 Adreno GPU 的设备上），则可以尝试 plasmashell
>   dbus-run-session -- bash -lc '
>       kwin_wayland plasmashell > /dev/null 2>&1 &
>       sleep 2
>       konsole > /dev/null 2>&1
>       wait
>   '
>   ```
>
> * 在 Termux 原生环境：
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
>   # 为搭载 Adreno GPU 的设备启用 Freedreno (KGSL) 驱动
>   export MESA_LOADER_DRIVER_OVERRIDE=kgsl TURNIP_KMD=kgsl GALLIUM_DRIVER=freedreno FD_FORCE_KGSL=1 XWAYLAND_FORCE_KGSL_SURFACELESS=1
>   
>   rm -f $XDG_RUNTIME_DIR/wayland-* > /dev/null 2>&1
>   dbus-run-session startplasma-wayland > /dev/null 2>&1
>   ```
