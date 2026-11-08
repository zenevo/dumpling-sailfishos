# 5.2.0.22 构建日志（aarch64）

记录本次在 x86-64 主机上从零构建 dumpling 5.2.0.22 镜像的完整过程、踩到的坑和修法，
方便下次复现或换机器重跑。

## 环境

| 项 | 值 |
|---|---|
| 主机 | CachyOS（Arch 系），Ryzen 7 8745H，16 核 / 28 GB，btrfs |
| Platform SDK | `releases.sailfishos.org` 的 `Jolla-latest-...Chroot-i486.tar.bz2`（内含 SFOS 5.1.0.11） |
| tooling | `SailfishOS-5.1.0.11`（i486 tooling tarball） |
| build target | `oneplus-dumpling-aarch64`（aarch64 target tarball） |
| HABUILD | `ubuntu-focal-20210531-android-rootfs.tar.bz2` → `/srv/mer/sdks/ubu`（SDK chroot 内部） |
| Android 树 | `mer-hybris/android` `hybris-16.0`，约 30 GB |

> 5.2.0.22 的官方 SDK target 压缩包还没发布（`latest` 仍是 5.1.0.11），
> 所以用 5.1.0.11 的 tooling/target 编包，最后用 5.2.0.22 的仓库组 rootfs。补丁版本 ABI 兼容，实测可行。

## 关键路径

```
/srv/mer/sdks/sfossdk             Platform SDK chroot
/srv/mer/sdks/sfossdk/srv/mer/sdks/ubu   Ubuntu HABUILD（必须放在 SDK rootfs 内部！）
/srv/mer/toolings/SailfishOS-5.1.0.11
/srv/mer/targets/oneplus-dumpling-aarch64
~/hadk                            Android 源码树（$ANDROID_ROOT）
```

## 步骤

```sh
# 1. Android 源码树
repo init -u https://github.com/mer-hybris/android.git -b hybris-16.0 --depth=1
# 放入 local_manifests/*.xml 后
repo sync -j8 --force-sync --no-clone-bundle

# 2. SDK 与 target
sudo tar --numeric-owner -p -xjf chroot.tar.bz2 -C /srv/mer/sdks/sfossdk
sudo /srv/mer/sdks/sfossdk/sdk-chroot sdk-assistant tooling create SailfishOS-5.1.0.11 tooling.7z
sudo /srv/mer/sdks/sfossdk/sdk-chroot sdk-assistant target create oneplus-dumpling-aarch64 target-aarch64.7z --tooling SailfishOS-5.1.0.11

# 3. HABUILD（注意路径在 SDK rootfs 里）
sudo mkdir -p /srv/mer/sdks/sfossdk/srv/mer/sdks/ubu
sudo tar --numeric-owner -xjf ubuntu-focal.tar.bz2 -C /srv/mer/sdks/sfossdk/srv/mer/sdks/ubu

# 4. 放进三个打包仓库
git clone <droid-hal-dumpling>      $ANDROID_ROOT/rpm
git clone <droid-config-dumpling>   $ANDROID_ROOT/hybris/droid-configs
git clone <droid-hal-version-dumpling> $ANDROID_ROOT/hybris/droid-hal-version-dumpling

# 5. 打 hybris 补丁 + 补 AOSP 项目（见下）
hybris-patches/apply-patches.sh
git clone --recurse-submodules https://github.com/mer-hybris/libhybris.git external/libhybris

# 6. 编 Android HAL（在 HABUILD 里）
cd $ANDROID_ROOT
source build/envsetup.sh
lunch lineage_dumpling-userdebug
make -j12 hybris-hal
make -j12 libdroidmedia libminisf minisfservice minimediaservice

# 7. 打包（Platform SDK 里）
rpm/dhd/helpers/build_packages.sh --droid-hal
rpm/dhd/helpers/build_packages.sh --configs
rpm/dhd/helpers/build_packages.sh --mw
rpm/dhd/helpers/build_packages.sh --gg
rpm/dhd/helpers/build_packages.sh --version

# 8. 出 rootfs
RELEASE=5.2.0.22 EXTRA_NAME=-my1 rpm/dhd/helpers/build_packages.sh --mic
```

## 踩过的坑与修法

1. **`repo sync -c` 漏项目**
   `-c` 只取 manifest 分支，AOSP 里按 tag 固定的项目会整个跳过，编译时报几百个
   “missing …”。改成不带 `-c` 的完整 sync。

