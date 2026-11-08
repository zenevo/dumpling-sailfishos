# aarch64 首启 bring-up 记录

最后更新：2026-11-01 12:45

## ✅ 状态：SFOS 已能正常启动（2026-11-01）

用 `out/sailfishos-5.2.0.22-20261101-dumpling-fixed.zip` 在 TWRP 刷入后，
重启成功进入 Sailfish OS。修复核心是两条：

1. `droid-init-done.sh` 用 `systemctl show -p MainPID` 取 PID（原 `pgrep droid-hal-init`
   因 Android init 改名 `init` 而返回空，READY 发不出去）；
2. `droid-hal-init` 的 `Type=simple` drop-in 兜底，保证 `basic.target` 不被阻塞。

包的安装器也修了 TWRP 的 `/system_root` 挂载问题（原版检查 `mount /system` 会报 error 5）。

## USB 开发者模式修复（2026-11-01 下午）

现象：选「开发模式」后自动跳回「仅充电」。

根因（三个叠加）：

1. usb-moded 的 configfs 探测要求 **`/config/usb_gadget/g1/UDC`** 存在
   （源码 `DEFAULT_GADGET_BASE_DIRECTORY=/config/usb_gadget/g1`）。这个 gadget 目录在正常
   Android 上由 `init.usb.rc` 创建，我们的系统没有 → 探测失败 → 回退充电。
2. 内核 RNDIS 网卡默认叫 **`usb0`**，但 `/etc/usb-moded/dyn-modes/developer_mode-android.ini`
   里写死 `network_interface = rndis0` → 匹配不上。
3. connman 防火墙 `connman-INPUT` 只放行 wlan0 的 22 端口，usb0 被 DROP。

修复：

```sh
# 1) 开机创建 gadget 骨架（/etc/systemd/system/usb-gadget-skeleton.service，已 enable）
#    等 /config 挂载后 mkdir /config/usb_gadget/g1/{strings/0x409,configs/c.1/strings/0x409,functions}

# 2) 网卡名统一成 usb0
sed -i 's/network_interface = rndis0/network_interface = usb0/' \
  /etc/usb-moded/dyn-modes/developer_mode-android.ini \
  /etc/usb-moded/dyn-modes/connection_sharing.ini

# 3) 防火墙放行 usb0（/etc/systemd/system/usb0-firewall.{service,timer}，timer 每 60s 重刷）
iptables -I connman-INPUT -i usb0 -p tcp --dport 22 -j ACCEPT
iptables -I connman-INPUT -i usb0 -p icmp --icmp-type 8 -j ACCEPT
```

验证：手机枚举为 `18d1:0a02`（RNDIS），主机侧网卡拿到 `192.168.2.13/24`，
手机 `usb0=192.168.2.15/24`，`ping` 1ms，`ssh defaultuser@192.168.2.15` 成功。

注意：
- 主机如果装了 Meta/Clash 透明代理，会把 `192.168.2.0/24` 劫持进 TUN，需要加
  `ip rule add to 192.168.2.0/24 lookup main priority 50`（主机侧，重启失效）。
- `/etc/connman/main.conf` 里 `FallbackDeviceTypes = rndis0:gadget` 仍写 rndis0，
  目前不影响使用；以后要规范可以改成 `usb0:gadget`。

## 刷机后已做（2026-11-01 下午）

- **DNS 修复**：systemd-resolved 在这个移植上是坏的：
  `systemd-resolve www.baidu.com` 报 "All attempts to contact name servers or networks failed"，
  但把 `/etc/resolv.conf` 换成静态文件、让 glibc 直连 DNS 就正常。
  最终用静态 `/etc/resolv.conf`：
  ```
  nameserver 192.168.8.1
  nameserver 223.5.5.5
  nameserver 119.29.29.29
  search lan
  ```
  验证：`getent hosts www.baidu.com` 成功，`HTTP/1.0 200 OK`（baidu）。
  注意：静态 resolv.conf 会失去 mDNS（`.local`），`search lan` 已保留。
- **浏览器打不开网页（IPv6 优先）**：手机只有 link-local IPv6、没有全局 IPv6，但 DNS 会返回
  AAAA 记录，Sailfish 浏览器优先走 IPv6 → 连不上。修复：`/etc/gai.conf` 里加
  `precedence ::ffff:0:0/96 100`，让 getaddrinfo 优先返回 IPv4。
  验证：`getent ahosts www.baidu.com` 第一条是 `110.242.x.x`（IPv4）。
- **已知缺失**：rootfs 里没有 `ip` 命令（iproute2 未装），排查网络时只能用 `/proc/net/route` 等。
- **内置浏览器黑屏/闪烁（已解决）**：Gecko WebRender 拿不到 GPU 的 EGLImage
  （日志 `No WebRender EGLImage available for browser frame grab`）。
  最终方案：**GPU WebRender + 禁用原生合成器**，写在系统模板
  `/usr/share/sailfish-browser/data/prefs.js`（已打进包，新 profile 自动生效）：
  ```
  user_pref("gfx.webrender.software", false);
  user_pref("gfx.webrender.all", false);
  user_pref("layers.acceleration.disabled", false);
  user_pref("gfx.webrender.compositor", false);
  user_pref("gfx.webrender.partial-present", true);
  user_pref("layers.omtp.enabled", true);
  user_pref("general.smoothScroll", false);
  ```
  关键项是 `gfx.webrender.compositor=false`（原生合成器在这套 libhybris 上不可用）；
  `partial-present` + `layers.omtp` 打开以改善滚动流畅度，`general.smoothScroll=false`
  减少滚动动画开销。软件渲染能出画面但滚动会闪，已弃用。改完需完全关闭浏览器再开。
- **软件源**：6 个源全部刷新成功（adaptation-common / adaptation-community-common /
  apps / hotfixes / jolla / harbour-storeman-obs），无可用系统更新。
- **已安装应用**（`/home/defaultuser/*.rpm` → zypper/rpm）：
  - `harbour-storeman-installer-2.3.0`（Storeman 应用商店）
  - `harbour-wave-0.2.8`（Spotify 客户端）
  - `harbour-xhvpn-1.6.5`（XhVPN；依赖的 `Xray` 仓库里没有，手动从 GitHub
    下载 Xray-core 26.10.31 aarch64，装到 `/usr/bin/Xray/xray`，
    geoip.dat/geosite.dat 同目录，再用 `rpm -Uvh --nodeps` 安装）
