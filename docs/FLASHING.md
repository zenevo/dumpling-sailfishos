# 刷入说明（5.2.0.22，aarch64）

本 zip 是 **hybris-installer 的完整重装包**，不是增量 OTA。它做四件事：

1. 校验设备与 Android 基线；
2. 把 rootfs 解压到 `/data/.stowaways/sailfishos`（不碰 `/system`、`/vendor`）；
3. 备份当前 boot 分区为 `/sdcard/droid-boot.img`；
4. 把包内的 `hybris-boot.img` 写到 boot 分区，并链到 `/sdcard/`。

因此 `/data` 至少要有 ~5 GB 空闲。

## 前置条件（缺一不可）

| 项 | 要求 | installer 的检查 |
|---|---|---|
| 设备 | OnePlus 5T（`dumpling`） | `ro.product.device` 必须是 `dumpling`，否则 abort 7 |
| 固件 | OxygenOS **9.0.11**（5T 版） | 提供匹配的 vendor 固件/blobs |
| 分区 | `/vendor` 存在（Treble） | 不存在则 abort 4 |
| 文件系统 | `/data`、`/cache` 都是 **ext4** | 否则 abort 1/3 |
| Android 基线 | **LineageOS 16.0**（官方 dumpling 包） | `/system/build.prop` 里 `lineage.build.version` 必须是 `16.0`，否则 abort 6 |
| Recovery | TWRP ≥ 3.3（`twrp-3.4.0-0-cheeseburger_dumpling`） | 需要 `/system/bin/toybox` 解包，abort 8 |

> 注意最后两条：**这个包只认 LineageOS 16.0**。官方 16.0 的 `build.prop` 带
> `lineage.build.version=16.0`，我们编 droid-hal/libhybris 用的也是这份源码。

## 步骤

```sh
# 0) 备份（可选但强烈建议）
#    TWRP → Backup → Boot/System/Vendor/Data

# 1) 准备 Android 9 基线
#    - 刷 OxygenOS 9.0.11 固件（firmware_9.0.11_oneplus5T.zip）
#    - 刷 TWRP（recovery 分区）
#    - 四清后刷官方 LineageOS 16.0 dumpling zip

# 2) 格式化 data 为 ext4（TWRP → Wipe → Advanced Wipe → 选中 Data → Repair or Change FS → ext4）
#    /cache 同理保持 ext4

# 3) 推包并刷入
adb push sailfishos-5.2.0.22-20261030-dumpling.zip /sdcard/
adb reboot recovery
#   TWRP → Install → 选择该 zip → 滑动确认

# 4) 重启
```

如果 `$ROOT/etc/os-release` 已存在（之前装过 SFOS），installer 会 abort 9 要求
**再刷一次**确认全量重装；照做即可。

## 首次启动与排错

- 首次启动 2–5 分钟，期间黑屏/转圈都正常。
- 卡住时：USB 连电脑，`telnet 192.168.2.15 2323`（SFOS 调试口），
  然后 `/usr/libexec/droid-hybris/system/bin/logcat` 看 Android 侧日志。
- 常见问题：
  - 开机直接进 recovery / 黑屏 → boot 镜像与内核不匹配，重刷 LineageOS boot 再排查；
  - 卡在 Sailfish logo → 看 logcat 里 `linker`/`hwservicemanager` 报错，多半是
    `/system`、`/vendor` 没挂上或 A9 基线不对。

## 回退

```sh
# 方法一：TWRP 里刷回备份的 boot
#   /sdcard/droid-boot.img → Install Image → Boot

# 方法二：直接重刷 LineageOS 16.0 zip（会覆盖 boot）

# SFOS 的 rootfs 还在 /data/.stowaways/sailfishos，不影响 LineageOS；
# 想彻底清掉：TWRP → 文件管理器删除该目录，或格式化 data。
```

## 关于 Android 基线（为什么不是 Android 10）

- **“Android 基线”指的是设备侧的 HAL/驱动 ABI，不是 Sailfish OS 的版本。**
  SFOS 5.2 的用户空间与 Android 版本解耦；`droid-hal-dumpling`、`libhybris`、
  `pulseaudio-modules-droid` 等是对着 **LineageOS 16.0（Android 9）的头文件和
  vendor blobs** 编译的，运行时通过 libhybris 调用 `/system`、`/vendor` 里的
  Android 9 库。
- 所以设备上必须真的是 Android 9 的 `/system` + `/vendor`（LOS 16 + OOS 9.0.11 固件）。
  换 Android 10 的 LOS 17，vendor 库的 soname/ABI 变了，libhybris 与 HAL 包会直接
  对不上，installer 也会先 abort（它检查的是 16.0）。
- SFOS 5.2 依然支持 Android 9 基线：`droid-hal-configs` 明确接受
  `android_version_major=9`，OnePlus 6/6T 的社区包至今也是 hybris 16.0 在跑 5.x。
  要上 Android 10+ 属于换基线（内核、blobs、HAL 接口都要重做），是另一个工程。
