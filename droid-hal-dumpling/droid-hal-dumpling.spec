# Reference: dhd/droid-hal-device.inc
#
# OnePlus 5T (dumpling) — Sailfish OS 5.2.0.22 (Finlayson)
#
# The Android base is kept at LineageOS 16.0 / Android 9 (Pie), the last
# release with a complete, well tested bring-up for msm8998 OnePlus 5/5T
# (same base as the OnePlus 6/6T community ports that still build on 5.x).

%define vendor oneplus
%define device dumpling

%define vendor_pretty OnePlus
%define device_pretty OnePlus 5T (A5010)

# Android side is 64-bit (msm8998), Sailfish OS userspace is armv7hl.
%define droid_target_aarch64 1

# Allow flashing a new boot.img from Sailfish OS (see droid-config sparse/).
%define enable_kernel_update 1

# Adds the zip updater scripts used by the recovery-flashable image.
%define installable_zip 1

%define android_config \
  #define WANT_ADRENO_QUIRKS 1 \
%{nil}

%define straggler_files \
  /bt_firmware \
  /dsp \
  /firmware \
  /persist \
  /d \
  /product \
  /verity_key \
%{nil}

# Ignore unpackaged (and broken) /bugreports & /sdcard symlinks
%define _unpackaged_files_terminate_build 0

# Don't create systemd mount units for these
%define makefstab_skip_entries /dev/cpuctl /dev/stune /sys/fs/pstore /mnt/vendor/persist

# The AOSP 9 build scripts (fs_config_generator.py, mkbootimg.py, ...) use
# Python 2 syntax, but Sailfish OS 5.x targets only ship python3. The
# "python" package (2.7, still built for aarch64/armv7hl) provides the
# /usr/bin/python these scripts expect.
BuildRequires: python

%include rpm/dhd/droid-hal-device.inc