- **root 访问方式**：`ssh defaultuser@<ip>` 后用
  `echo <密码> | devel-su -c "单条命令"` 只能跑**无参数**命令；
  带参数要用交互式：
  ```sh
  printf '<密码>\n<命令>\nexit\n' | ssh -tt defaultuser@<ip> 'devel-su'
  ```

## 重大进展：SFOS 已经成功启动过

2026-11-01 上午，SFOS 5.2.0.22 **成功启动到首次启动向导**，用户完成设定并正常使用过
（联网、开发者模式、SSH 都跑起来了）。说明内核、图形栈（lipstick/Wayland/hwcomposer）、
aarch64 用户态都是通的。

## 当前问题：第二次及以后启动卡死 / 重启循环

### 已确认的根因链（2026-11-01 下午定位）

1. `droid-hal-init.service` 是 `Type=notify`，且 `Before=basic.target`。
2. 它**没有发送 READY**（日志里只有 `Starting droid-hal-init...`，没有 `Started droid-hal-init.`）。
3. 因此 `basic.target` 永远不到达 → `start-user-session.service` → `user@100000.service`
   → lipstick 全部起不来。
4. 设备在约 15–20 秒后被重启（DSME 启动超时 / 其他机制），表现成"循环一加 logo"。

**READY 发不出去的直接原因（2026-11-01 12:15 定位）**：

```
systemd-notify: Failed to parse PID .
droid-hal-init: Service 'droid_init_done' (pid 3731) exited with status 1
```

`/usr/bin/droid/droid-init-done.sh` 用 `DROID_PID=$(pgrep droid-hal-init)` 找 PID，
但 **Android init 启动后会把进程名改成 `init`**（prctl），`pgrep droid-hal-init` 返回空 →
`systemd-notify --pid=` 解析失败 → READY 永远发不出去。
第一次开机能成功是**时序侥幸**（改名之前 pgrep 恰好抓到了）。

**修复**（已打进下面的完整包）：

```sh
# /usr/bin/droid/droid-init-done.sh
DROID_PID=$(systemctl show -p MainPID --value droid-hal-init.service 2>/dev/null)
if [ -z "$DROID_PID" ] || [ "$DROID_PID" = "0" ]; then
    DROID_PID=$(pgrep -f droid-hal-init | head -n1)
fi
```

另外加了 `Type=simple` 的兜底 drop-in（即使 READY 失败也不阻塞 `basic.target`）：

```ini
# /etc/systemd/system/droid-hal-init.service.d/override.conf
[Service]
Type=simple
TimeoutStartSec=0
```

### 为什么第一次能成功？

第一次是**刚格式化 /data 后的首次启动**，走的是 Startup Wizard（SUW）路径；
那次日志里确实有 `Reached target Basic System` 和 `Started droid-hal-init.`。
完成向导后，后续启动走普通会话路径，而 droid-hal-init 在这条路径上不再 READY。

### 已排除的假设

| 假设 | 结论 / 证据 |
|---|---|
| DSME 启动超时重启 | 屏蔽 `dsme.service` 后仍循环（约 15 秒） |
| QCOM 硬件看门狗 | 内核日志无 `bark`/`alive mask`；`watchdog_v2.enable=0 nowatchdog` 后仍循环 |
| 内核 panic | 11 次启动的日志里 **0 次** `Kernel panic` |
| `systemd.log_level=debug` 拖慢 | 去掉 debug 后仍循环 |
| 内核/initramfs 不执行 | mark 5/6（mount /data、bind /target）都写入且 rc=0 |

### 完整刷机包（2026-11-01 20:40 最终版）

现在有两个包（内容相同，App 部分不同）：

| 包 | 大小 | md5 | 说明 |
|---|---|---|---|
| `sailfishos-5.2.0.22-20261101-dumpling-fixed.zip` | 710 MB | `c99501a2b0b5eab3bab63a87594aee7d` | 清版，不含 App |
| `sailfishos-5.2.0.22-20261101-dumpling-fixed-apps.zip` | 728 MB | `138ff21984740a272c80bfc58967eaa6` | 带 Storeman + XhVPN（自用） |

App 版的做法：zip 里带 `apps/` 目录（15 个 rpm + Xray 的 xray/geoip/geosite），
安装器在解压 rootfs 后 `mount --bind /dev`、`chroot $ROOT rpm -Uvh --nodeps` 安装，
再把 Xray 放到 `/usr/bin/Xray/`。依赖已从 Jolla 仓库补齐（libsailfishsecrets 一族、
libsailfishcrypto、sqlcipher、qt5-plugin-sqldriver-sqlcipher 等）。

`out/sailfishos-5.2.0.22-20261101-dumpling-fixed.zip`（710 MB），TWRP 直接刷。

包内：
- `sfos-rootfs.tar.gz`：打好全部补丁的 rootfs
  - `defaultuser`（uid/gid 100000）已预建，home 已从 skel 复制
  - `droid-init-done.sh` 已修（用 `systemctl show -p MainPID` 取 PID）
  - `droid-hal-init` 的 `Type=simple` drop-in
  - `/init-debug` 已删
  - **USB 修复**：`usb-gadget-skeleton.service`（开机建 gadget 骨架，已 enable）、
    `/etc/usb-moded/dyn-modes/*.ini` 的 `network_interface = usb0`、
    `usb0-firewall.service` + `usb0-firewall.timer`（放行 usb0 的 SSH/ICMP，已 enable）
  - **DNS**：静态 `/etc/resolv.conf`（`192.168.8.1` + `223.5.5.5` + `119.29.29.29` + `search lan`）。
    原因：这个移植的 systemd-resolved 是坏的（stub 查询失败，但 glibc 直连 DNS 正常）
  - `/boot/hybris-boot.img` = `/tmp/boot-nodebug.img`（含 kmsg 日志钩子）
- `META-INF/com/google/android/update-binary`：改过的 hybris 安装器
  - 用 `tar -xz` 解 gzip（比原版 bzip2 快）
  - 检查/挂载 `/system_root`（TWRP 的系统分区位置，修复 error 5）
  - 自动 `sed s/fileencryption/encryptable/` 改 vendor fstab
  - 刷完 boot 后自动清 `misc`（BCB）
  - 去掉了"需二次确认重装"的拦截（一次刷完）

**不含**：Storeman / Wave / XhVPN 等 App（按用户要求不置入）。

刷机步骤（TWRP）：
1. 关机 → **音量上 + 电源** 进 TWRP；
2. **Install** → 选 `/sdcard/sailfishos-5.2.0.22-20261101-dumpling-fixed.zip`；
3. 等安装器完成（解压 1.5 GB，约 2–3 分钟）；
4. **Reboot → System**；
5. 首次启动会进 Sailfish 向导。

