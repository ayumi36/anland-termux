#!/usr/bin/env python3
"""Verify an actual APK using the real Android SDK and API 29 NDK stubs."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import zipfile


def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True, stderr=subprocess.STDOUT)


def symbols(readelf, library):
    defined, required = set(), set()
    for line in run(readelf, "--dyn-syms", "--wide", library).splitlines():
        fields = line.split()
        if len(fields) < 8 or not fields[0].endswith(":"):
            continue
        name = fields[7].split("@")[0]
        if fields[6] == "UND":
            if fields[4] != "WEAK":
                required.add(name)
        elif fields[4] in ("GLOBAL", "WEAK"):
            defined.add(name)
    return defined, required


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify-api29-apk.py APK")
    apk = Path(sys.argv[1]).resolve(strict=True)
    sdk_setting = os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if not sdk_setting:
        raise SystemExit("ANDROID_HOME must point to an installed Android SDK")
    sdk = Path(sdk_setting)
    build_tools = sdk / "build-tools/36.0.0"
    ndk = sdk / "ndk/29.0.14206865/toolchains/llvm/prebuilt/linux-x86_64"
    readelf = ndk / "bin/llvm-readelf"
    stubs = ndk / "sysroot/usr/lib/aarch64-linux-android/29"
    if not stubs.is_dir():
        raise SystemExit("The actual NDK API 29 linker stubs are required")

    signature = run(build_tools / "apksigner", "verify", "--verbose", "--print-certs",
                    "--min-sdk-version", "29", apk)
    badging = run(build_tools / "aapt2", "dump", "badging", apk)
    print("aapt2 dump badging output:")
    print(badging, end="" if badging.endswith("\n") else "\n")
    min_sdk = re.search(r"(?:^|\s)sdkVersion:'([^']+)'(?:\s|$)", badging)
    if not min_sdk or min_sdk.group(1) != "29":
        actual = min_sdk.group(1) if min_sdk else "missing"
        raise SystemExit(f"APK minSdk must be exactly 29 (aapt2 reported {actual})")
    if not re.search(r"^package: name='com.anland.termux'", badging, re.M):
        raise SystemExit("Unexpected application ID")
    manifest = run(build_tools / "aapt2", "dump", "xmltree", apk, "--file", "AndroidManifest.xml")
    if not re.search(r'sharedUserId[^\n]*"com\.termux"', manifest):
        raise SystemExit("Standard transport must retain com.termux sharedUserId")

    # Linker stubs, not a hand-written allowlist, define API 29 availability.
    platform_symbols = set()
    for stub in stubs.glob("*.so"):
        platform_symbols.update(symbols(readelf, stub)[0])

    native_reports = []
    with tempfile.TemporaryDirectory(prefix="anland-apk-check-") as temp, zipfile.ZipFile(apk) as z:
        native = [name for name in z.namelist() if name.startswith("lib/") and name.endswith(".so")]
        if "lib/arm64-v8a/libanland_consumer.so" not in native:
            raise SystemExit("Missing actual ARM64 display backend")
        if any(not name.startswith("lib/arm64-v8a/") for name in native):
            raise SystemExit("Unexpected architecture; this build targets ARM64 only")
        app_symbols, required_by_library = set(), {}
        for index, name in enumerate(native):
            library = Path(temp) / (str(index) + ".so")
            library.write_bytes(z.read(name))
            header = run(readelf, "-h", library)
            if not re.search(r"Machine:\s+AArch64", header):
                raise SystemExit("Non-AArch64 ELF: " + name)
            defined, required = symbols(readelf, library)
            app_symbols.update(defined)
            required_by_library[name] = required
        for name, required in required_by_library.items():
            unavailable = required - platform_symbols - app_symbols
            if unavailable:
                raise SystemExit(f"Symbols unavailable in API 29: {name}: {sorted(unavailable)}")
            if "memfd_create" in required:
                raise SystemExit("API-30-only memfd_create dependency remains")
            native_reports.append(f"{name}: ARM64; {len(required)} strong imports available on API 29")

    report = "\n".join([
        "APK BUILD VERIFICATION (not device runtime verification)",
        f"APK: {apk.name}",
        "SHA256: " + hashlib.sha256(apk.read_bytes()).hexdigest(),
        "minSdk: 29; applicationId: com.anland.termux; sharedUserId: com.termux",
        *native_reports, signature,
        "Still requires Android 10 device testing of GPU import, UI and container session.",
    ]) + "\n"
    (apk.parent / "api29-verification.txt").write_text(report)
    print(report, end="")


if __name__ == "__main__":
    main()
