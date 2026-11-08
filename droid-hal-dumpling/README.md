# droid-hal-dumpling

Droid HAL side packaging for the **OnePlus 5T (dumpling, A5010)**, targeting
**Sailfish OS 5.2.0.22 (Finlayson)** on a **LineageOS 16.0 / Android 9** base.

This is the `dumpling` successor of the old
[`sailfishos-oneplus5/droid-hal-cheeseburger`](https://github.com/sailfishos-oneplus5/droid-hal-cheeseburger)
packaging (the old repo covered both OnePlus 5 and 5T under the `cheeseburger`
codename). The git history is inherited from it so the device bring-up
decisions stay traceable.

## Submodule

| name | path | upstream | pinned |
|------|------|----------|--------|
| `dhd` | `dhd` | https://github.com/mer-hybris/droid-hal-device | `775f6c0` (2026-07-01) |

`dhd` is the generic droid-hal-device packaging framework; this repo only
holds the OnePlus 5T specific macros (see `droid-hal-dumpling.spec`).

## Layout expected by the build

In a HADK tree this repository lives at `$ANDROID_ROOT/rpm/`:

```
$ANDROID_ROOT/
├── rpm/                       <- this repo
│   ├── dhd/                   <- submodule, mer-hybris/droid-hal-device
│   └── droid-hal-dumpling.spec
```

Build it with:

```sh
cd $ANDROID_ROOT
rpm/dhd/helpers/build_packages.sh --droid-hal
```

Environment (`~/.hadk.env`) for this device:

```sh
export VENDOR=oneplus
export DEVICE=dumpling
export PORT_ARCH=armv7hl
```

## Notes

* `droid_target_aarch64 1`: the Android side is 64-bit (msm8998), the
  Sailfish OS userspace is `armv7hl`.
* `enable_kernel_update 1`: allows `flash-bootimg.sh` from
  `droid-config-dumpling` to flash `/boot/hybris-boot.img`.
* `installable_zip 1`: adds the recovery updater scripts to the detritus
  package.
