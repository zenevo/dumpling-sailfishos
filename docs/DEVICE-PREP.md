# 真机准备清单（OnePlus 5T / dumpling）

目标：把手机刷成 **Android 9（Pie）基线 + TWRP**，然后刷我们的 SFOS 5.2.0.22 aarch64 zip。

> 刷机有风险，会清空数据。先备份，再动手。

## 0. 需要准备的文件

| 文件 | 说明 | 来源 |
|---|---|---|
| `firmware_9.0.11_oneplus5T.zip` | OOS 9.0.11 固件（vendor/modem 等） | [crdroid sourceforge](https://sourceforge.net/projects/crdroid/files/dumpling/6.x/firmware/) |
| `twrp-3.4.0-0-cheeseburger_dumpling.img` | TWRP（5/5T 通用） | [dl.twrp.me](https://dl.twrp.me/cheeseburger_dumpling/) |
| Android 9 `/system` 基线 | 官方 LineageOS 16.0 已下架；见下方“基线选择” | — |
| `sailfishos-5.2.0.22-20261030-dumpling.zip` | 我们的 SFOS 包 | `out/` |

电脑上装好 `adb`、`fastboot`，手机电量 > 50%。

## 1. 解锁与 TWRP

```sh
# 手机：设置 → 关于手机 → 连点版本号开启开发者选项
#       开发者选项 → 打开“OEM 解锁”和“USB 调试”

adb reboot bootloader
fastboot oem unlock          # 手机上确认，会清空数据
fastboot flash recovery twrp-3.4.0-0-cheeseburger_dumpling.img
fastboot boot twrp-3.4.0-0-cheeseburger_dumpling.img
```

## 2. 刷固件（Android 9 vendor）

TWRP → Install → `firmware_9.0.11_oneplus5T.zip`。这步决定 vendor/modem 版本，
**必须是 9.0.11**（Android 9 的最后固件）。

## 3. 基线选择（关键）

SFOS 的 HAL 是对着 **LineageOS 16.0（Android 9）** 编的，设备上 `/system` 必须是
Android 9 的 LineageOS 系。官方 16.0 包已经不在 download.lineageos.org 上了，三个选择：

### A. 找一份 LineageOS 16.0 dumpling zip（推荐）
第三方镜像、XDA 存档都可以，只要是官方/非官方 LOS 16.0，或任何
`lineage.build.version=16.0` 的 Pie ROM。TWRP 直接刷。

### B. 用我们编的 `/system` 镜像（后备，正在生成）
`out/system.img` 与 HAL 同一份源码编出，最匹配。刷法：

```sh
adb reboot bootloader
fastboot flash system system.img
```

vendor 仍用第 2 步 OOS 9.0.11 的，不要刷我们的 `vendor.img`。

### C. 直接跑在 OOS 9.0.11 的 /system 上
理论上 OOS 也是 Android 9，但改装多、没验证过，**不推荐**。

> installer 会检查 `/system/build.prop` 里的 `lineage.build.version`。用 B/C 时
> 第一次刷会提示不要当 OTA 刷、要求确认；**再刷一次同一个 zip** 就会强制安装。

## 4. 格式化 data

TWRP → Wipe → Advanced Wipe → 选中 **Data** → Repair or Change File System →
**ext4**（`/cache` 也保持 ext4）。installer 会检查，f2fs 会直接拒绝。

## 5. 刷入前自检

```sh
adb shell getprop ro.product.device        # 必须是 dumpling
adb shell getprop ro.build.version.release # 必须是 9
adb shell ls /system/bin/toybox            # 必须存在
adb shell mount | grep " /data "           # 必须是 ext4
```

## 6. 刷 SFOS

```sh
adb push sailfishos-5.2.0.22-20261030-dumpling.zip /sdcard/
adb reboot recovery
# TWRP → Install → 选 zip → 滑动确认
```

installer 会：解 rootfs 到 `/data/.stowaways/sailfishos` → 把当前 boot 备份成
`/sdcard/droid-boot.img` → 写入 `hybris-boot.img`。之后重启，首启 2–5 分钟。

## 7. 回退

- TWRP → Install Image → 选 `/sdcard/droid-boot.img` 刷回 boot；或
- 重刷 LineageOS zip / 用 `fastboot flash boot boot.img`。

SFOS 的 rootfs 在 `/data/.stowaways`，不影响 Android 的 `/system`。

## 提醒

- 这是 **aarch64 首次构建**，还没人验证过首启。先把回退路径准备好再刷。
- 没有 VoLTE：国内 2G/3G 退网后**打电话会失败**，数据和 Spotify 不受影响。
- 首启卡住：USB 连电脑 `telnet 192.168.2.15 2323`，
  跑 `/usr/libexec/droid-hybris/system/bin/logcat` 看 Android 侧日志。