前置条件（安装器会自己检查）：
- `/data`、`/cache` 是 ext4
- `/system_root` 上是 LineageOS 16.0（`lineage.boot.version=16.0`）
- 有 vendor 分区

刷完后 USB 验证：
```sh
# 手机开发者模式打开、插 USB 线后，主机应看到 18d1:0a02 和 RNDIS 网卡
ip -br addr | grep -i enp     # 拿到 192.168.2.x
ssh defaultuser@192.168.2.15  # 密码 002124
```

**注意（2026-11-01 12:30 遇到的 error 5）**：这台机器的 TWRP 把系统分区挂在
`/system_root`，而 `/system` 是 TWRP 自己的目录。原版安装器检查 `mount /system` 会失败
（报 `E5`）。安装器已改成检查/挂载 `/system_root`，并用 `/system_root/build.prop`
做 LOS16 版本检查。若以后重刷 ROM 后 `/system_root` 未挂载，安装器会自己
`mount -t ext4 -o ro /dev/block/bootdevice/by-name/system /system_root`。

前置条件（安装器会自己检查）：
- `/data`、`/cache` 是 ext4
- `/system` 是 LineageOS 16.0（`lineage.build.version=16.0`）
- 有 vendor 分区

### 打包方法（可复现）

```sh
# 1) 解压原始 rootfs
mkdir -p /home/zen/sfos-pkg/rootfs
sudo tar -xf /tmp/rootfs.tar -C /home/zen/sfos-pkg/rootfs

# 2) 打补丁（见 /tmp/patch_rootfs.py 的完整逻辑）
#    - 删 init-debug
#    - 改 droid-init-done.sh
#    - 建 defaultuser + home
#    - 换 boot/hybris-boot.img
#    - 写 droid-hal-init drop-in

# 3) 打包（gzip 比 bzip2 快很多）
cd /home/zen/sfos-pkg
sudo tar -C rootfs -cf - . | gzip -6 > sfos-rootfs.tar.gz

# 4) 改 update-binary（-xj → -xz、加 misc 清零、去掉二次确认）

# 5) 用 python zipfile 组包（系统没装 zip 命令）
#    sfos-rootfs.tar.gz 用 ZIP_STORED，META-INF 用 DEFLATED

# 6) 拷到交付目录
cp sailfishos-5.2.0.22-20261101-dumpling-fixed.zip \
   /home/zen/文档/deepseek-harness/default-workspace/dumpling-5.2.0.22/out/
```

## 设备当前状态

| 项目 | 状态 |
|---|---|
| boot 分区 | SFOS 调试镜像（`/tmp/boot-nodebug.img`，无 debug、无 pstore2、带 kmsg 日志钩子） |
| `/data` | 已格式化为明文 ext4（FBE 已清） |
| SFOS rootfs | `/data/.stowaways/sailfishos`，已重新解压并修复 |
| `defaultuser` | uid/gid 100000，已创建 |
| `startup-misc.service` | 已禁用 |
| `/vendor/etc/fstab.qcom` | userdata 已改为 `encryptable=ice,quota` |
| `/init-debug` | 已删除 |
| DSME | 已恢复（取消 mask） |
| droid-hal-init | 已加 `Type=simple` drop-in（待验证） |

## 关键脚本与命令（都在本机执行/记录）

### 1. 从 rootfs tar 提取并修复（TWRP adb shell）

```sh
# 推入并解压（rootfs.tar 约 1.6 GB，在 /tmp/rootfs.tar）
adb push /tmp/rootfs.tar /data/rootfs.tar
adb shell 'R=/data/.stowaways/sailfishos; rm -rf $R; mkdir -p $R; toybox tar -xf /data/rootfs.tar -C $R'

# 修复
adb shell 'R=/data/.stowaways/sailfishos
rm -f $R/init-debug $R/init_enter_debug $R/init_enter_debug2
rm -f $R/etc/systemd/system/multi-user.target.wants/startup-misc.service
mknod -m 666 $R/dev/null c 1 3
chroot $R /usr/sbin/groupadd -g 100000 defaultuser
chroot $R /usr/sbin/useradd -u 100000 -g 100000 \
  -G users,privileged,input,audio,video,bluetooth,inet \
  -m -d /home/defaultuser -s /bin/bash defaultuser'
```

### 2. 格式化 /data（清 FBE 加密）

```sh
adb shell 'umount /sdcard; umount /data
mke2fs -t ext4 -F /dev/block/bootdevice/by-name/userdata
mount -t ext4 /dev/block/bootdevice/by-name/userdata /data'
```

### 3. vendor fstab 关闭强制加密

```sh
adb shell 'mount -t ext4 -o rw /dev/block/bootdevice/by-name/vendor /vendor
sed -i "s/fileencryption/encryptable/" /vendor/etc/fstab.qcom
grep userdata /vendor/etc/fstab.qcom
umount /vendor'
```

### 4. 构建 boot 镜像（主机）

```sh
# 重新生成 ramdisk（init-script 改动后必须删中间产物，否则不会重建！）
rm -f ~/hadk/out/target/product/dumpling/obj/ROOT/hybris-boot_intermediates/boot-initramfs.gz
sudo /srv/mer/sdks/sfossdk/sdk-chroot ubu-chroot -r /srv/mer/sdks/ubu /home/zen/hadk/build-boot.sh

# 用旧内核 + 新 ramdisk 合成（避免 fbcon 新内核的问题）
unpack_bootimg --boot_img ~/hadk/out/target/product/dumpling/hybris-boot.img --out /tmp/hbX
mkbootimg --kernel /tmp/hb7/kernel --ramdisk /tmp/hbX/ramdisk \
  --cmdline "selinux=0 audit=0 androidboot.hardware=qcom user_debug=31 ... loop.max_part=7 systemd.log_target=kmsg" \
  --pagesize 4096 --kernel_offset 0x00008000 --ramdisk_offset 0x01000000 \
  --second_offset 0x00f00000 --tags_offset 0x00000100 --output /tmp/boot-test.img
```

### 5. 刷入（一定用 fastboot，不要用 TWRP 的 dd）

```sh
adb reboot bootloader
fastboot flash boot /tmp/boot-test.img
fastboot erase misc          # 清 BCB，否则容易被带回 recovery
fastboot reboot
```

### 6. 读日志（TWRP）