2. **mer-hybris 的 manifest 注释掉了 324 个 AOSP 项目**
   只保留 281 个活跃项目。`make hybris-hal` 仍会解析全部 `Android.mk`，于是依赖逐个报缺。
   除了下面第 3 条的 `hybris-patches`，还必须在 local_manifest 里补回：
   `external/one-true-awk`、`external/fsck_msdos`、`external/puffin`、`external/jarjar`、
   `external/xz-embedded`、`external/turbine`、`prebuilts/r8`、`frameworks/opt/net/ims`、
   `external/proguard`、`external/chromium-libpac` 等（见 `~/hadk/.repo/local_manifests/extra-projects.xml`）。

3. **必须先打 hybris-patches**
   `hybris-patches/apply-patches.sh`（65 个补丁）会砍掉大量测试/Java 目标；
   不打的话会像上面那样一路缺件。注意 `--mb` 是 `git am` 模式，需要用干净树。

4. **`libhwc2_compat_layer` 找不到**
   在 `libhybris/compat/hwc2`。按 HADK FAQ：`git clone --recurse-submodules mer-hybris/libhybris external/libhybris`。

5. **`/usr/bin/env python` 不存在**
   AOSP 9 脚本是 python2，SFOS 5.x target 只有 python3。两种修法都做了：
   - Ubuntu chroot 里装 `python2` 并把 `python` 指过去；
   - dhd spec 加 `BuildRequires: python`（5.1 仓库里有 python 2.7 包）。

6. **`tools/metalava/manual` 不存在**
   metalava 在 AOSP 9 manifest 里被注释，但 `droiddoc.mk` 会 `find` 它。`mkdir -p tools/metalava/manual` 即可。

7. **缺 ImageMagick**
   `vendor/lineage/bootanimation` 需要 `convert`：Ubuntu chroot 里 `apt-get install imagemagick`。
   注意 chroot 里的 `/etc/resolv.conf` 要重建（宿主是 systemd-resolved 的 127.0.0.53）。

8. **target 仓库/依赖**
   - `qtscenegraph-adaptation` 等来自 Jolla 的 `jolla-hw/adaptation-common`，target 需要该源；
   - 本地 RPM 需要 `droid-local` 源。mb2 reset 会清掉 `/srv/mer/targets/*/etc/zypp/repos.d` 下手写的
     repo 文件，**要写进 `*.default` 模板**才能持久。
   - `usb-moded-defaults` 与 droid-config 的 `Obsoletes` 冲突，先在 target 里移除。

9. **`/etc/ofono/binder.conf` 文件冲突**
   dhc 基础 sparse 带这个文件，`ofono-configs-binder` 也带。写进 `delete_file.list` 让出。

10. **mic 的 `%pack` 失败**
    rootfs 已生成，只是最后打 zip 的脚本挂了。用 `hybris-installer` 手工打：
    `out/` 下把 tar 改名 `sfos-rootfs.tar.bz2`，套用 `update-binary` 模板并替换
    `%DEVICE% %VERSION% %VERSION_ID% %DATE% %IMAGE_SIZE% %DST_PKG% %LOS_VER%`。

## 产物

```
out/sailfishos-5.2.0.22-20261030-dumpling.zip         614 MB  recovery 可刷
out/sailfishos-5.2.0.22-20261030-dumpling.zip.md5sum
out/sailfishos-5.2.0.22-20261030-dumpling.zip.sha256sum
out/sfe-dumpling-5.2.0.22-my1.tar.bz2                 618 MB  rootfs
```

rootfs 内已确认：`/etc/hw-release`、`/boot/hybris-boot.img`、`/usr/bin/droid/*`、
`/usr/share/ssu/features.d/adaptation-community.ini`（OTA 源）、`/etc/ssu/ssu.ini`（aarch64 / 5.2.0.22）。

## 本版未启用

- 指纹（`sailfish-fpd-community` + `droid-biometry-fp`）：5.2 上还没有现成包，需要从
  `piggz/sailfish-fpd-community` 编（先在 Android 树 `make libbiometry_fp_api`）。
- 一加三段开关/手势（`triambienced`、`gestured`、`onyx-*`）：同理，需在 OBS 或本地补编。

这些在 pattern 里已注释并保留说明，补好包后取消注释、重跑 `--configs` 与 `--mic` 即可。
