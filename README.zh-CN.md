# 一加 5T（`dumpling`）Sailfish OS 5.2.0.22

这是一份面向 **一加 5T**（代号 `dumpling`，高通 MSM8998 / 骁龙 835）的
**Sailfish OS 5.2.0.22（Finlayson）** 非官方社区移植。

仓库内容 = 移植打包源码（`droid-hal` / `droid-config` / `droid-hal-version`）+ 构建脚本 + 文档，
另外在 Releases 里提供构建好的可刷入镜像。

本移植延续了老项目 [sailfishos-oneplus5](https://github.com/sailfishos-oneplus5)
（`cheeseburger`，覆盖一加 5 / 5T，止步于 Sailfish 4.3 时代），
把底包升级到 **Sailfish OS 5.2.0.22**，Android 底层仍是 **LineageOS 16.0 / Android 9（Pie）**。

英文文档：[README.md](README.md)

> [!WARNING]
> **这不是 Jolla 官方产品。** 这是用 [Sailfish OS HADK](https://docs.sailfishos.org/Tools/HADK/)
> 构建的社区移植。刷入非官方固件可能导致设备变砖、失去保修，风险自负。

---

## 功能状态

测试机：一加 5T（`A5010`，6/64 GB），Android 底层为 LineageOS 16.0。

| 功能 | 状态 |
|---|---|
| 启动进入 Sailfish OS | ✅ |
| 触摸 / 显示（1080 × 2160） | ✅ |
| Wi-Fi | ✅ |
| 移动数据（LTE） | ✅ |
| 通话 / 短信 | ❌ LTE-only 网络下不可用 —— 需要 VoLTE，详见下文 |
| 蓝牙 | ⚠️ 未测试 |
| 音频播放 | ✅ |
| GPS | ⚠️ 未测试 |
| 相机（拍照） | ✅ |
| 相机（录像） | ✅ 1080p（4K 被限制，见「已知问题」） |
| 视频播放（相册） | ✅ |
| USB 开发者模式 / SSH | ✅ |
| Storeman / OpenRepos | ✅ |
| 浏览器（Gecko） | ✅ |

---

## 前置要求

- 一加 5T（`dumpling`），**已解锁 bootloader**，装有 **TWRP 3.7.x**；
- 设备上已有 **OxygenOS 9.0.x** 固件（或 LineageOS 16.0 底包）——移植需要 Android 9 的 vendor blobs；
- 一台装有 `adb` / `fastboot` 的电脑；
- [LineageOS 16.0 for `dumpling`](https://download.lineageos.org/)
  （`lineage-16.0-*-nightly-dumpling-signed.zip`）；
- 本仓库 Releases 里的 Sailfish OS 镜像。

---

## 下载

镜像在本仓库的 **Releases** 页面：

| 文件 | 大小 | MD5 |
|---|---|---|
| `sailfishos-5.2.0.22-20261107-dumpling.zip` | 712 MB | `38e8557369ade28d0a8f491afbb8180e` |

这个镜像是**纯净版**（未预装额外应用），由本仓库源码构建，包含下文列出的设备修复。

刷入前请先校验：

```sh
md5sum sailfishos-5.2.0.22-*-dumpling.zip
```

---

## 刷入步骤

> [!IMPORTANT]
> **「Format Data」不能跳过。** LineageOS 16.0 的 `/data` 是文件级加密，Sailfish OS 读不了。
> 如果在刷完 LineageOS 之后先启动过一次 Android，`/data` 会被重新加密，Sailfish 安装器就会失败。

1. 进入 **TWRP**（音量上 + 电源）；
2. **Wipe → Format Data → 输入 `yes`**；
3. 推送两个 zip：
   ```sh
   adb push lineage-16.0-*-nightly-dumpling-signed.zip /sdcard/
   adb push sailfishos-5.2.0.22-*-dumpling.zip /sdcard/
   ```
4. 在 TWRP 里刷入 **LineageOS 16.0**；
   **不要重启进 Android。**
5. 不重启，直接刷入 **Sailfish OS** 镜像；
6. **Reboot → System**。

### 极简流程

```text
Format Data
  → 刷 LineageOS 16.0（不要启动它）
  → 刷 Sailfish OS 镜像
  → 重启进入系统
```

首次启动约 1–2 分钟。

---

## 相比原生 HADK 构建做了哪些修复

| 修复 | 位置 |
|---|---|
| **录像** —— 原版 LineageOS 的 `libstagefright.so` 缺少 hybris 补丁（64 位相机进程与 32 位 OMX 编码器之间的缓冲区编组，涉及 `ACodec.cpp`、`VideoNativeMetadata_ptrSized`）。没有这个补丁，每一帧都会被丢弃，停止录像时相机还会卡死。 | 安装器写入 `/system/lib64` 的补丁版 `libstagefright.so`（以及去掉快门声的 `libcameraservice.so`） |
| **相机 + 视频播放** —— `gst-droid` 用 **hybris shim** 重新编译：把 `/usr/share/droidmedia/hybris.c` 编译进插件和基础库，使插件**不依赖任何 bionic（Android）库**，运行时通过 `libhybris-common` 动态加载 Android 媒体栈。简单的 `-ldroidmedia` 链接会在相机进程里加载失败（camerabin 随后退回到 `fake-auto-video-src` —— 也就是彩色条纹预览）。该构建还包含 NV12 解码修复。录像上限 1080p（见「已知问题」）。 | Sailfish rootfs（`/usr/lib64/gstreamer-1.0/libgstdroid.so`、`/usr/lib64/libgstdroid-1.0.so.1.0.0`、`/usr/lib64/qt5/qml/com/jolla/camera/capture/CaptureView.qml`） |
| **USB 开发者模式** —— 默认的 `usb-moded`「询问」模式会在锁屏时重新枚举 USB gadget，导致 RNDIS / SSH 断连。本移植使用持久的开发者模式。 | `/etc/usb-moded/70-use-developer-mode.ini` |
| **网络** —— 静态 `/etc/resolv.conf`（这个内核上 systemd-resolved 的 stub 不工作），以及优先 IPv4 的 `/etc/gai.conf`（多数网络没有全局 IPv6）。 | Sailfish rootfs |
| **浏览器** —— `/usr/share/sailfish-browser/data/prefs.js` 里设置 `gfx.webrender.compositor=false`；否则在这套 libhybris 栈上 Gecko WebRender 会显示黑屏。 | Sailfish rootfs |

完整的 bring-up 记录、调试笔记和构建脚本见
[`docs/BRINGUP-NOTES.md`](docs/BRINGUP-NOTES.md)，刷机细节见 [`docs/FLASHING.md`](docs/FLASHING.md)。

---

## 预装应用：CPU Mode

本仓库构建的每个镜像都内置 **CPU Mode** —— 一个针对骁龙 835 大小核架构
（4 × Cortex-A53「小核」+ 4 × Cortex-A73「大核」）的原生 Sailfish 小应用。

| 功能 | 说明 |
|---|---|
| 三种模式 | **省电**（限制频率，优先续航）· **均衡**（默认；interactive 调度，小核保持在 1.2 GHz 以上）· **性能**（全核解锁） |
| 实时曲线 | 小核 / 大核各 60 个采样点的滚动频率图，每 500 ms 采样一次 |
| 读数 | 当前小核 / 大核频率（MHz）以及各模式的频率上限 |
| 开机保持 | 选择的模式保存在 `~/.config/cpu-mode/mode`，开机时由 `cpu-mode.service`（通过 `cpu-mode.path` 触发）重新应用 |
| 相关文件 | `/usr/bin/cpu-mode`、`/usr/bin/cpu-mode-apply`、`/usr/share/cpu-mode/qml/`、`/usr/lib/systemd/system/cpu-mode.{path,service}` |

同时预装了 `lscpu`，`/usr/bin/lscpu` 可直接使用。

应用源码（C++ 后端、Silica QML、systemd unit 和 RPM spec）见 [`cpu-mode/`](cpu-mode)。

---

## 自行构建

本移植用 [Sailfish OS HADK](https://docs.sailfishos.org/Tools/HADK/)
针对 **LineageOS 16.0** 源码树构建。

```sh
# 在 HADK 环境里
source build/envsetup.sh
lunch lineage_dumpling-userdebug
make hybris-hal
```

然后按 HADK「Deploy the build」章节生成三个包：

- `droid-hal-dumpling` —— Android HAL
- `droid-config-dumpling` —— 设备配置
- `droid-hal-version-dumpling` —— 版本包

### 仓库结构

```text
.
├── README.md                       # 英文说明
├── README.zh-CN.md                 # 本文件
├── LICENSE
├── docs/
│   ├── BRINGUP-NOTES.md            # 详细 bring-up / 调试记录
│   ├── BUILD-LOG.md                # 5.2.0.22 构建日志
│   ├── DEVICE-PREP.md              # 真机准备清单
│   ├── FLASHING.md                 # 刷入说明
│   └── SPOTIFY-WAVE.md             # Wave（Spotify 客户端）打包说明
├── build/                          # kickstart
├── local_manifests/                # repo 清单（lineage + oneplus msm8998 common）
├── droid-hal-dumpling/             # droid-hal 打包
├── droid-config-dumpling/          # droid-config 打包
└── droid-hal-version-dumpling/     # droid-hal-version 打包
```

---

## 已知问题

- **4K 录像画质** —— 相机默认会选 3840×2160，但编码器只输出约 11.6 Mbps 的 H.264 Baseline，
  运动画面方块化严重。本移植把录像限制在 1920×1080（见 `CaptureView.qml` 的 `_pickResolution()`）；
  去掉 `maxPixels` 限制即可恢复 4K，代价是画质下降。
- **无 VoLTE —— 在 LTE-only 网络下无法通话和收发短信。** modem 和它的 IMS 守护进程
  （`imsqmidaemon`、`imsdatadaemon`、`imsrcsd`、`ims_rtp_daemon`）都在跑，移植也配置了
  `ims` APN，但 **oFono 1.29 没有 IMS 客户端**，而 Android 9 的 vendor **没有 IMS HAL**
  （`android.hardware.radio.ims` / `vendor.qti.hardware.radio.ims`）可供驱动 —— Android 9 的
  IMS 是跑在 Android framework 内部的。新版移植（如 Android 13 上的 VerdandiTeam/miami）
  使用的 `ofono-binder-plugin-ext-qti` 扩展在这里能装、能加载，但没有对接对象。
  因此 LTE-only 网络下通话约 30 秒后失败；只有在仍有 2G/3G 覆盖的地方才能回落通话。
  要支持 VoLTE，必须把移植的 Android 底包升级到 **Android 11+**。
- **oFono 连续重启几次后会丢 SIM** —— 连续 `systemctl restart ofono` 可能导致
  `SimManager.Present = false`，重启手机即可恢复。
- **USB `idVendor` 显示 `0000`** —— Linux 下只是观感问题，Windows 驱动匹配可能受影响。
- **rootfs 里没有 `iproute2`**（`ip` 命令缺失）；可用 `/proc/net/*`，或通过 `zypper` 安装。
- **录像按钮延迟** —— 相机 HAL 重新配置视频流时约有 1 秒延迟。

---

## 致谢

没有 Sailfish OS 社区的工作，就没有这个移植。

### Sailfish OS / Mer

- [Sailfish OS HADK](https://docs.sailfishos.org/Tools/HADK/) —— 构建本移植所用的硬件适配开发套件
- [Sailfish OS Platform SDK](https://docs.sailfishos.org/Tools/Platform_SDK/)
- [Sailfish OS Open Build Service](https://build.sailfishos.org/) —— 其中
  [`nemo:devel:hw:oneplus:dumpling`](https://build.sailfishos.org/project/show/nemo:devel:hw:oneplus:dumpling)
  项目作为设备适配参考
- [sailfishos-oneplus5](https://github.com/sailfishos-oneplus5) —— 本仓库所基于的原始一加 5/5T 移植

### Hybris / Android 兼容层

- [libhybris](https://github.com/libhybris/libhybris) —— GL/EGL/Android 库封装（`LGPL-2.1`）
- [mer-hybris/droidmedia](https://github.com/sailfishos/droidmedia) —— Android 媒体框架封装（`Apache-2.0`）
- [mer-hybris/gst-droid](https://github.com/sailfishos/gst-droid) —— Android HAL 的 GStreamer 元件（`LGPL-2.1-or-later`）
- [mer-hybris/hybris-boot](https://github.com/mer-hybris/hybris-boot) —— 启动 initramfs

### Android 底层

- [LineageOS 16.0](https://lineageos.org/) for `dumpling` —— Android 9 底层（移植依赖它的 `system` 与 `vendor` 分区）
- [TWRP](https://twrp.me/) —— 刷机用的 recovery
- [Android Open Source Project](https://source.android.com/) —— 为录像修复而打补丁的 `frameworks/av`

### 工具 / 库

- [GStreamer](https://gstreamer.freedesktop.org/)（`LGPL-2.0-or-later`）
- [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
- [connman](https://01.org/connman)
- [Storeman](https://github.com/mentaljam/harbour-storeman)（`GPL-3.0`）—— 可选
- [Xray-core](https://github.com/XTLS/Xray-core)（`MPL-2.0`）—— 可选

### 文档 / 社区

- [Sailfish OS docs](https://docs.sailfishos.org/)
- [Sailfish OS Forum](https://forum.sailfishos.org/)

如果你是本仓库用到的某个组件的作者、希望调整署名方式，欢迎开 issue。

---

## 许可证

本仓库中的**脚本、配置和文档**以 [MIT License](LICENSE) 发布。

> [!CAUTION]
> **可刷入镜像内嵌了 Sailfish OS 根文件系统**，那是 Jolla 的专有软件，**不**受 MIT 许可证覆盖。
> 分发完整的 Sailfish OS 镜像可能违反 Jolla 的使用条款。如果你要发布这个移植，建议只发布脚本、
> 配置和补丁（加上构建说明），让用户自行构建镜像——或者先确认 Jolla 的授权条款。

第三方组件保留各自的许可证：

| 组件 | 许可证 |
|---|---|
| `droidmedia` | Apache-2.0 |
| `gst-droid` | LGPL-2.1-or-later |
| `libhybris` | LGPL-2.1 |
| GStreamer | LGPL-2.0-or-later |
| Storeman | GPL-3.0 |
| Xray-core | MPL-2.0 |

完整许可证文本请见各上游仓库。

---

## 免责声明

- 「Sailfish OS」是 **Jolla Ltd.** 的商标；
- 「OnePlus」「OnePlus 5T」是 **OnePlus Technology** 的商标；
- 「LineageOS」是 **LineageOS project** 的商标；
- 本项目**与 Jolla、OnePlus、LineageOS 无任何关联、未获其认可、不受其支持**；
- 软件按**「原样」**提供，不附带任何形式的担保。