```sh
adb pull /data/.stowaways/sailfishos/boot/kmsg.log /tmp/k.log
tr -d '\000' < /tmp/k.log > /tmp/k
# 找最后一次启动
grep -an "Booting Linux on physical CPU" /tmp/k | tail -1
# 看这次启动的时间戳（µs）和结尾
sed -n 'START,$p' /tmp/k | tail -50
```

## initramfs 里的改动（`~/hadk/hybris/hybris-boot/init-script`）

- 加了 `mark()` 阶段标记：写 `/mark.txt` 再 `dd` 到 `/dev/sde19` 的 32 MB+ 偏移
  （mark 5 = mount /data，mark 6 = bind /target）。
- 加了 kmsg 日志钩子：`mount_stowaways` 之后
  `( exec >> /target/boot/kmsg.log 2>&1; exec /bin/busybox cat /dev/kmsg ) &`
- **已删除** pstore2 调试块（它会在第二次启动时强制写 BCB 重启进 TWRP，掩盖真实问题）。
- 去掉了 `switch_root ... &>` 重定向（busybox ash 对 `&>` 支持不好）。
- 切根前增加 `umount /config /sys /dev/pts /dev /proc`。

## 已知问题：视频录像卡死（2026-11-01 排查）

现象：录像时移动手机画面很卡；停止录像后相机 App 无响应，视频文件损坏（`moov atom not found`）。

排查结论：

- 与分辨率无关：4K（默认 `3840x2160`）和 1080p 都一样；
- 与音频无关：去掉相机麦克风权限（静音录像）后仍然卡死；
- 相机 App 卡在 `futex_wait_queue_me`（等管线结束）；录制文件没有 moov atom，说明
  **管线没有正常收尾/EOS**；
- 栈：QtMultimedia 5.6.2 `gstcamerabin` + `libgstdroid.so`（gst-droid）+ `droidmedia-0.20260902.0`
  + Venus 硬件编码器（固件已加载）。

**GST_DEBUG=3 深挖结果（2026-11-01 21:35）**：

- `droidcamsrc` 报 `Unknown HAL color format 0x7f000789`（相机 HAL 返回的实现相关格式，
  gst-droid 不认识）；
- `gstaggregator: <muxer> Latency query failed`；
- `aacparse1:sink` 收到的是 `audio/x-raw` 而不是 `audio/mpeg`（音频分支 caps 没接对）；
- 手动 `gst-launch` 测试 `droidcamsrc ! droidvenc` 直接报 `stream stopped, reason not-linked`
  （两者 buffer 内存类型不匹配：`DroidMediaQueueBuffer` vs `DroidVideoMetaData`），
  说明录像链在 gst-droid 层面就接不起来，靠 Qt camerabin 内部才勉强工作。

已排除：分辨率（4K/1080p）、麦克风（关掉仍卡）、audioCodec 置空仍卡。

结论：这是 gst-droid/droidmedia 录像管线（buffer 类型 + EOS/收尾）的移植层 bug，
需要改代码（打 gst-droid 补丁或换编码路径），不是配置能修的。

**2026-11-02 继续深挖（logcat + dmesg）**：

- 录制中文件一直 **0 字节**：编码器完全没有输出；
- logcat 关键序列：
  ```
  DroidMediaCodec: Creating video encoder for video/avc
  OMXMaster: makeComponentInstance(OMX.qcom.video.encoder.avc)   ← Venus 硬件编码器
  ACodec: setupVideoEncoder succeeded
  MediaCodecSource (video) started
  CameraSource: Waiting on an available memory base timed out. Dropping a recording frame.  ← 之后全是这个
  ```
- `CameraSource` 的 memory base 被编码器占着不归还 → 每帧都丢 → `MPEG4Writer`
  等不到帧 → 停止时 App 死锁（futex）。
- 已排除：分辨率（QML `_pickResolution` 永远选最大 4K，已改成固定 1920x1080，
  HAL 流确认变成 1080p，仍卡）、音频（QML 去掉 audio 轨，只剩视频编码器，仍卡）、
  麦克风权限。
- 内核 dmesg 无 Venus/msm_vidc 实时报错（启动时 `cce0000.qcom,venus: invalid resource /
  Failed to iomap base register` 是旧 venus 节点，实际编码走 `cc00000.qcom,vidc`）。
- 解码器/摄像头工作正常，只有 **Venus 编码器不消费帧**。

**2026-11-02 已修复！** 根因与修法：

1. 用 NDK `AMediaCodec` 写了个 `venc-test` 直接调 `OMX.qcom.video.encoder.avc`，
   30 帧全部出码 → **Venus 编码器本身没问题**；
2. 对比发现：我们 Android 树里本来就有两个 hybris 补丁，**系统自带的 `libstagefright`
   没有这些补丁**：
   - `frameworks/av/media/libstagefright/ACodec.cpp`：修复 64 位相机进程给 32 位 OMX
     传 native handle buffer 时的大小不匹配（`VideoNativeMetadata_ptrSized`），
     这正是录像全丢帧的根因；
   - `services/camera/libcameraservice/CameraService.cpp`：禁用快门音（避免崩溃）；
3. 用树里编出的 `libstagefright.so` 替换 `/system/lib64/libstagefright.so` 后，
   录像**正常**：文件带 moov、可解码、1080p/30fps、无丢帧。

**打包方式**：两个 zip 都内置 `system-libs/libstagefright.so` 和
`system-libs/libcameraservice.so`，安装器在解压 rootfs 后
`mount -o remount,rw /system_root` → 覆盖写入 → `remount,ro`。

**仍未解决**：视频播放（相册里所有视频，包括自带的都放不了）。换回系统原版
`libstagefright` 也一样，说明是独立的移植问题。日志线索：
`ACodec: [OMX.qcom.video.decoder.avc] does not support color format -705811232`
（64/32 位参数被截断的典型现象）、
`BufferQueueProducer: [DroidMediaCodecBufferQueue] connect: already connected`，
之后相册/播放器卡死。属于解码器输出 surface/buffer 队列问题，待后续处理。

**播放问题进一步排查（2026-11-02 21:50）**：
- 用户音频正常（`paplay` 播放 wav 成功；用户会话 pulseaudio 在跑，sink 有 `sink.primary_output` 等）；
- 系统级 `pulseaudio.service` 是 inactive 且 `RefuseManualStart=yes`（WantedBy=emergency-call.target），
  但用户会话的 pulseaudio 正常，SFOS 应用音频没问题；
- 相册依赖 `qt5-qtdeclarative-import-multimedia` + `sailfish-components-media-qt5`，
  播放走 QtMultimedia → GStreamer → `libgstdroid.so`（droidvdec/droideglsink/droidvideotexturesink）；
