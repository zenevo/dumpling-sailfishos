%package -n patterns-sailfish-device-adaptation-dumpling
Summary: Jolla HW Adaptation dumpling
Requires: droid-hal-dumpling
Requires: droid-hal-dumpling-detritus
Requires: droid-hal-dumpling-img-boot
Requires: droid-hal-dumpling-kernel
Requires: droid-hal-dumpling-kernel-modules
Requires: droid-config-dumpling-sailfish
Requires: droid-config-dumpling-pulseaudio-settings
Requires: droid-config-dumpling-policy-settings
Requires: droid-config-dumpling-preinit-plugin
Requires: droid-config-dumpling-flashing
Requires: droid-config-dumpling-bluez5
Requires: droid-hal-version-dumpling

# Hybris packages
Requires: libhybris-libEGL
Requires: libhybris-libGLESv2
Requires: libhybris-libwayland-egl

# Telephony (Android 9 binder RIL)
Requires: ofono-ril-binder-plugin

# Cellular data
Requires: dummy_netd

# Bluetooth
Requires: bluebinder

# NFC (OnePlus 5T only, the OnePlus 5 has no NFC)
Requires: nfcd-binder-plugin

# Sensors
Requires: hybris-libsensorfw-qt5

# Vibra
Requires: ngfd-plugin-native-vibrator
Requires: libngf-qt5-qtfeedback

# Needed for /dev/touchscreen symlink
Requires: qt5-plugin-generic-evdev

# Audio (Android 9 uses the jb2q / HIDL split)
Requires: pulseaudio-modules-droid-jb2q
Requires: pulseaudio-modules-droid-hidl
# for audio recording to work:
Requires: qt5-qtmultimedia-plugin-mediaservice-gstmediacapture
# needed for the camera and in-call audio on >=Android 9 Qualcomm
Requires: audiosystem-passthrough-dummy-af

# These need to be per-device due to differing backends (fbdev, eglfs, hwc, ..?)
Requires: qt5-qtwayland-wayland_egl
Requires: qt5-qpa-hwcomposer-plugin
Requires: qtscenegraph-adaptation

# For devices using droidmedia & gst-droid
Requires: gstreamer1.0-droid
Requires: gstreamer1.0-libav
Requires: gmp-droid

# This is needed for notification LEDs
Requires: mce-plugin-libhybris

# USB mode controller
Requires: usb-moded
Requires: usb-moded-defaults-android
Requires: usb-moded-developer-mode-android
Requires: usb-moded-connection-sharing-android-config

# hammerhead, grouper, and maguro use this in scripts, so include for all
Requires: rfkill

# For GPS
Requires: geoclue-provider-hybris

# OnePlus 5T specific packages (tri-state alert slider + gestures)
#Requires: triambienced
#Requires: onyx-triambience-settings-plugin
#Requires: gestured
#Requires: onyx-gesture-settings-plugin

%description -n patterns-sailfish-device-adaptation-dumpling
Pattern with packages for dumpling HW Adaptation

%files -n patterns-sailfish-device-adaptation-dumpling
