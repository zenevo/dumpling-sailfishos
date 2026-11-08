# Sailfish OS 5.2.0.22 for OnePlus 5T (`dumpling`)

An unofficial community build of **Sailfish OS 5.2.0.22 (Finlayson)** for the
**OnePlus 5T** (codename `dumpling`, Qualcomm MSM8998 / Snapdragon 835).

This repository is the packaging set (`droid-hal`, `droid-config`,
`droid-hal-version`) plus the pre-built flashable images. It continues the
work of the old [sailfishos-oneplus5](https://github.com/sailfishos-oneplus5)
port (`cheeseburger`, covering OnePlus 5 / 5T at the Sailfish 4.3 era),
rebased on **Sailfish OS 5.2.0.22** and kept on the **LineageOS 16.0 /
Android 9 (Pie)** Android base layer.

中文文档：[README.zh-CN.md](README.zh-CN.md)

> [!WARNING]
> **This is not an official Jolla product.** It is a community port built with
> the [Sailfish OS HADK](https://docs.sailfishos.org/Tools/HADK/).
> Flashing custom firmware can brick your device and void your warranty.
> You are responsible for your own device.

---

## Status

Tested on a OnePlus 5T (`A5010`, 6/64 GB) with LineageOS 16.0 as the Android
base layer.

| Feature | Status |
|---|---|
| Boot into Sailfish OS | ✅ |
| Touch / display (1080 × 2160) | ✅ |
| Wi-Fi | ✅ |
| Mobile data (LTE) | ✅ |
| Calls / SMS | ⚠️ untested — no VoLTE (no IMS stack in the port) |
| Bluetooth | ⚠️ untested |
| Audio playback | ✅ |
| GPS | ⚠️ untested |
| Camera (photos) | ✅ |
| Camera (video recording) | ✅ 1080p (4K capped — see Known issues) |
| Video playback (Gallery) | ✅ |
| USB developer mode / SSH | ✅ |
| Storeman / OpenRepos | ✅ |
| Browser (Gecko) | ✅ |

---

## Requirements

- OnePlus 5T (`dumpling`) with **unlocked bootloader** and **TWRP 3.7.x**.
- **OxygenOS 9.0.x firmware** (or the LineageOS 16.0 firmware) already on
  the device — the port needs the Android 9 vendor blobs.
- A host PC with `adb` / `fastboot`.
- [LineageOS 16.0 for `dumpling`](https://download.lineageos.org/)
  (`lineage-16.0-*-nightly-dumpling-signed.zip`).
- A flashable Sailfish OS image from the [Releases](../../releases) page.

---

## Download

The image is attached to the **Releases** of this repository:

| File | Size | MD5 |
|---|---|---|
| `sailfishos-5.2.0.22-20261107-dumpling.zip` | 712 MB | `38e8557369ade28d0a8f491afbb8180e` |

This image is the plain port (no extra pre-installed applications), built from
the sources in this repository with the device fixes described below.

Verify the checksum before flashing:

```sh
md5sum sailfishos-5.2.0.22-*-dumpling.zip
```

## Installation

> [!IMPORTANT]
> **Do not skip "Format Data".** LineageOS 16.0 uses file-based encryption on
> `/data`; Sailfish OS cannot read it. Booting Android *after* flashing
> LineageOS (before flashing Sailfish OS) re-encrypts `/data` and the Sailfish
> installer will fail.

1. Boot into **TWRP** (Volume Up + Power).
2. **Wipe → Format Data → `yes`**.
3. Push both zips:
   ```sh
   adb push lineage-16.0-*-nightly-dumpling-signed.zip /sdcard/
   adb push sailfishos-5.2.0.22-*-dumpling-fixed.zip /sdcard/
   ```
4. Flash **LineageOS 16.0** in TWRP.
   **Do not reboot into Android.**
5. Without rebooting, flash the **Sailfish OS** image.
6. **Reboot → System.**

### TL;DR

```text
Format Data
  → flash LineageOS 16.0 (do NOT boot it)
  → flash Sailfish OS image
  → reboot to system
```

First boot takes 1–2 minutes.

---

## What is included / what was fixed

This port differs from a plain HADK build in a few device-specific ways:

| Fix | Where |
|---|---|
| **Video recording** — the stock LineageOS `libstagefright.so` lacks the hybris patch for 64-bit buffer marshalling between the 64-bit camera process and the 32-bit OMX encoder (`ACodec.cpp`, `VideoNativeMetadata_ptrSized`). Without it every recorded frame is dropped and the camera app deadlocks when stopping a recording. | patched `libstagefright.so` (and a `libcameraservice.so` with the shutter sound disabled) written to `/system/lib64` by the installer |
| **Camera + video playback** — `gst-droid` rebuilt with the **hybris shim**: `/usr/share/droidmedia/hybris.c` is compiled into both the plugin and the base library, so the plugin has **no bionic (Android) library dependencies** and `dlopen()`s the Android media stack at runtime through `libhybris-common`. A naive `-ldroidmedia` link fails to load in the camera app process (the camerabin then falls back to `fake-auto-video-src` — the colour-bar preview). The build also carries the NV12 decoder fix. Video capture is capped at 1080p (see Known issues). | Sailfish rootfs (`/usr/lib64/gstreamer-1.0/libgstdroid.so`, `/usr/lib64/libgstdroid-1.0.so.1.0.0`, `/usr/lib64/qt5/qml/com/jolla/camera/capture/CaptureView.qml`) |
| **USB developer mode** — the default `usb-moded` "ask" mode re-enumerates the USB gadget on screen-lock, breaking RNDIS/SSH. The port uses a persistent developer mode. | `/etc/usb-moded/70-use-developer-mode.ini` |
| **Networking** — static `/etc/resolv.conf` (the systemd-resolved stub does not work on this kernel) and `/etc/gai.conf` preferring IPv4 (no global IPv6 on most networks). | Sailfish rootfs |
| **Browser** — `gfx.webrender.compositor=false` in `/usr/share/sailfish-browser/data/prefs.js`; without it Gecko WebRender shows a black page on this libhybris stack. | Sailfish rootfs |

The full bring-up log, debugging notes and build scripts are in
[`docs/BRINGUP-NOTES.md`](docs/BRINGUP-NOTES.md) (Chinese), and the flashing steps in
[`docs/FLASHING.md`](docs/FLASHING.md).

---

## Building

The port is built with the
[Sailfish OS Hardware Adaptation Development Kit (HADK)](https://docs.sailfishos.org/Tools/HADK/)
against a **LineageOS 16.0** tree.

```sh
# inside the HADK environment
source build/envsetup.sh
lunch lineage_dumpling-userdebug
make hybris-hal
```

Then follow the HADK "Deploy the build" chapters to produce the three
packages:

- `droid-hal-dumpling` — Android HAL
- `droid-config-dumpling` — device configuration
- `droid-hal-version-dumpling` — version package

### Repository layout

```text
.
├── README.md                  # this file
├── docs/
│   └── BRINGUP-NOTES.md       # detailed bring-up / debugging notes (CN)
├── out/                       # pre-built flashable images (not committed)
├── droid-hal/                 # droid-hal packaging
├── droid-config/              # droid-config packaging
└── droid-hal-version/         # droid-hal-version packaging
```

---

## Known issues

- **4K recording quality** — the camera app would pick 3840×2160 but the
  encoder only produces ~11.6 Mbps H.264 Baseline, which shows blockiness on
  moving scenes. This port caps video capture at 1920×1080 (see
  `CaptureView.qml` `_pickResolution()`); 4K can be re-enabled by removing the
  `maxPixels` cap, at the cost of the blockiness.
- **No VoLTE** — no IMS stack; calls fall back to 2G/3G where available.
- **USB `idVendor` reports `0000`** — cosmetic on Linux, may confuse Windows
  driver matching.
- **`iproute2` is not installed** in the rootfs (`ip` is missing); use
  `/proc/net/*` or install it with `zypper`.
- **Record button delay** — ~1 s while the camera HAL reconfigures the video
  stream.

---

## Credits & references

This port would not exist without the work of the Sailfish OS community.

### Sailfish OS / Mer

- [Sailfish OS HADK](https://docs.sailfishos.org/Tools/HADK/) — the Hardware
  Adaptation Development Kit used to build this port.
- [Sailfish OS Platform SDK](https://docs.sailfishos.org/Tools/Platform_SDK/)
- [Sailfish OS Open Build Service](https://build.sailfishos.org/) — the
  [`nemo:devel:hw:oneplus:dumpling`](https://build.sailfishos.org/project/show/nemo:devel:hw:oneplus:dumpling)
  project was used as a reference for the device adaptation.
- [sailfishos-oneplus5](https://github.com/sailfishos-oneplus5) — the original
  OnePlus 5/5T port this repository is based on.

### Hybris / Android compatibility layer

- [libhybris](https://github.com/libhybris/libhybris) — GL/EGL/Android library
  wrapper (`LGPL-2.1`).
- [mer-hybris/droidmedia](https://github.com/sailfishos/droidmedia) — Android
  media framework wrapper (`Apache-2.0`).
- [mer-hybris/gst-droid](https://github.com/sailfishos/gst-droid) — GStreamer
  elements for the Android HAL (`LGPL-2.1-or-later`).
- [mer-hybris/hybris-boot](https://github.com/mer-hybris/hybris-boot) — boot
  initramfs.

### Android base

- [LineageOS 16.0](https://lineageos.org/) for `dumpling` — Android 9 base
  layer (the port requires its `system` and `vendor` partitions).
- [TWRP](https://twrp.me/) — recovery used for flashing.
- [Android Open Source Project](https://source.android.com/) — the
  `frameworks/av` sources patched for the recording fix.

### Tools / libraries

- [GStreamer](https://gstreamer.freedesktop.org/) (`LGPL-2.0-or-later`)
- [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
- [connman](https://01.org/connman)
- [Storeman](https://github.com/mentaljam/harbour-storeman) (`GPL-3.0`) —
  optional, only in the `-apps` image.
- [Xray-core](https://github.com/XTLS/Xray-core) (`MPL-2.0`) — optional,
  only in the `-apps` image.

### Documentation / community

- [Sailfish OS docs](https://docs.sailfishos.org/)
- [Sailfish OS Forum](https://forum.sailfishos.org/)

If you are the author of a component used here and want the attribution
changed, please open an issue.

---

## License

The **scripts, configuration and documentation** in this repository are
released under the [MIT License](LICENSE).

> [!CAUTION]
> The **flashable images embed a Sailfish OS root filesystem**, which is
> Jolla's proprietary software and is **not** covered by the MIT license.
> Redistributing a full Sailfish OS image may violate Jolla's terms of use.
> If you publish this port, prefer publishing only the scripts, configuration
> and patches (plus build instructions) and let users build the image
> themselves — or verify Jolla's licensing terms first.

Third-party components keep their own licenses:

| Component | License |
|---|---|
| `droidmedia` | Apache-2.0 |
| `gst-droid` | LGPL-2.1-or-later |
| `libhybris` | LGPL-2.1 |
| GStreamer | LGPL-2.0-or-later |
| Storeman | GPL-3.0 |
| Xray-core | MPL-2.0 |

See the respective upstream repositories for the full license texts.

---

## Disclaimer

- "Sailfish OS" is a trademark of **Jolla Ltd.**
- "OnePlus", "OnePlus 5T" are trademarks of **OnePlus Technology**.
- "LineageOS" is a trademark of the **LineageOS project**.
- This project is **not affiliated with, endorsed by, or supported by** Jolla,
  OnePlus or the LineageOS project.
- The software is provided **“as is”**, without warranty of any kind.