- 日志有 `libEGL: call to OpenGL ES API with no current context`，怀疑是视频纹理 sink 的 EGL 上下文问题；
- Android 侧 NuPlayer 报 `openAudioSink failed status: -19`（那是 Android audioserver 路径，和 SFOS 应用播放器不是同一条链路）。

结论：播放是独立的 QtMultimedia/GStreamer 视频渲染问题，和录像修复无关，需要后续专门排查。

**2026-11-02 22:30–23:00 继续排查**：

- 相册运行在 **sailjail/firejail 沙箱**里（profile 含 Base/Audio/MediaIndexing/UserDirs 等权限）；
- 症状：点播放后进度条不动、无画面无声音，退回相册卡死；
- 试过 `QSG_RENDER_LOOP=basic`（写进 `/var/lib/environment/nemo/60-multimedia.conf` 后重启）——**无效**，已还原；
- 尝试用 SDK + mb2 重编 gst-droid 做源码级调试：`mb2 -t oneplus-dumpling-aarch64 -s gst-droid.spec build`
  失败于缺 BuildRequires（target 里没有 GStreamer devel 包、libexif devel、
  nemo-gstreamer-interfaces devel，以及我们的 `droidmedia-devel`）。要重编需要先把这些
  devel 包装进 SDK target（droidmedia-devel 还得先为 SDK target 编一版）；
- 下一步建议：
  1. 给 SDK target 装齐 devel 依赖 + 编 droidmedia-devel，再用 mb2 编带日志的 gst-droid；
  2. 或者像修录像那样，给 `libstagefright`/droidmedia 的解码路径加日志，确认解码器是否出帧；
  3. 也检查相册 sailjail profile 是否限制了 GPU/dri 或 hybris 媒体库访问。

**SDK 构建 gst-droid 的阻塞点（2026-11-02 22:10）**：

- SDK target `oneplus-dumpling-aarch64` 存在，mb2 可用；
- `zypper --root` 在 SDK chroot 里按**宿主 i486 架构**选包，
  所以 `gstreamer1.0-devel` / `droidmedia-devel` 解析出来都是 i486，
  aarch64 的 BuildRequires 永远满足不了；
- `droidmedia-devel` 在 adaptation-common 里是 `0.20260522.0`，
  而 gst-droid spec 要求 `>= 0.20260902.0`（我们设备上跑的是后者），
  需要放宽 spec 或为 target 编一版 droidmedia-devel；
- adaptation-common 的 ssu URL 解析只对 i486 生效（refresh 时没报错但没有 aarch64 候选）。

解决思路（下次做）：用 `rpm --root --ignorearch --nodeps` 直接装 aarch64 devel RPM，
或给 target 的 zypper 指定架构（`--force-architecture` 不够，需要 zypp.conf 的 arch 生效），
并把 spec 的 droidmedia 版本要求改成 `>= 0.20260522.0`。

**2026-11-02 23:00 gst-droid 构建环境已打通（重要）**：

- 从 Jolla 仓库下载 aarch64 devel RPM（gstreamer1.0-devel / plugins-base-devel /
  plugins-bad-devel / libexif-devel / nemo-gstreamer1.0-interfaces-devel / elfutils-devel /
  xz-devel / orc-devel），用
  `rpm --root /srv/mer/targets/oneplus-dumpling-aarch64 --install --ignorearch --nodeps`
  装进 SDK target；
- 因为 SDK 的 zypper 按宿主 i486 选包，直接 zypper 装不上 aarch64 版本；
- droidmedia 的 devel 环境手工造：头文件放 target 的 `/usr/include/droidmedia/`，
  自写 `/usr/lib64/pkgconfig/droidmedia.pc`（Version 0.20260902.0），
  `libdroidmedia.so` 放 target 的 `/usr/lib64/`；
- 构建要用 `mb2 build-init` + `rpm/` 目录放 spec 和 tarball；
  mb2 会从 `.default` 快照重置 target，所以改完 target 要**同步改快照**；
- spec 需要小改：`%build`/`%install` 加 `cd %{name}-%{version}`、
  去掉 `BuildRequires: pkgconfig(droidmedia)`、去掉 `%license COPYING`；
- 产物：`/home/zen/gstdroid-build/RPMS/gstreamer1.0-droid-1.0.0-1.aarch64.rpm`。

**关键的兼容性发现**：

- 手机上的 `libdroidmedia.so` 只导出 `_droid_media_init`，而 gst-droid 源码调用
  `droid_media_init()` → 我们编的插件加载失败（相机退回 videotestsrc，彩条+雪花）；
  Jolla 官方插件里没有这个符号。**解法：编译前把 plugin.c 里那段 `droid_media_init()` 调用删掉**；
- 播放修复来自 gst-droid master 的 droidvdec 改动（NV12 semi-planar 段错误修复，
  commit `f06bd029`，用到 `droid_media_codec_get_output_info()`，需要 Sep 版 droidmedia 头文件）；
- 相机彩条在多次 kill 相机进程后，即使换回官方插件也会出现，**重启后恢复**——
  所以之前"我们的插件导致彩条"的结论被污染了。

**待验证的最终修复版**（已编译，在 `/home/zen/gstdroid-build/RPMS/`，
对应源码 = May 基线 + `f06bd029` NV12 修复 + 删除 `droid_media_init` 调用）：
装到手机 `/usr/lib64/gstreamer-1.0/libgstdroid.so` + `/usr/lib64/libgstdroid-1.0.so.1.0.0`，
重启后应同时得到"相机正常 + 播放正常"。

**官方 gst-droid 原始库**在 `/home/zen/sfos-pkg/rootfs/usr/lib64/`（md5
`619b753f7ff2d1be190f0501d6c3b573`），可用于回滚。

**持久工作目录：`/home/zen/sfos-pkg/gstdroid-work/`**（主机 /tmp 是 tmpfs，会被清空，
所以源码、补丁、RPM、构建步骤都放在这里，详见其中的 `README.md`）。

**2026-11-02 深夜最终结论（播放修复实验）**：

- `droid_media_init()` 调用**不能删**：删掉后插件加载时段错误（`gst-inspect` 里 rc=139），
  相机退回 videotestsrc（彩条+雪花）、播放也失败；
- 手机上必须装**带 `droid_media_init` 符号**的 `libdroidmedia.so`
  （我们编的版本同时导出 `_droid_media_init` 和 `droid_media_init`，已装在手机上）；
- 在这个前提下，用「May 基线源码 + NV12 解码补丁（回移成不依赖 Sep 的
  `droid_media_codec_get_output_info`）+ May 头文件」构建的插件：
  **播放正常**，但**相机预览仍不正常**；
