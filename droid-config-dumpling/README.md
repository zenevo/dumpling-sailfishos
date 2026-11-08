# droid-config-dumpling

OBS-buildable / `mb2`-buildable device configuration packages for the
**OnePlus 5T (dumpling, A5010)**, targeting **Sailfish OS 5.2.0.22 (Finlayson)**
on a **LineageOS 16.0 / Android 9** base.

Successor of `sailfishos-oneplus5/droid-config-cheeseburger`; history inherited.

## Submodule (dhc)

| name | path | upstream | pinned |
|------|------|----------|--------|
| `droid-configs-device` | `droid-configs-device` | https://github.com/mer-hybris/droid-hal-configs | `ffddf7f` (2026-10-26) |

`dhc` provides the shared `droid-configs.inc`, the generic `sparse/` tree and
the kickstart pack/attachment plumbing. This repo only adds the OnePlus 5T
specific bits.

## Layout

```
$ANDROID_ROOT/hybris/droid-configs/     <- this repo
├── droid-configs-device/               <- submodule (dhc)
├── rpm/droid-config-dumpling.spec
├── patterns/
│   ├── patterns-sailfish-device-adaptation-dumpling.inc
│   └── patterns-sailfish-device-configuration-dumpling.inc
├── sparse/                             <- device specific overlay
├── kickstart/pack_package-droid-updater
└── delete_file.list
```

## Build

```sh
cd $ANDROID_ROOT
rpm/dhd/helpers/build_packages.sh --configs
```

This produces `droid-config-dumpling*` packages plus the
`patterns-sailfish-device-{adaptation,configuration}-dumpling` patterns.

## What the device overlay provides

* `sparse/usr/libexec/droid-hybris/...` — patched Android init `.rc` files,
  `ld.config.28.txt` and the disabled-services list needed to bring the
  Android 9 msm8998 container up under Sailfish OS.
* `sparse/usr/lib/systemd/system/{system,vendor,data-android}.mount` — explicit
  mount units for the ext4 `/system`, `/vendor` and `/data/android`
  partitions (the phone is A-only, no dynamic partitions).
* `sparse/usr/bin/droid/*` — bind-mount and first-boot helpers.
* `sparse/var/lib/flash-partition/device-info` — `boot` partition is `sde19`.
* `sparse/etc/ofono/ril_subscription.conf` — dual-SIM binder RIL config.
* `sparse/etc/mce/`, `sparse/etc/dconf/` — display, LED, gesture, proximity and
  fingerprint tuning.

## 5.2 changes vs. the 4.3 packaging

* dhc bumped from the 2021 pin to `ffddf7f` (blkid based `flash-partition`,
  bluez5 `uhid` group, ohm/xpolicy fixes).
* patterns updated to the package set that current (`5.x`) community and
  mer-hybris adaptations use — `pulseaudio-modules-droid-jb2q` +
  `-hidl`, `ofono-ril-binder-plugin`, `audiosystem-passthrough-dummy-af`,
  `bluebinder`, `nfcd-binder-plugin`, … — while keeping the OnePlus
  alert-slider/gesture extras.
* `droid-hal-dumpling-kernel` added to the adaptation pattern (the kernel is
  packaged separately from `-img-boot` since the 4.x dhd layout).
