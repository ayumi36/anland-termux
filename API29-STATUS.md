# Android 10 / ARM64 backport — build blocked

**No APK was produced. This is modified source with a source audit and passing
host checks, not a successfully built or device-tested Android release.**

Base repository: https://github.com/lfdevs/anland-termux

Base commit: `23c77435032e7d9df00f40b90ecee04f1fc4ad9e`

Target: Android 10 / API 29, ARM64, Snapdragon 660 / Adreno 512.

The normal build was attempted with `bash tools/build-app.sh`. It exited with
status 1: `gradle must be available on PATH`. The environment has Java 17 but
no installed Gradle, Android SDK, Android NDK, or Android build tools. Fetching
Google's SDK command-line tools ended with `network approval was cancelled
before a decision was returned`. No reviewer reason was provided. Neither
Android compilation nor Android lint could run. This is a build-environment
blocker, not a conclusion that Android 10 graphics cannot support Anland.

The connected GitHub account has read-only access to upstream, and searching
its accessible repositories found no Anland-Termux fork. The prepared workflow
therefore could not be submitted to a remote build runner either.

## Changes

| File / area | Change |
| --- | --- |
| `app/build.gradle` | `minSdk` 30 → 29; version name `5.13.3-api29`. ARM64, application ID, versionCode 8, compile/target SDK 36, NDK, flavors, and signing retained. |
| `PlatformCompat.java` | API-gated window helpers; legacy immersive flags, display metrics, keyboard insets/visible-frame inference, and settings resize handling. API-30 framework calls are isolated in an API-gated nested class. |
| `MainActivity.java` | Replaced three `Activity.getDisplay()` calls; routed fullscreen, keyboard, and cutout selection through fallbacks; added legacy visible-frame observation and focus-time immersive restoration. |
| `SettingsActivity.java` | Replaced unguarded modern window metrics and edge-to-edge inset setup with compatible helpers. |
| `SystemIME.java` | Gated insets-controller access and supplied legacy visibility inference. Existing InputMethodManager show/hide and bounded retries retained. |
| `display_consumer.c` | Android shared control memory now uses `ASharedMemory_create`, sets close-on-exec, and does not call `ftruncate` on ashmem. The non-Android memfd path remains. Also copies packed resource arguments into aligned storage before callbacks. |
| `anw_hidden.h` | Checked native-window operation-table fallbacks when VNDK wrappers cannot be resolved; ARM64 ABI layout assertions and runtime magic/version validation. |
| `native_consumer.c` | Checks geometry, buffer-count query, and buffer-count setup errors, with specific diagnostics. |
| CMake / build tools | Native unguarded-API and implicit-function errors; no-undefined linking; standard build also invokes Android lint. |
| Tests / Actions | Host tests and an Android build workflow with actual APK signature, minSdk, package/shared UID, ARM64, and API-29 native-symbol checks. The Actions workflow has not run. |

## API and runtime audit

The audit covered the repository's Android Java/AIDL, manifests/resources,
Gradle/CMake configuration, JNI/native source, Termux daemon and socket protocol,
container startup scripts, and build workflows. This was source inspection;
Android lint, Android Java type checking, and NDK compilation remain outstanding.

| Dependency / behavior | API-29 finding and handling |
| --- | --- |
| `memfd_create` | **Actual native blocker.** Bionic declares it `__INTRODUCED_IN(30)`; Android 10 lacks the libc export. Replaced on Android with API-26 shared memory. No raw syscall or seccomp-policy workaround is used. |
| `Window.setDecorFitsSystemWindows`, `WindowInsetsController`, `WindowInsets.Type`, `getInsets(int)`, `isVisible(int)` | API 30. New helper calls modern methods only at API 30+, uses legacy inset/frame geometry, immersive UI flags, and InputMethodManager at API 29. |
| `Context.getDisplay` / inherited `Activity.getDisplay` | API 30. Uses the Activity's WindowManager display instead. Refresh-rate forwarding and rotation-aware pointer transformation remain. |
| `WindowManager.getMaximumWindowMetrics` | API 30. Uses `Display.getRealSize()` for physical resolution presets on API 29. |
| `LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS` | API 30 constant/behavior. Uses `SHORT_EDGES` on API 29; `NEVER` remains available. |
| `RoundedCorner`, `getRoundedCorner` | API 31, already guarded by upstream. No public equivalent for exact corner radii at API 29. Existing guard retained. |
| Native-window wrappers | `ANativeWindow_setBufferCount`, `query`, `dequeueBuffer`, `queueBuffer`, and `cancelBuffer` exist in Android 10 platform exports, but are VNDK rather than guaranteed public NDK APIs. Checked operation-table equivalents cover omitted wrapper exports. |
| Native buffer layout | Android 10 ARM64 layout matches the vendored fields: handle offset 96, buffer size 168, window table size 192. Static assertions and runtime window checks added. OEM differences are still a device-testing concern. |
| Public native-window / Surface JNI calls | Existing calls predate API 29. No frame-rate API introduced in API 30 is used; refresh rate is sent through the existing protocol. |
| Audio | Used AAudio APIs are available from API 26. No backend removed. |
| Camera / image reader | Used Camera2 NDK and image-reader calls are available by API 26; camera shared memory already used API 26 `ASharedMemory_create`. Runtime camera/microphone permissions retained. |
| libc / fd transport | Used eventfd, mmap, sockets, SCM_RIGHTS, pthread, clock, and fd operations exist on API 29. The discovered newer direct call was `memfd_create`. Actual final ELF imports still need the NDK-stub check. |
| Notifications / receivers | POST_NOTIFICATIONS request and exported-receiver overload are already API-33-gated. The permission declaration is harmless on API 29. Notification channels and immutable PendingIntents predate API 29. |
| Manifest / resources | Newer `enableOnBackInvokedCallback` and window compatibility property do not provide API-29 behavior; they are retained for newer Android versions. Exported flags, PiP, resizeability, shortcuts and resource styles are compatible with API 29. No new storage or all-files-access permission added. |
| PiP / input / accessibility | PiP and pointer capture predate API 29; `MotionEvent.getClassification()` and the AppOps call used here exist at API 29. Newer touchpad classifications may not be emitted by Android 10; ordinary touch/pointer paths remain. Accessibility still requires the user's system setting. |
| Clipboard | Android 10 restricts background clipboard reading. Existing focus-time synchronization is retained. |
| Native executable / Termux | Root helper remains in native-library packaging; standard app remains in shared UID/process `com.termux`. No daemon executable is moved into target-36 private writable app storage. |

## Graphics and 5.13.3 compatibility

No GPU renderer has been removed or replaced with software rendering. The Android
side still dequeues gralloc buffers and sends their dma-buf fds to the producer;
the producer's completed GPU fence is still queued to SurfaceFlinger. The new
shared-memory allocation is only the **four-byte selected-buffer index**, not
the rendered image storage. Its fd occupies the same handshake slot. Upstream
producer code maps this fd using `mmap(..., sizeof(uint32_t), PROT_READ,
MAP_SHARED, ...)`, without memfd-specific operations.

Both local `protocol.h` copies are byte-identical to tag `5.13.3`, with SHA-256:

`0fef0d9c04a7f7e46e5b340d953689aa7abfd17dbab4d08adb57e4a5f0278481`

The Termux daemon source is also unchanged from tag `5.13.3`. Thus the source
changes preserve the interface expected by `anland_5.13.3_aarch64.deb`; the actual
released binary has not been run in this environment.

KWin/Weston/Mutter Anland backends, patched XWayland, Mesa Freedreno/KGSL,
surfaceless EGL and `XWAYLAND_FORCE_KGSL_SURFACELESS` live on the Linux/Termux side.
Their startup configuration was inspected and retained. The APK does not bundle
or replace these Linux packages. Debian 13 still needs the matching patched
compositor and XWayland, along with its existing working KGSL Mesa build.

API-level inspection does not reveal a mandatory Android-11 GPU API after these
patches. It does **not** prove that an OEM's Adreno 512 gralloc buffers can be
imported by the particular Mesa build. `Accelerated: yes` confirms the existing
renderer, but not that additional cross-process buffer-import path. If imports
fail, the relevant evidence is native-window/gralloc metadata, the failing KGSL
ioctl, and Mesa/EGL logs; minSdk changes cannot fix a vendor kernel/driver ABI.

## Remaining limits and unverified work

- **APK build, Android lint, Android Java type checking and ARM64 linking have
  not completed. There is no installable deliverable yet.**
- No Android 10 emulator, Snapdragon 660 device, or Debian GPU session was run.
- API 29 cannot implement `ALWAYS` cutout placement exactly. `SHORT_EDGES` is the
  closest platform-supported mode. Exact rounded-corner radii are unavailable.
- Legacy keyboard visibility inference cannot reliably detect every floating
  third-party IME or OEM freeform-window behavior; docked keyboard resizing,
  show/hide, rotation, PiP and extra-key-bar placement need real-device checks.
- VNDK/private graphics layout remains vendor-dependent, as in upstream. The
  fallback fails when the checked ABI is unsupported, rather than calling
  unknown function pointers or silently disabling GPU acceleration.
- `standard` requires the signing identity of GitHub Termux. In this repository,
  **standard uses sharedUserId**; the **compatible variant omits it** for F-Droid
  and other Termux variants. Do not switch a user's Termux installation or
  delete its data just to resolve a signature mismatch.

## Validation actually completed

- Host C tests passed with `-Wall -Wextra -Werror`, compiling the real Android
  branch of `display_consumer.c` against explicitly labeled host test stubs.
  These exercised missing native exports, invalid native-window layouts,
  queue/cancel fence forwarding, allocator failure, FD_CLOEXEC, SCM_RIGHTS
  transfer, and two-way shared mmap visibility. Host backing is an unlinked
  regular file, not an Android ashmem implementation.
- All 17 Java files passed Java syntax parsing. This does not resolve Android
  framework types or methods.
- All 12 manifest/resource XML files parsed successfully.
- Changed shell scripts and the APK verification Python script passed syntax
  checks; `git diff --check` passed.
- The unchanged 5.13.3 protocol was byte-checked.
- The Android build command exited before compilation because Gradle is absent.

## Finish the build

The project retains its upstream build requirements: JDK 21, Gradle 9.6.0,
Android Gradle Plugin 9.2.1, SDK platform 36 and NDK 29.0.14206865. The added
workflow installs build-tools 36.0.0 and CMake 3.22.1 explicitly.

`.github/workflows/build-api29-apk.yml` can run manually or on a push to
`android10-api29` in a writable fork with Actions enabled. It builds the standard
flavor, runs lint, verifies actual output, and uploads the APK artifact only
after verification succeeds. It has been prepared locally, not dispatched.

Once the actual toolchain is available, the local entry points are:

```sh
bash tests/host/run.sh
tools/build-app.sh
python3 tools/verify-api29-apk.py out/AnlandTermux-5.13.3-api29.apk
```

The expected APK filename is a future output, **not a file included in this
archive**. No build result or signature validation is claimed in advance.

## Install/run commands — only after a successful APK build

Download the successfully built APK and the unchanged upstream daemon `.deb`
into Android's Download folder. Run this in **Termux**, outside Debian:

```sh
termux-setup-storage
echo "$TERMUX_APP__APK_RELEASE"
# This build requires the GitHub Termux release; expect GITHUB.
termux-open "$HOME/storage/downloads/AnlandTermux-5.13.3-api29.apk"
# Complete the Android package install dialog, then:
pkg install "$HOME/storage/downloads/anland_5.13.3_aarch64.deb"
command -v anland
pkill -x anland 2>/dev/null || true
anland --socket "$TMPDIR/anland/display_daemon.sock" > "$HOME/anland-daemon.log" 2>&1 &
am start -n com.anland.termux/.MainActivity
```

For a PC with ADB, the APK installation alternative is:

```sh
adb install -r AnlandTermux-5.13.3-api29.apk
```

For an existing Debian 13 PRoot container named `debian`, already configured with
this project's patched compositor/XWayland and your working KGSL Mesa driver:

```sh
# Termux:
proot-distro login debian --shared-tmp --bind /dev/kgsl-3d0:/dev/kgsl-3d0