- 试过 Sep 头文件 / May 头文件 / 去 init / 留 init 各种组合，**只有 Jolla 官方插件
  （rootfs 里的原始二进制）相机正常**。我们构建的插件相机路径为何失败还没定位，
  下一步需要用 gdb/valgrind 对比两个插件的 droidcamsrc 初始化，或搭真正的 OBS 构建环境。

**USB 不稳定问题（已修）**：

- 现象：USB gadget 周期性重枚举（dmesg 里 device 号不断增大），RNDIS 断开，主机 IP 丢失；
- 原因：`/etc/usb-moded/80-use-ask-mode.ini` 把 USB 模式设为 `ask`，锁屏/插拔会重新询问；
- 修法：`/etc/usb-moded/70-use-developer-mode.ini` 写入 `[usbmode] mode=developer_mode`，
  并把 `80-use-ask-mode.ini` 改名为 `.disabled`，重启后 USB 稳定。

**2026-11-03 22:40 相机预览根因找到！**

GST_DEBUG 日志里出现：

```
WARN GST_ELEMENT_FACTORY: no such element factory "droideglsink"!
...
<preview-appsrc>, <fake-auto-video-src>   ← Qt camerabin 退回了测试源
```

也就是说：相机 App 里 **gst-droid 插件根本没加载**，camerabin 只能用
`fake-auto-video-src`（彩条）。对比两个插件的动态依赖：

| 插件 | 直接依赖 `libdroidmedia.so` |
|---|---|
| Jolla 官方 | **否**（只通过 `libgstdroid-1.0.so` 间接依赖） |
| 我们构建的 | **是**（plugin.c 的 `droid_media_init()` 和 NV12 补丁里的 `droid_media_codec_get_output_info()` 调用） |

相机 App 的沙箱环境里 `LD_LIBRARY_PATH` 不含 `/usr/libexec/droid-hybris/system/lib64`，
所以**带直接依赖的插件在相机进程里加载失败**；相册进程环境不同，播放才正常。

**修法（已编译，待验证）**：给插件加
`-Wl,-rpath,/usr/libexec/droid-hybris/system/lib64`
（meson.build 里 `add_project_link_arguments`），同时去掉 plugin.c 的 `droid_media_init()` 调用。
产物 md5 `8ac28f20388bc338a8702c9aca4102cb`，在 `/home/zen/sfos-pkg/gstdroid-hybrid/`，
装到手机 `/usr/lib64/gstreamer-1.0/libgstdroid.so` + `/usr/lib64/libgstdroid-1.0.so.1.0.0`
重启即可验证。

**2026-11-03 22:50 修复已打进刷机包**：

两个 zip 的 rootfs 里已内置：
- `/usr/lib64/gstreamer-1.0/libgstdroid.so`（带 RPATH 的修复版，md5 `8ac28f20…`）
- `/usr/lib64/libgstdroid-1.0.so.1.0.0`
- `/usr/libexec/droid-hybris/system/lib64/libdroidmedia.so`（带 `droid_media_init` 别名）

新包 md5：
- 清版 `600972f306f1f4fc8a84313ea4d2803d`
- App 版 `06d70c260478008900b5231bb53d63a8`

刷入后应同时得到：录像正常 + 播放正常 + 相机预览正常（待重刷验证）。

**2026-11-04 最终结论（播放/相机兼容性根源）**：

- 我们通过 SDK/mb2 构建的 `libdroidmedia.so` 和 gst-droid **链接的是 Android（bionic）库**
  （`libcamera_client.so`、`android.hidl.*` 等），这些库在 SFOS 的 glibc 进程里加载不了
  （ldconfig 不索引带 `@` 的 HIDL 库名，sailjail/firejail 又会剥掉 `LD_LIBRARY_PATH`）；
- 结果：我们构建的插件在相机 App 进程里加载失败 → camerabin 退回 `fake-auto-video-src`（彩条）；
- Jolla/droid-hal 构建的官方插件（md5 `619b753f…`）是经过 hybris 处理的，能正常加载 → 相机正常；
- **结论：要同时得到相机+播放，必须在 droid-hal（Android 树 + hybris）里编 gst-droid，而不是 SDK/mb2 路径。**

**最终稳定组合（2026-11-05 修正）**：
- **hybris 方式构建的 gst-droid（md5 `d62157a6…`）** + 原始 droid-hal libdroidmedia
  + libstagefright/libcameraservice 补丁；
- 相机预览/拍照/录像 ✅ **播放也 ✅**（之前「播放不行」的结论已过时——
  用 hybris shim 构建后两者都正常，实测确认）。

**新包（2026-11-04）**：
- 清版 `sailfishos-5.2.0.22-20261104-dumpling-fixed.zip`（745,435,978 字节，md5 `12b35bbc921e04915249de0d174340aa`）
- App 版 `sailfishos-5.2.0.22-20261104-dumpling-fixed-apps.zip`（764,582,736 字节，md5 `d8c446fa478af81c76c587217176295c`）

原始库备份在 `/tmp/old-libs/`（来自 `20261030-dumpling.zip` 的 rootfs，重启会清空，
需要的话及时拷到持久目录）。

**2026-11-04 23:00 正确路线（hybris 方式，已验证全部功能正常）**：

**结论：gst-droid 必须用 hybris shim 方式构建，不能直接链 bionic 库。**

具体做法（在 SDK/mb2 环境里即可，不需要 OBS）：

1. 从手机（或 droidmedia-devel 包）取 `/usr/share/droidmedia/hybris.c`
   （它是 glibc shim：dlopen `libdroidmedia.so` 经 `libhybris-common` 的 Android linker 转发）
2. 把 `hybris.c` 加入：
   - `gst/meson.build` 的 `libgstdroid_sources`
   - `gst-libs/gst/droid/meson.build` 的 `gstdroid_sources`
3. droidmedia 用 **0.20260522.0**（5.2 配套版本）的 headers/.pc
4. 正常 `mb2 build`

产物特征：插件 **NEEDED 里没有 libdroidmedia/libcamera_client 等 bionic 库**，
`droid_media_init` 由插件自己定义（shim），运行时 dlopen Android 库。

**效果**：相机预览 ✅ 拍照 ✅ 录像 ✅ 播放 ✅（全部正常！）

**录制画质**：默认会选 4K（3840x2160），但编码器码率只有 ~11.6 Mbps + Baseline，
运动画面有马赛克。已在 `CaptureView.qml` 的 `_pickResolution()` 里加上限
`maxPixels = 1920 * 1080`，录制降为 1080p，画质明显改善。

