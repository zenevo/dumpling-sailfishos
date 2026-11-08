# Wave：Sailfish OS 上的 Spotify 客户端（自建 aarch64 包）

你要的功能（登录、搜索、播放、收藏、建歌单、访问已有歌单）不用从零写——
2026-09 有人开了新项目 [defecador/Wave](https://github.com/defecador/Wave)，
定位就是“SFOS 5.x 的非官方 Spotify 客户端”，并且**内置 librespot 在手机上播放**，
支持 aarch64/armv7hl。它还没有发布二进制，这个包是我从 master（0.2.8，2026-10-19）
自己编的。

## 产物

```
out/harbour-wave-0.2.8-1.aarch64.rpm   4.7 MB
  /usr/bin/harbour-wave
  /usr/share/harbour-wave/qml/...      UI
  /usr/share/harbour-wave/bin/librespot  librespot 0.8.0（本机播放）
```

## 安装

```sh
devel-su zypper --no-gpg-checks in harbour-wave-0.2.8-1.aarch64.rpm
```

依赖 `amber-qml-plugin-mpris`（Jolla 5.2 仓库有，zypper 会自动装）。

## 首次配置

1. 电脑上打开 <https://developer.spotify.com/dashboard>，Create app：
   - Redirect URI 必须**一字不差**：`http://127.0.0.1:8898/callback`
   - 勾选 Web API
2. 复制 **Client ID**（不需要 secret）。
3. 手机打开 Wave → 下拉 → **Account** → 粘贴 Client ID → **Log in with Spotify** →
   浏览器里同意授权，会自动跳回 Wave。
4. 下拉 → **Devices** → 打开 **Play on this phone** → 再点一次 **Log in with Spotify**
   （这一次是 librespot 自己的登录，只需一次）。显示 Ready 后，播放设备里会出现 **Wave**。

## 能做什么 / 不能做什么

✅ 登录（PKCE，无 secret）、搜索、歌单/收藏专辑/喜欢的歌、播放/暂停/下一首/进度/随机、
切 Connect 设备、本机播放、锁屏/耳机/车机控制（MPRIS）、点赞、按链接添加歌单。

⚠️ 限制（大多是 Spotify 侧）：
- 需要在开发者后台建自己的 app；development mode 每个 app 最多 5 个用户。
- 搜索每类最多 10 条（development mode 限制）。
- Spotify 自己生成的歌单（Discover Weekly、Daily Mix）无法列出曲目，只能按链接添加后播放。
- refresh token 目前明文存在 `~/.config/harbour-wave/`（上游 roadmap 里说以后换 Sailfish Secrets）。
- 早期项目，界面/功能还在迭代；Spotify 侧改动也可能随时弄坏 librespot。

## 我是怎么编的（可复现）

1. 在 Platform SDK chroot 里装 rustup（Rust 1.99）+ `aarch64-unknown-linux-gnu` target。
   SDK 自带的 Rust 1.75 太旧，librespot 0.8 要 1.85+。
2. 用 Wave 自带的 `scripts/build-librespot.sh` 思路，把交叉工具换成
   `sb2 -t oneplus-dumpling-aarch64 gcc/ar`，PulseAudio 用目标 sysroot 里的
   `libpulse.so.0` 做链接 stub：
   `cargo build --release --no-default-features --features rustls-tls-webpki-roots,pulseaudio-backend`
3. 把 librespot 二进制放进 `build/deps/librespot/target/aarch64-unknown-linux-gnu/release/`，
   连同源码打 tarball，用 mb2 在 `oneplus-dumpling-aarch64` target 里编。
4. 上游 spec 在 mb2 下 `%build` 不会进解包目录，补了 `cd %{name}-%{version}`。

验证：librespot 在 qemu 下 `--version` 正常（`librespot 0.8.0 d36f9f1`），
依赖只有 libpulse/libpulse-simple/libgcc_s/libm/libc，SFOS 都有。
