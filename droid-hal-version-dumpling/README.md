# droid-hal-version-dumpling

Sailfish OS HW adaptation version / umbrella package for the
**OnePlus 5T (dumpling, A5010)**, targeting **Sailfish OS 5.2.0.22**.

Successor of `sailfishos-oneplus5/droid-hal-version-cheeseburger`.

## Submodule

| name | path | upstream | pinned |
|------|------|----------|--------|
| `droid-hal-version` | `droid-hal-version` | https://github.com/mer-hybris/droid-hal-version | `09c2745` (2026-08-08) |

## Layout

```
$ANDROID_ROOT/
└── hybris/droid-hal-version-dumpling/
    ├── droid-hal-version/                  <- submodule
    └── rpm/droid-hal-version-dumpling.spec
```

Build with:

```sh
cd $ANDROID_ROOT
rpm/dhd/helpers/build_packages.sh --version
```

## Notes

* `have_vibrator_native 1`: msm8998 exposes the vibrator through the kernel
  timed-output driver (`/sys/class/leds/vibrator`), no binder vibrator HAL.
* `have_led 1`: the notification LED is driven through `mce-plugin-libhybris`.