**1080p60 尝试（失败）**：在 videoRecorder 加 `frameRate: 60` 后预览黑屏，
说明 gst-droid 的 video profile / HAL 路径不支持 1080p60，已回滚。
QML 备份：`CaptureView.qml.bak.60fps`（60fps 版）、`CaptureView.qml.bak.1080p`（1080p 版）。

**新包（2026-11-05）**：
- 清版 `sailfishos-5.2.0.22-20261105-dumpling-fixed.zip`（745,927,479 字节，md5 `8e044fe004da52d07ee2528f33feb191`）
- App 版 `sailfishos-5.2.0.22-20261105-dumpling-fixed-apps.zip`（765,074,237 字节，md5 `ea52b3cf4aea9ac104886b789b33fc5a`）

**当前手机状态（2026-11-02 23:45）**：
- gst-droid：我们构建的「May + NV12 回移 + 留 init」版（播放正常、相机预览不正常）；
- libdroidmedia：带 `droid_media_init` 别名版；
- 相机想恢复正常：把 rootfs 里的官方 gst-droid 两个库推回去重启即可。

当前默认视频分辨率已改成 1080p（dconf `/apps/jolla-camera/primary|secondary/video/videoResolution`）。

## 已知坑

1. **ramdisk 中间产物缓存**：改了 `init-script` 后如果 `boot-initramfs.gz` 时间戳更新，
   make 不会重建。必须手动 `rm -f .../boot-initramfs.gz`。
2. **TWRP 的 `dd` 写 boot 分区**：多次出现"写了但引导器不认"，统一改用 `fastboot flash boot`。
3. **`misc` BCB**：`fastboot erase misc` 比 `dd` 清 1 MB 更可靠。
4. **FBE 加密**：只要 `/vendor/etc/fstab.qcom` 是 `fileencryption`，/data 就会被加密，
   SFOS 读不了；必须先改 `encryptable` 再格式化 /data。
5. **`/tmp` 在 initramfs 里不存在**：init 脚本里的临时文件要写 `/mark.txt` 这类根路径。
6. **`/init-debug`**：rootfs 里存在时 initramfs 会切到调试 init（大容量存储 + telnet），
   看起来像"卡 logo"。当前 init 里已改成检查 `.disabled` 并删除该文件。

## 下次继续的步骤

1. 验证 `droid-hal-init` 的 `Type=simple` drop-in 是否让 basic.target 通过、
   用户会话是否起来（看屏幕 / 读 kmsg.log）。
2. 如果通了：去掉 `systemd.log_target=kmsg` 之类的调试参数，做一次干净启动验证。
3. 如果没通：查 `droid-init-done.sh` 的触发点（它靠 init rc 里的
   `service droid_init_done ...` 调用），确认为什么 Android init 不发 READY。
4. 之后再做：数据、Wi-Fi、通话、Spotify（Wave）验证。

## 产物

- 构建树：`~/hadk`；SDK：`/srv/mer/sdks/sfossdk`
- 交付仓库：`dumpling-5.2.0.22/`
- 临时镜像：`/tmp/boot-nodebug.img`、`/tmp/boot-nowd.img`、`/tmp/boot-final.img`
  （主机重启会丢，按上面第 4 节可重建）

---

## 12. 源文件与链接清单（本次移植实际用到的）

### gst-droid（相机/播放插件）

| 用途 | 链接 |
|---|---|
| 上游仓库 | https://github.com/sailfishos/gst-droid |
| 实际使用的版本（0.20260928.0 源码，commit `39825736`） | https://github.com/sailfishos/gst-droid/tree/0.20260928.0 |
| 该版本源码 tar | https://github.com/sailfishos/gst-droid/archive/refs/tags/0.20260928.0.tar.gz |
| 官方手机上的版本（0.20260508.1，commit `e5152aee`） | https://github.com/sailfishos/gst-droid/tree/0.20260508.1 |
| NV12 播放修复补丁（commit `f06bd029`） | https://github.com/sailfishos/gst-droid/commit/f06bd029 |

OBS（dumpling 项目，可匿名浏览）：
- 项目页：https://build.sailfishos.org/project/show/nemo:devel:hw:oneplus:dumpling
- 包页：https://build.sailfishos.org/package/show/nemo:devel:hw:oneplus:dumpling/gst-droid
- `_service`（自动拉 git 的配置）：https://build.sailfishos.org/package/view_file/nemo:devel:hw:oneplus:dumpling/gst-droid/_service?expand=1
- spec（含 `droidmedia >= 0.20260902.0` 要求）：https://build.sailfishos.org/package/view_file/nemo:devel:hw:oneplus:dumpling/gst-droid/_service:tar_git:gst-droid.spec?expand=1
- **构建日志（免登录）**：https://build.sailfishos.org/public/build/nemo:devel:hw:oneplus:dumpling/sailfish_latest_armv7hl/armv8el/gst-droid/_log
  （日志里可见 `hybris.c` + `-lhybris-common` 的链接方式，是本次修正构建路线的关键）

### droidmedia（Android 媒体框架封装）

| 用途 | 链接/文件 |
|---|---|
| 上游仓库 | https://github.com/sailfishos/droidmedia |
| **5.2 配套版本 0.20260522.0**（朋友确认） | https://github.com/sailfishos/droidmedia/releases/tag/0.20260522.0 |
| 手机上的 devel 包（含 hybris 构建素材） | `droidmedia-devel-0.20260522.0-1.11.2.jolla.aarch64.rpm`（adaptation-common） |
| 其中关键文件 | `/usr/share/droidmedia/hybris.c`、`/usr/lib64/libdroidmedia.a`、`/usr/include/droidmedia/*.h`、`/usr/lib64/pkgconfig/droidmedia.pc` |
| 本地构建树 | `~/hadk/external/droidmedia`（commit `8b82f5e8`，0.20260902.0） |

### hybris / 启动

- libhybris：https://github.com/libhybris/libhybris
- hybris-boot：https://github.com/mer-hybris/hybris-boot

### Android 基线

- LineageOS 16.0（dumpling）：https://wiki.lineageos.org/devices/dumpling/
- LineageOS 下载：https://download.lineageos.org/
- AOSP frameworks/av（录像修复的补丁基础）：https://android.googlesource.com/platform/frameworks/av/
- TWRP：https://twrp.me/oneplus/oneplus5t.html

### Sailfish OS

