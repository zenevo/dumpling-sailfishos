# Reference: ../droid-hal-version/droid-hal-version.inc

%define vendor oneplus
%define device dumpling

%define vendor_pretty OnePlus
%define device_pretty OnePlus 5T (A5010)

# msm8998 uses the kernel's native vibrator/timed-output driver
%define have_vibrator_native 1
%define have_led 1

%include droid-hal-version/droid-hal-version.inc
