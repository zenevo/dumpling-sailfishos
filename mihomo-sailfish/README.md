# mihomo on Sailfish OS（Clash.Meta 内核移植）

`ClashMetaForAndroid` 不能直接搬到 Sailfish OS：它是 Kotlin + Jetpack Compose 的
Android 应用，依赖 `VpnService`、前台 Service 和 gomobile 生成的 `.aar`，这些都
需要 Android 运行时（AlienDalvik/AppSupport，社区端口没有授权）。

但它的**内核** mihomo（Clash.Meta）是纯 Go 的 Linux 程序，可以原生跑在 SFOS 上。
本目录就是第一层：内核 + systemd 服务 + 默认配置，打成一个 SFOS rpm。

## 产物

```
out/mihomo-1.19.33-1.aarch64.rpm     # 16 MB，静态二进制 + systemd unit + /etc/mihomo/config.yaml
```

已用 qemu-aarch64 验证：

```
Mihomo Meta v1.19.33 linux arm64 with go1.26.10
```

dumpling 内核配置确认支持 tun 与透明代理：
`CONFIG_TUN=y`、`CONFIG_NETFILTER_XT_TARGET_TPROXY=y`、`CONFIG_NETFILTER_XT_TARGET_REDIRECT=y`、
`CONFIG_NETFILTER_XT_MATCH_SOCKET=y`、`CONFIG_NET_NS=y`。

## 安装与使用

```sh
# 装包
devel-su zypper --no-gpg-checks in mihomo-1.19.33-1.aarch64.rpm

# 放配置（订阅可用 mihomo 自带的 -f 转换，或直接写 proxies/rules）
devel-su vi /etc/mihomo/config.yaml

# 启动
devel-su systemctl enable --now mihomo
devel-su systemctl status mihomo
```

两种用法：

1. **代理模式**（默认，无需 root 权限外的配置）
   `mixed-port: 7890`，支持 SOCKS5/HTTP。浏览器等支持代理的应用填 `127.0.0.1:7890`。
2. **TUN 全局模式**
   把配置里的 `tun:` 段取消注释并设 `enable: true`：
   ```yaml
   tun:
     enable: true
     stack: gvisor
     auto-route: true
     auto-detect-interface: true
     dns-hijack: ["any:53"]
     mtu: 9000
   ```
   服务已经带 `CAP_NET_ADMIN`，`systemctl restart mihomo` 后所有流量走 mihomo。

## 还缺什么（与原 App 的差距）

| ClashMetaForAndroid | 现在 | 说明 |
|---|---|---|
| 图形界面（Compose） | ❌ | 需要写 QML/Silica UI |
| 订阅导入/更新 | 部分 | 可手动放配置；自动更新要写脚本或 UI |
| 分应用代理 | ❌ | Android 用 VpnService 的 allowed/disallowed；SFOS 需 nftables/uid 规则 |
| 规则/策略组编辑 | ❌ | 只能编辑 YAML |
| 流量统计、通知、开机自启 | 部分 | 服务可自启；统计要走 external-controller API |
| 一键开关 | ❌ | 目前 `systemctl`；做 UI 需要一个 root D-Bus helper |

## 下一步（如果要做成“App”）

1. **QML 外壳**：Harbour 应用，读取 `/etc/mihomo/config.yaml`（只读）、显示状态/日志/流量
   （通过 mihomo 的 `external-controller` REST API，默认 `127.0.0.1:9090`）。
2. **特权 helper**：SFOS 应用以 `defaultuser` 运行，不能直接 `systemctl`。
   需要一个小 root 服务（systemd + D-Bus 接口，配 polkit 规则）负责 start/stop/写入配置。
3. **订阅管理**：helper 里用 `mihomo -f <url>` 或直接下载 YAML，校验后原子替换。
4. **分应用代理**：mihomo 支持 `tun.include-uid` / `exclude-uid`，UI 里列出已安装应用即可。

工作量粗估：可用的最小 UI（开关、订阅、日志）1–2 周；接近原 App 的功能面按月计。