- HADK 文档：https://docs.sailfishos.org/Tools/HADK/
- Platform SDK：https://docs.sailfishos.org/Tools/Platform_SDK/
- Sailfish OBS：https://build.sailfishos.org/
- 旧移植（本仓库的前身）：https://github.com/sailfishos-oneplus5

### 应用（App 版包内）

- Storeman：https://github.com/mentaljam/harbour-storeman
- Xray-core：https://github.com/XTLS/Xray-core

### 本机持久化文件（重启不丢）

| 路径 | 内容 |
|---|---|
| `/home/zen/sfos-pkg/rootfs/` | 打包用的 rootfs 暂存目录 |
| `/home/zen/sfos-pkg/gstdroid-hybris/` | **最终可用的 hybris 版 gst-droid**（md5 `d62157a6…`） |
| `/home/zen/sfos-pkg/gstdroid-work/hybris.c` | hybris shim 源码 |
| `/home/zen/sfos-pkg/gstdroid-work/libdroidmedia.a` | 静态 hybris 库 |
| `/home/zen/sfos-pkg/gstdroid-work/obs-gstdroid-build.log` | OBS 构建日志 |
| `/home/zen/sfos-pkg/gstdroid-work/original-libs/` | 原始（droid-hal 版）gst-droid/libdroidmedia 备份 |
| `/home/zen/gstdroid-build/` | mb2 构建目录（当前源码 + spec） |

---

## 13. gst-droid 构建方法（可复用于其他设备）

> 2026-11-06 整理。这一节是给"下一个设备适配"用的：**不需要 OBS 账号**，本地就能构建出
> 相机 + 播放都正常的 gst-droid。

### 13.1 核心结论

gst-droid 插件**不能直接链接 bionic 的 `libdroidmedia.so`**（Android 库），否则在 SFOS 的
glibc 进程里加载失败（相机 App 里表现为 camerabin 退回 `fake-auto-video-src` → 彩条，
播放表现为崩溃）。

正确做法：**把 droidmedia-devel 提供的 `hybris.c` 编进插件和基库**。
`hybris.c` 是 glibc 侧的 shim，通过 `dlopen` 加载 Android 媒体库并转发 `droid_media_*` 调用。
产物特征：**NEEDED 里没有 `libdroidmedia.so`/`libcamera_client.so`/`android.hidl.*`**。

### 13.2 需要的材料（都不需要 OBS 账号）

| 材料 | 获取方式 |
|---|---|
| gst-droid 源码 | GitHub tag（如 `0.20260928.0`，commit `39825736`）`https://github.com/sailfishos/gst-droid` |
| **droidmedia-devel（与 SFOS 版本配套）** | 在**手机上**用 zypper 装（手机带设备 SSU 凭据，能访问 adaptation-common 仓库）：`zypper install droidmedia-devel` |
| `hybris.c` | 上面的 droidmedia-devel 包里：`/usr/share/droidmedia/hybris.c` |
| 头文件 + `.pc` | 同上：`/usr/include/droidmedia/*.h`、`/usr/lib64/pkgconfig/droidmedia.pc` |
| 静态库 | `/usr/lib64/libdroidmedia.a`（只含 `hybris.c.o`，即 hybris shim） |

> 5.2.0.22 对应的 droidmedia-devel 是 **0.20260522.0**（朋友提示，实测可用）。

### 13.3 构建步骤（SDK/mb2）

1. 准备工作目录：
   ```sh
   cd ~/gstdroid-build
   # 放 gst-droid 源码为 gstreamer1.0-droid-1.0.0/（从 git tag 解压）
   # 放 spec 到 rpm/gst-droid.spec（从源码带的 rpm/ 目录，改两处：删 droidmedia 的 BuildRequires、%build/%install 加 cd %{name}-%{version}）
   ```
2. 把 `hybris.c` 加进两个 meson 构建：
   - `gst/meson.build` 的 `libgstdroid_sources` 加 `'hybris.c'`
   - `gst-libs/gst/droid/meson.build` 的 `gstdroid_sources` 加 `'hybris.c'`
   - 把 `hybris.c` 复制到这两个目录
3. 把 droidmedia-devel 的文件放进 SDK target：
   ```sh
   T=/srv/mer/targets/<你的target>
   sudo cp -a /path/to/droidmedia-devel/usr/include/droidmedia/* $T/usr/include/droidmedia/
   sudo cp /path/to/droidmedia-devel/usr/lib64/pkgconfig/droidmedia.pc $T/usr/lib64/pkgconfig/
   sudo cp /path/to/droidmedia-devel/usr/lib64/libdroidmedia.a $T/usr/lib64/   # 可选
   ```
4. `mb2 build`，产物在 `RPMS/gstreamer1.0-droid-*.rpm`。
5. 验证（关键）：
   ```sh
   readelf -d usr/lib64/gstreamer-1.0/libgstdroid.so | grep NEEDED | grep -cE "droidmedia|camera_client"
   # 应为 0（或只剩 hybris 的间接依赖）
   nm -D usr/lib64/gstreamer-1.0/libgstdroid.so | grep -c "T droid_media_init"
   # 应为 1（shim 自己定义）
   ```
6. 安装到手机：
   - `/usr/lib64/gstreamer-1.0/libgstdroid.so`
   - `/usr/lib64/libgstdroid-1.0.so.1.0.0`
   - （如果 droidmedia 是自编的，还要替换 `/usr/libexec/droid-hybris/system/lib64/libdroidmedia.so`）
   - 重启后测试相机预览 + 录像 + 播放。

### 13.4 踩过的坑（给下一个设备）

- 相机彩条**不是** gst-droid 版本问题，而是插件**没加载**（查 `journalctl | grep "Failed to load plugin"`）；
- 加 `LD_LIBRARY_PATH` / `ld.so.conf` **解决不了** bionic 依赖问题（`ldconfig` 不索引带 `@` 的 HIDL 库名，
  sailjail 又会剥掉 `LD_LIBRARY_PATH`）；
- `droid_media_init()` 在独立进程里会崩（需要 Android binder），**不要自己在 plugin.c 里调**，
  让 shim 提供定义即可；
- 不同 droidmedia 版本的结构体不同（如 `stride`/`slice_height` 在 MetaData 还是 CodecData），
  编 0928 源码要用 **Sep 版**头文件，编 0508 源码用 **May 版**；
- 构建产物 **unstripped** 也能用，不必纠结体积。

### 13.5 一句话总结

> **gst-droid = 源码（GitHub）+ droidmedia-devel（手机 zypper 拿）+ `hybris.c` 编进插件 + 本地 mb2 构建。**
> 不需要 OBS 账号，也不需要 Jolla 的构建环境。
