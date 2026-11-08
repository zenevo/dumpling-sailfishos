# build/

## `Jolla-@RELEASE@-dumpling-@ARCH@.ks`

用于 `mic` 的 kickstart，已按 **5.2.0.22** 写死仓库：

| token | 值 |
|---|---|
| `@RELEASE@` | `5.2.0.22` |
| `@ARCH@` | `armv7hl` |
| `@EXTRA_NAME@` | 例如 `-my1`（不能含 `.`） |

仓库：

- `adaptation-common` → `releases.jolla.com/releases/@RELEASE@/jolla-hw/adaptation-common/@ARCH@/`
- `adaptation-community-common-dumpling` → `repo.sailfishos.org/obs/nemo:/testing:/hw:/common/sailfishos_5.2_@ARCH@/`
- `adaptation-community-dumpling`（**默认注释**）→ 由 `build_packages.sh --mic` 注入本地
  `file://` 仓库；等 OBS 项目 `nemo:testing:hw:oneplus:dumpling` 发布
  `sailfishos_5.2` 仓库后再取消注释。
- `apps` / `hotfixes` / `jolla` → 官方 5.2.0.22 仓库

两种用法：

```sh
# A. 走 HADK（推荐，会自动注入本地仓库）
cd $ANDROID_ROOT
RELEASE=5.2.0.22 EXTRA_NAME=-my1 rpm/dhd/helpers/build_packages.sh --mic

# B. 独立 mic（需要已经有可用的 5.2 适配包在 OBS/本地仓库）
mic create fs --arch=armv7hl \
  --tokenmap=ARCH:armv7hl,RELEASE:5.2.0.22,EXTRA_NAME:-my1 \
  --outdir=out --pack-to=sfe-dumpling-5.2.0.22.tar.bz2 \
  Jolla-@RELEASE@-dumpling-@ARCH@.ks
```

## `%pack` 做了什么

`%pack` 段在 rootfs 生成后调用
[sailfishos-oneplus5/hybris-installer](https://github.com/sailfishos-oneplus5/hybris-installer)，
把 `sfe-dumpling-5.2.0.22.tar.bz2` 打包成 recovery 可直接刷的
`sailfishos-<版本>-<日期>-dumpling<EXTRA_NAME>.zip`，并生成 md5/sha1/sha256。