# Now inside Debian:
test -S /tmp/anland/display_daemon.sock
test -r /dev/kgsl-3d0
startplasma-anland
```

The unchanged helper sets the PRoot/KGSL environment and manages desktop audio.
For the upstream preconfigured Debian image, use its actual container alias,
for example `debian-anland`, instead of `debian`.

For diagnosing KWin's display path manually inside that prepared Debian
container, first stop the previous desktop session. This diagnostic command does
not set up PipeWire; temporarily turn off camera/microphone forwarding in the
Anland settings, as upstream's manual-start instructions require:

```sh
unset DISPLAY ANLAND_DRM_DEVICE LIBGL_ALWAYS_SOFTWARE
export ANLAND=1 ANLAND_SOCKET=/tmp/anland/display_daemon.sock
export ANLAND_NO_DRM_DEVICE=1 EGL_PLATFORM=surfaceless
export MESA_LOADER_DRIVER_OVERRIDE=kgsl GALLIUM_DRIVER=freedreno
export FD_FORCE_KGSL=1 XWAYLAND_FORCE_KGSL_SURFACELESS=1
export QT_QPA_PLATFORM=wayland
export XDG_CURRENT_DESKTOP=KDE XDG_SESSION_DESKTOP=KDE XDG_SESSION_TYPE=wayland
export XDG_RUNTIME_DIR="/tmp/anland-runtime-$(id -u)"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
mkdir -p /tmp/.X11-unix
chmod 1777 /tmp/.X11-unix
dbus-run-session -- startplasma-wayland > "$HOME/anland-plasma.log" 2>&1
```

These commands keep Freedreno OpenGL enabled; Turnip/Vulkan is not required for
this Adreno 512 path. `XWAYLAND_FORCE_KGSL_SURFACELESS` needs the project's
patched XWayland; exporting it alone cannot add that support to stock XWayland.

## Primary sources consulted

- Project source and [user guide](https://github.com/lfdevs/anland-termux/blob/23c77435032e7d9df00f40b90ecee04f1fc4ad9e/docs/user-guide.md).
- [Bionic API-30 declaration](https://github.com/LineageOS/android_bionic/blob/lineage-18.1/libc/include/sys/mman.h), compared with the Android-10 branch's headers and libc symbol map.
- [Android NDK shared memory reference](https://developer.android.com/ndk/reference/group/memory).
- [Android WindowInsets reference](https://developer.android.com/reference/android/view/WindowInsets).
- Android-10 platform [native-window exports](https://github.com/LineageOS/android_frameworks_native/blob/lineage-17.1/libs/nativewindow/libnativewindow.map.txt), [window operation table](https://github.com/LineageOS/android_frameworks_native/blob/lineage-17.1/libs/nativewindow/include/system/window.h), and [buffer layout](https://github.com/LineageOS/android_frameworks_native/blob/lineage-17.1/libs/nativebase/include/nativebase/nativebase.h), as preserved in LineageOS 17.1.
- Upstream [producer shared-memory pickup](https://github.com/superturtlee/anland/blob/6e8f1c6f8a53918484dc0c117b4d60ed85bb383a/libdisplay_producer/display_producer.c).
